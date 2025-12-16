/*
 * ZivyObraz.eu - Orchestrate your ePaper displays
 *
 * You need to change some initial things like ePaper type etc. - see below.
 * Default password for Wi-Fi AP is: zivyobraz
 *
 * Kits for ZivyObraz to buy:
 * LáskaKit: https://www.laskakit.cz/vyhledavani/?string=%C5%BEiv%C3%BD+obraz
 * Pájeníčko: https://pajenicko.cz/vyhledavani?search=%C5%BEiv%C3%BD%20obraz
 *
 * Libraries:
 * EPD library: https://github.com/ZinggJM/GxEPD2
 * EPD library for 4G "Grayscale": https://github.com/ZinggJM/GxEPD2_4G
 * PNG Decoder Library: https://github.com/kikuchan/pngle
 * WiFi manager by tzapu https://github.com/tzapu/WiFiManager
 * QRCode generator: https://github.com/ricmoo/QRCode
 * SHT4x (temperature, humidity): https://github.com/adafruit/Adafruit_SHT4X
 * BME280 (temperature, humidity, pressure): https://github.com/adafruit/Adafruit_BME280_Library
 * SCD41 (CO2, temperature, humidity): https://github.com/sparkfun/SparkFun_SCD4x_Arduino_Library
 * STCC4 (CO2, temperature, humidity): https://github.com/Sensirion/arduino-i2c-stcc4
 */

///////////////////////////////////////////////
// Modular architecture includes
///////////////////////////////////////////////

#include "board.h"
#include "display.h"
#include "http_client.h"
#include "image_handler.h"
#include "sensor.h"
#include "state_manager.h"
#include "utils.h"
#include "wireless.h"

///////////////////////////////////////////////
// Configuration
///////////////////////////////////////////////

const char *host = "cdn.zivyobraz.eu";
const char *firmware = "2.4";
const String wifiPassword = "zivyobraz";
const String urlWiki = "https://wiki.zivyobraz.eu";

///////////////////////////////////////////////
// WiFi AP configuration mode callback
///////////////////////////////////////////////

void configModeCallback()
{
  // Increment failure counter
  StateManager::incrementFailureCount();

  // Reset timestamp to force update when reconnected
  StateManager::setTimestamp(0);

  // Show WiFi configuration screen on display
  Display::showWiFiError(Wireless::getSoftAPSSID(), wifiPassword, "http://" + Wireless::getSoftAPIP(), urlWiki);
}

///////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////

String getSensorData()
{
#ifdef SENSOR
  float temperature;
  int humidity;
  int pressure;
  if (Sensor::readSensorsVal(temperature, humidity, pressure))
  {
    String data = "&s=" + String(Sensor::getSensorTypeStr());
    data += "&temp=" + String(temperature) + "&hum=" + String(humidity);

    switch (Sensor::getSensorType())
    {
      case Sensor::SensorType::BME280:
        data += "&pres=" + String(pressure);
        break;
      case Sensor::SensorType::SCD4X:
      case Sensor::SensorType::STCC4:
        data += "&co2=" + String(pressure);
        break;
      default:
        break;
    }
    return data;
  }
#endif
  return "";
}

void initializeWiFi()
{
  String hostname = "INK_" + Wireless::getMacAddress();
  hostname.replace(":", "");
  Wireless::init(hostname, wifiPassword, configModeCallback);
}

void downloadAndDisplayImage(HttpClient &httpClient)
{
  // Enable ePaper power
  Board::setEPaperPowerOn(true);
  delay(500);

  // Partial (fast) refresh if supported, driven by server request
  if (httpClient.hasPartialRefresh() && Display::supportsPartialRefresh())
  {
    Display::setToPartialWindow(0, 0, Display::getResolutionX(), Display::getResolutionY());
  }
  else
  {
    Display::setToFullWindow();
  }

  // Display rotation?
  if (httpClient.hasRotation())
    Display::setRotation(2); // 2 = 180 degrees

  // Get that lovely image and put it on your gorgeous grayscale ePaper screen!
  // If you can't use whole display at once, there will be multiple pages and therefore
  // requests and downloads of one image from server
  Display::setToFirstPage();

  // Store number of pages needed to fill the buffer of the display to turn off the WiFi after last page is loaded
  uint16_t pagesToLoad = Display::getNumberOfPages();

  do
  {
    // For paged displays, download image once per page
    if (!httpClient.startImageDownload())
      break;
    ImageHandler::readImageData(httpClient);

    // turn of WiFi if no more pages left
    if (--pagesToLoad == 0)
    {
      httpClient.stop();
      Wireless::turnOff();
    }
  } while (Display::setToNextPage());

  // Disable ePaper power
  delay(100);
  Board::setEPaperPowerOn(false);

#ifdef ES3ink
  // Show success indicator
  Display::resetPixelColor(0, 0, 150, 0);
#endif
}

