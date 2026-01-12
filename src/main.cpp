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
 * ArduinoJSON: https://github.com/bblanchon/ArduinoJson
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
#include "logger.h"
#include "state_manager.h"
#include "streaming_handler.h"
#include "wireless.h"

///////////////////////////////////////////////
// Configuration
///////////////////////////////////////////////

const char *host = "cdn.zivyobraz.eu";
const char *firmware = "3.0";
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

  // Show WiFi configuration screen on display only if ShowNoWifiError is enabled (default: 1)
  if (StateManager::getShowNoWifiError() == 1)
  {
    Display::showWiFiError(Wireless::getSoftAPSSID(), wifiPassword, "http://" + Wireless::getSoftAPIP(), urlWiki);
  }
  else
  {
    Logger::log<Logger::Level::DEBUG, Logger::Topic::DISP>(
      "ShowNoWifiError disabled, not showing AP configuration screen\n");
  }
}

///////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////

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

  // Start tracking download duration
  StateManager::startDownloadTimer();

  // Check if direct streaming mode is available and should be used
  // (direct streaming doesn't support rotation - displays require sequential row writes)
  bool useDirectStreaming = ImageHandler::isDirectStreamingAvailable() && !httpClient.hasRotation();

  if (useDirectStreaming)
  {
    Logger::log<Logger::Level::INFO, Logger::Topic::IMAGE>("Using direct streaming mode\n");

    // Determine if partial refresh should be used BEFORE initializing display
    bool usePartialRefresh = httpClient.hasPartialRefresh() && Display::supportsPartialRefresh();

    // Initialize display for direct streaming with partial refresh flag and max row count
    // This must be done BEFORE writing any image data
#ifdef STREAMING_ENABLED
    Display::initDirectStreaming(usePartialRefresh, STREAMING_BUFFER_ROWS_COUNT);
#else
    Display::initDirectStreaming(usePartialRefresh);
#endif

    // Display rotation?
    if (httpClient.hasRotation())
      Display::setRotation(2);

    // Check if image data is already available (from checkForUpdate with keepConnectionOpen)
    // If not, try to start a new download (shouldn't happen in normal flow)
    bool connectionReady = httpClient.hasImageDataReady();
    if (connectionReady)
    {
      Logger::log<Logger::Level::DEBUG, Logger::Topic::IMAGE>("Using existing connection from timestamp check\n");
    }
    else
    {
      Logger::log<Logger::Level::WARNING, Logger::Topic::IMAGE>("Starting separate image download\n");
      connectionReady = httpClient.startImageDownload();
    }

    // Stream image data directly to display buffer
    bool success = connectionReady && ImageHandler::readImageDataDirect(httpClient);

    // Always close connection before proceeding
    httpClient.stop();

    if (success)
    {
      StateManager::endDownloadTimer();
      Wireless::turnOff();

      // Enable light sleep during refresh
      Display::enableLightSleepDuringRefresh(true);
      StateManager::startRefreshTimer();

      // Finish streaming (triggers display refresh)
      Display::finishDirectStreaming();

      Display::enableLightSleepDuringRefresh(false);
      StateManager::endRefreshTimer();
    }
    else
    {
      Logger::log<Logger::Level::WARNING, Logger::Topic::IMAGE>(
        "Direct streaming failed, falling back to paged mode\n");
      useDirectStreaming = false; // Fall through to paged mode below
    }
  }

  if (!useDirectStreaming)
  {
    // Fall back to paged mode (original behavior)
    Logger::log<Logger::Level::INFO, Logger::Topic::IMAGE>("Using paged mode (multiple downloads)\n");

    // Partial (fast) refresh if supported, driven by server request
    if (httpClient.hasPartialRefresh() && Display::supportsPartialRefresh())
      Display::setToPartialWindow(0, 0, Display::getResolutionX(), Display::getResolutionY());
    else
      Display::setToFullWindow();

    // Display rotation?
    if (httpClient.hasRotation())
      Display::setRotation(2);

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
        // End download timing before WiFi turns off
        StateManager::endDownloadTimer();
        Wireless::turnOff();

        // Enable light sleep during display refresh to save power
        Display::enableLightSleepDuringRefresh(true);

        // Start refresh timing after WiFi is off
        StateManager::startRefreshTimer();
      }
    } while (Display::setToNextPage());

    // Disable light sleep callback after refresh completes
    Display::enableLightSleepDuringRefresh(false);
  }

  // Disable ePaper power
  delay(100);
  Board::setEPaperPowerOn(false);

  // End refresh timing
  StateManager::endRefreshTimer();

