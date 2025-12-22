#include "http_client.h"

#include "board.h"
#include "sensor.h"
#include "display.h"
#include "logger.h"
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
      m_partialRefresh(false),
      m_jsonPayload("")
{
}

void HttpClient::buildJsonPayload()
{
  // Only build once - if already built, skip
  if (m_jsonPayload.length() > 0)
    return;

  // API and firmware info
  m_jsonDoc["fwVersion"] = firmware;
  m_jsonDoc["apiVersion"] = firmware; // tells server what are firmware capabilities, same as fwVersion in our case
  m_jsonDoc["board"] = Board::getBoardType();

  // System info
  JsonObject system = m_jsonDoc["system"].to<JsonObject>();
  system["cpuTemp"] = Board::getCPUTemperature();
  system["resetReason"] = Board::getResetReasonString();
  system["vccVoltage"] = Board::getBatteryVoltage();

  // Network info
  JsonObject network = m_jsonDoc["network"].to<JsonObject>();
  network["ssid"] = WiFi.SSID();
  network["rssi"] = Wireless::getStrength();
  network["mac"] = Wireless::getMacAddress();
  network["apRetries"] = StateManager::getFailureCount();
  network["ipAddress"] = Wireless::getIPAddress();
  // Add last download duration if available from previous run (in milliseconds)
  if (StateManager::getLastDownloadDuration() > 0)
    network["lastDownloadDuration"] = StateManager::getLastDownloadDuration();

  // Display info
  JsonObject display = m_jsonDoc["display"].to<JsonObject>();
  display["type"] = Display::getDisplayType();
  display["width"] = Display::getResolutionX();
  display["height"] = Display::getResolutionY();
  display["colorType"] = Display::getColorType();
  // Add last refresh duration if available from previous run (in milliseconds)
  if (StateManager::getLastRefreshDuration() > 0)
    display["lastRefreshDuration"] = StateManager::getLastRefreshDuration();

#ifdef SENSOR
  // Add sensor data if available
  SensorData sensorData = Sensor::getInstance().getSensorData();
  if (sensorData.isValid)
  {
    JsonArray sensors = m_jsonDoc["sensors"].to<JsonArray>();
    sensorData.toJson(sensors);
  }
#endif

  serializeJson(m_jsonDoc, m_jsonPayload);
}

bool HttpClient::sendRequest(bool timestampCheck)
{
  // Build JSON payload (only built once, cached for subsequent requests)
  buildJsonPayload();

  Logger::log<Logger::Topic::HTTP>("Connecting to: {}\n", host);

#ifdef USE_CLIENT_HTTPS
  // Configure secure connection without certificate verification
  m_client.setInsecure();
  m_client.setTimeout(15); // Increased timeout for TLS handshake
#endif

  // Try to connect with retries
  for (uint8_t attempt = 0; attempt < 3; attempt++)
  {
    if (m_client.connect(host, CONNECTION_PORT))
      break;

    Logger::log<Logger::Topic::HTTP>("Connection failed, retrying... {}/3\n", attempt + 1);
    if (attempt == 2)
    {
      m_sleepDuration = StateManager::DEFAULT_SLEEP_SECONDS;
      if (!timestampCheck)
        return false; // For image download, fail immediately
      delay(500);
    }
    if (!timestampCheck)
      delay(200);
  }

  // Send HTTP/HTTPS POST request with JSON body
  Logger::log<Logger::Topic::HTTP>("Sending POST to: {}{}/index.php\n", CONNECTION_URL_PREFIX, host);

  // Pretty print JSON payload for debugging
  String prettyJson;
  serializeJsonPretty(m_jsonDoc, prettyJson);
  Logger::log<Logger::Topic::HTTP>("JSON Payload:\n{}\n", prettyJson);

  // Build URL with timestampCheck query parameter
  String url = "/index.php?timestampCheck=";
  url += timestampCheck ? "1" : "0";

  m_client.print(String("POST ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" +
                 "X-API-Key: " + String(Utils::getStoredAPIKey()) + "\r\n" + "Content-Type: application/json\r\n" +
                 "Content-Length: " + String(m_jsonPayload.length()) + "\r\n" + "Connection: close\r\n\r\n" +
                 m_jsonPayload);

  Logger::log<Logger::Topic::HTTP>("Request sent\n");

  // Wait for response with timeout
  uint32_t timeout = millis();
  while (m_client.available() == 0)
  {
    if (millis() - timeout > 10000)
    {
      Logger::log<Logger::Topic::HTTP>(">>> Client Timeout!\n");
      m_client.stop();
      if (timestampCheck)
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
        Logger::log<Logger::Topic::HEADER>("Timestamp now: {}\n", m_serverTimestamp);
      }

      // Let's try to get info about how long to go to deep sleep
      if (line.startsWith("Sleep"))
      {
        uint64_t sleepMinutes = line.substring(7).toInt();
        m_sleepDuration = sleepMinutes * 60; // convert minutes to seconds
        Logger::log<Logger::Topic::HEADER>("Sleep: {}\n", sleepMinutes);
      }

      // Is there another header (after the Sleep one) with sleep in Seconds?
      if (line.startsWith("PreciseSleep"))
      {
        m_sleepDuration = line.substring(14).toInt();
        Logger::log<Logger::Topic::HEADER>("Precise Sleep in seconds: {}\n", m_sleepDuration);
      }

      // Do we want to rotate display? (IE. upside down)
      if (line.startsWith("Rotate"))
      {
        m_displayRotation = line.substring(8).toInt();
        m_hasRotation = true;
        Logger::log<Logger::Topic::HEADER>("Rotation: {}\n", m_displayRotation);
      }

      // Partial refresh request from server (only if this line exists)
      if (line.startsWith("PartialRefresh"))
      {
        m_partialRefresh = true;
        Logger::log<Logger::Topic::HEADER>("Partial refresh requested\n");
      }
    }

    // Check for successful HTTP response (always check)
    if (!connectionOk)
    {
      // Support both HTTP/1.0 and HTTP/1.1
      connectionOk = line.startsWith("HTTP/1.1 200 OK") || line.startsWith("HTTP/1.0 200 OK");
      Logger::log<Logger::Topic::HTTP>("Response: {}\n", line);
    }

    // End of headers
    if (line == "\r")
    {
      Logger::log<Logger::Topic::HTTP>("Headers received\n");
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
  String data = "";
  while (m_client.connected() || m_client.available()) {
    if (m_client.available()) {
      data += m_client.read();  // Read one byte
    }
  }
  m_client.stop();
  Logger::log<Logger::Topic::HTTP>("Byte by byte:{}\n", data);
  /* */

  // If checking timestamp, see if content changed
  if (checkTimestampOnly)
  {
    if (foundTimestamp && (m_serverTimestamp == storedTimestamp))
    {
      Logger::log<Logger::Topic::HTTP>("No screen reload, still at current timestamp: {}\n", storedTimestamp);

      StateManager::setLastRefreshDuration(0);

      return false;
    }

    // Set timestamp to actual one
    StateManager::setTimestamp(m_serverTimestamp);
  }

  return true;
}

bool HttpClient::checkForUpdate(bool timestampCheck)
{
  if (!sendRequest(timestampCheck))
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
  if (!sendRequest(false))
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
  uint16_t result;
  ((uint8_t *)&result)[0] = readByte(); // LSB
  ((uint8_t *)&result)[1] = readByte(); // MSB
  return result;
}