void handleConnectedState()
{
  StateManager::resetFailureCount();

  HttpClient httpClient;
  String sensorData = getSensorData();

  if (httpClient.checkForUpdate(sensorData))
  {
    Serial.println("[IMAGE] Update available, downloading...");
    downloadAndDisplayImage(httpClient);
  }
  else
  {
    Serial.println("[IMAGE] No update needed");
  }
}

void handleDisconnectedState()
{
  Serial.println("[WiFi] No Wi-Fi connection, failure count: " + String(StateManager::getFailureCount()));

  // Calculate and set sleep duration based on failure count
  uint64_t sleepDuration = StateManager::calculateSleepDuration();
  StateManager::setSleepDuration(sleepDuration);

  // Reset timestamp to force update on next successful connection
  StateManager::setTimestamp(0);

  // Show error message on display
  Display::showNoWiFiError(sleepDuration, urlWiki);
}

void enterDeepSleepMode()
{
  uint64_t sleepDuration = StateManager::getSleepDuration();
  // Calculate compensation in milliseconds
  unsigned long programRuntimeCompensationMs = millis() - StateManager::getProgramRuntimeCompensationStart();

  // Save compensation time in milliseconds for next run (survives deep sleep)
  StateManager::setLastCompensationTime(programRuntimeCompensationMs);

  // Convert to seconds for sleep duration adjustment, capped to max 60 seconds
  unsigned long programRuntimeCompensation = programRuntimeCompensationMs / 1000;
  if (programRuntimeCompensation > 60)
    programRuntimeCompensation = 60;

  if (programRuntimeCompensation < sleepDuration)
    sleepDuration -= programRuntimeCompensation;

  Serial.print("[SLEEP] Going to sleep for (seconds): ");
  Serial.print(sleepDuration);
  Serial.print(" (compensated by ");
  Serial.print(programRuntimeCompensation);
  Serial.println(" seconds)");

  Board::enterDeepSleepMode(sleepDuration);
}

// Handle special actions with extra button at boot (clear display, reset WiFi credentials)
void handleButtonActions()
{
  unsigned long pressDuration = Board::checkButtonPressDuration();

  // Button not pressed or EXT_BUTTON not defined
  if (pressDuration == 0)
    return;

  // >6 seconds: Reset WiFi credentials and reboot
  if (pressDuration > 6000)
  {
    Serial.println("[BUTTON] Long press detected (>6s): Clearing display and resetting WiFi...");
    Wireless::resetCredentialsAndReboot();
  }
  // >2 seconds: Clear display only
  else if (pressDuration > 2000)
  {
    Serial.println("[BUTTON] Medium press detected (>2s): Clearing display for storage...");
    Display::clear();
    Serial.println("[BUTTON] Display cleared. Entering deep sleep...");

    // Enter deep sleep indefinitely (until next reset)
    Board::enterDeepSleepMode(StateManager::DEFAULT_SLEEP_SECONDS);
  }
  // <2 seconds: Perform normal restart
  else
  {
    Serial.println("[BUTTON] Short press detected (<2s): Restarting ESP...");
    delay(100);
    ESP.restart();
  }
}

///////////////////////////////////////////////
// Main Setup
///////////////////////////////////////////////

void setup()
{
  Serial.begin(115200);
  Serial.println("[SYSTEM] Starting firmware for Zivy Obraz service");

  Board::setupHW();

  handleButtonActions();

  Utils::initializeAPIKey();

  initializeWiFi();

  if (Wireless::isConnected())
    handleConnectedState();
  else
    handleDisconnectedState();

  enterDeepSleepMode();
}

void loop() {}