#ifdef ES3ink
  // Show success indicator
  Display::resetPixelColor(0, 0, 150, 0);
#endif
}

void handleConnectedState()
{
  StateManager::resetFailureCount();

  HttpClient httpClient;

  // For direct streaming mode, keep connection open to avoid second request
  // For paged mode, close connection (will reopen for each page)
  bool useDirectStreaming = ImageHandler::isDirectStreamingAvailable();

  if (httpClient.checkForUpdate(true, useDirectStreaming))
  {
    // Re-evaluate direct streaming: rotation requires paged mode
    // (displays require sequential row writes, can't handle rotation in streaming)
    if (useDirectStreaming && httpClient.hasRotation())
    {
      Logger::log<Logger::Level::INFO, Logger::Topic::IMAGE>(
        "Rotation requested, switching from direct streaming to paged mode\n");
      useDirectStreaming = false;
      // Close the kept-open connection - paged mode will reopen for each page
      httpClient.stop();
    }

    // Check if OTA update is requested by server
    if (httpClient.hasOTAUpdate())
    {
      if (!httpClient.performOTAUpdate())
      {
        // OTA failed - go to sleep for default duration to retry later
        StateManager::setSleepDuration(StateManager::DEFAULT_SLEEP_SECONDS);
      }
      // If OTA succeeded, device restarts automatically and we never reach here
      return;
    }

    Logger::log<Logger::Topic::IMAGE>("Update available, downloading...\n");
    downloadAndDisplayImage(httpClient);
  }
  else
  {
    Logger::log<Logger::Topic::IMAGE>("No update needed\n");
  }
}

void handleDisconnectedState()
{
  Logger::log<Logger::Level::ERROR, Logger::Topic::WIFI>("No Wi-Fi connection, failure count: {}\n",
                                                         StateManager::getFailureCount());

  // Calculate and set sleep duration based on failure count
  uint64_t sleepDuration = StateManager::calculateSleepDuration();
  StateManager::setSleepDuration(sleepDuration);

  // Reset timestamp to force update on next successful connection
  StateManager::setTimestamp(0);

  // Show error message on display only if ShowNoWifiError is enabled (default: 1)
  if (StateManager::getShowNoWifiError() == 1)
  {
    Display::showNoWiFiError(sleepDuration, urlWiki);
  }
  else
  {
    Logger::log<Logger::Level::DEBUG, Logger::Topic::DISP>(
      "ShowNoWifiError disabled, keeping existing display content\n");
  }
}

void enterDeepSleepMode()
{
  uint64_t sleepDuration = StateManager::getSleepDuration();

  // Get total compensation from download + refresh durations (in milliseconds)
  unsigned long totalCompensation = StateManager::getTotalCompensation();

  // Convert to seconds for sleep duration adjustment, capped to max 60 seconds
  unsigned long compensationSeconds = totalCompensation / 1000;
  if (compensationSeconds > 60)
    compensationSeconds = 60;

  if (compensationSeconds < sleepDuration)
    sleepDuration -= compensationSeconds;

  Logger::log<Logger::Topic::SYSTEM>("Going to sleep for (seconds): {} (compensated by {} seconds)\n", sleepDuration,
                                     compensationSeconds);

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
    Logger::log<Logger::Topic::BTN>("Long press detected (>6s): Clearing display and resetting WiFi...\n");
    Wireless::resetCredentialsAndReboot();
  }
  // >2 seconds: Clear display only
  else if (pressDuration > 2000)
  {
    Logger::log<Logger::Topic::BTN>("Medium press detected (>2s): Clearing display for storage...\n");
    Display::clear();
    Logger::log<Logger::Topic::BTN>("Display cleared. Entering deep sleep...\n");

    // Enter deep sleep indefinitely (until next reset)
    Board::enterDeepSleepMode(StateManager::DEFAULT_SLEEP_SECONDS);
  }
  // <2 seconds: Perform normal restart
  else
  {
    Logger::log<Logger::Topic::BTN>("Short press detected (<2s): Restarting ESP...\n");
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
  Logger::log<Logger::Topic::SYSTEM>("Starting firmware for Zivy Obraz service\n");

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
