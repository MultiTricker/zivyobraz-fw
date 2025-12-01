#include "http_client.h"

#include "board.h"
#include "display.h"
#include "state_manager.h"
#include "wireless.h"

// External configuration
extern const char *host;
extern const char *firmware;

HttpClient::HttpClient()
    : m_sleepDuration(StateManager::DEFAULT_SLEEP_SECONDS),
      m_serverTimestamp(0),
      m_displayRotation(0),
      m_hasRotation(false),
      m_partialRefresh(false)
{
}

bool HttpClient::sendRequest(bool timestampCheckOnly, const String &extraParams)
{
  // Build URL
  String url = "/index.php?mac=" + Wireless::getMacAddress();

  if (timestampCheckOnly)
    url += "&timestamp_check=1";

  url += "&rssi=" + String(Wireless::getStrength());
  url += "&ssid=" + Wireless::getSSID();
  url += "&v=" + String(Board::getBatteryVoltage());
  url += "&x=" + String(Display::getResolutionX());
  url += "&y=" + String(Display::getResolutionY());
  url += "&c=" + String(Display::getColorType());
  url += "&fw=" + String(firmware);
  url += "&ap_retries=" + String(StateManager::getFailureCount());
  url += extraParams;

  Serial.print("Connecting to: ");
  Serial.println(host);

  // Try to connect with retries
  for (uint8_t attempt = 0; attempt < 3; attempt++)
  {
    if (m_client.connect(host, 80))
      break;

    Serial.println("Connection failed, retrying... " + String(attempt + 1) + "/3");
    if (attempt == 2)
    {
      m_sleepDuration = StateManager::DEFAULT_SLEEP_SECONDS;
      if (!timestampCheckOnly)
        return false; // For image download, fail immediately
      delay(500);
    }
    if (!timestampCheckOnly)
      delay(200);
  }

  // Send HTTP request
  Serial.print("Requesting URL: http://");
  Serial.print(host);
  Serial.println(url);

  m_client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
  Serial.println("Request sent");

  // Wait for response with timeout
  uint32_t timeout = millis();
  while (m_client.available() == 0)
  {
    if (millis() - timeout > 10000)
    {
      Serial.println(">>> Client Timeout!");
      m_client.stop();
      if (timestampCheckOnly)
        m_sleepDuration = StateManager::DEFAULT_SLEEP_SECONDS;
      return false;
    }
  }

  return true;
}

bool HttpClient::parseHeaders(bool checkTimestampOnly, uint64_t storedTimestamp)
{
  bool connectionOk = false;
  bool foundTimestamp = false;
  m_hasRotation = false;
  m_partialRefresh = false;

  while (m_client.connected())
  {
    String line = m_client.readStringUntil('\n');

    // Parse headers only when checking timestamp
    if (checkTimestampOnly)
    {
      // Parse Timestamp header
      if (line.startsWith("Timestamp"))
      {
        foundTimestamp = true;
        // Skipping also colon and space - also in following code for sleep, rotate, ...
        m_serverTimestamp = line.substring(11).toInt();
        Serial.print("Timestamp now: ");
        Serial.println(m_serverTimestamp);
      }

      // Let's try to get info about how long to go to deep sleep
      if (line.startsWith("Sleep"))
      {
        uint64_t sleepMinutes = line.substring(7).toInt();
        m_sleepDuration = sleepMinutes * 60; // convert minutes to seconds
        Serial.print("Sleep: ");
        Serial.println(sleepMinutes);
      }

      // Is there another header (after the Sleep one) with sleep, but in Seconds?
      if (line.startsWith("SleepSeconds"))
      {
        m_sleepDuration = line.substring(14).toInt(); // already in seconds

        // Deal with program runtime compensation (if not already set) for more accurate sleep time in seconds
        if (StateManager::getProgramRuntimeCompensation() == 0)
          StateManager::setProgramRuntimeCompensation(millis());

        Serial.print("SleepSeconds: ");
        Serial.println(m_sleepDuration);
      }

      // Do we want to rotate display? (IE. upside down)
      if (line.startsWith("Rotate"))
      {
        m_displayRotation = line.substring(8).toInt();
        m_hasRotation = true;
        Serial.print("Rotate: ");
        Serial.println(m_displayRotation);
      }

      // Partial refresh request from server (only if this line exists)
      if (line.startsWith("PartialRefresh"))
      {
        m_partialRefresh = true;
        Serial.println("Partial refresh requested");
      }
    }

    // Check for successful HTTP response (always check)
    if (!connectionOk)
    {
      // Support both HTTP/1.0 and HTTP/1.1
      connectionOk = line.startsWith("HTTP/1.1 200 OK") || line.startsWith("HTTP/1.0 200 OK");
      Serial.println(line);
    }

    // End of headers
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
  }

  // Is there a problem? Fallback to default deep sleep time to try again soon
  if (!connectionOk)
  {
    m_sleepDuration = StateManager::DEFAULT_SLEEP_SECONDS;
    return false;
  }

  // For debug purposes - print out the whole response
  /*
  Serial.println("Byte by byte:");

  while (m_client.connected() || m_client.available()) {
    if (m_client.available()) {
      char c = m_client.read();  // Read one byte
      Serial.print(c);         // Print the byte to the serial monitor
    }
  }
  m_client.stop();
  /* */

  // If checking timestamp, see if content changed
  if (checkTimestampOnly)
  {
    if (foundTimestamp && (m_serverTimestamp == storedTimestamp))
    {
      Serial.print("No screen reload, because we already are at current timestamp: ");
      Serial.println(storedTimestamp);
      return false;
    }

    // Set timestamp to actual one
    StateManager::setTimestamp(m_serverTimestamp);
  }

  return true;
}

bool HttpClient::checkForUpdate(const String &sensorData)
{
  if (!sendRequest(true, sensorData))
    return false;

  if (!parseHeaders(true, StateManager::getTimestamp()))
  {
    m_client.stop();
    return false;
  }

  // Update sleep duration from headers
  StateManager::setSleepDuration(m_sleepDuration);

  m_client.stop();
  return true; // Update available
}

bool HttpClient::startImageDownload()
{
  if (!sendRequest(false, ""))
    return false;

  if (!parseHeaders(false, 0))
  {
    m_client.stop();
    return false;
  }

  // Client stays open for image data
  return true;
}

bool HttpClient::isConnected() { return m_client.connected() || m_client.available(); }

int HttpClient::available() { return m_client.available(); }

void HttpClient::stop() { m_client.stop(); }

uint32_t HttpClient::readBytes(uint8_t *buf, int32_t bytes)
{
  int32_t remaining = bytes;
  uint32_t startTime = millis();

  while ((m_client.connected() || m_client.available()) && remaining > 0)
  {
    if (m_client.available())
    {
      int16_t val = m_client.read();
      if (buf)
        *buf++ = (uint8_t)val;
      remaining--;
    }
    else
    {
      delay(1);
    }
    if (millis() - startTime > 2000)
      break; // Timeout
  }

  return bytes - remaining;
}

uint8_t HttpClient::readByte()
{
  uint8_t result;
  readBytes(&result, 1);
  return result;
}

uint8_t HttpClient::readByteValid(bool *valid)
{
  uint8_t result;
  *valid = readBytes(&result, 1) == 1;
  return result;
}

uint16_t HttpClient::read16()
{
  // BMP data is little-endian
  uint16_t result;
  ((uint8_t *)&result)[0] = readByte(); // LSB
  ((uint8_t *)&result)[1] = readByte(); // MSB
  return result;
}

uint32_t HttpClient::read32()
{
  // BMP data is little-endian
  uint32_t result;
  ((uint8_t *)&result)[0] = readByte(); // LSB
  ((uint8_t *)&result)[1] = readByte();
  ((uint8_t *)&result)[2] = readByte();
  ((uint8_t *)&result)[3] = readByte(); // MSB
  return result;
}
