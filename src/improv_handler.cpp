#include "improv_handler.h"
#include "board.h"
#include "display.h"
#include "logger.h"

#include <Arduino.h>
#include <ImprovWiFiLibrary.h>
#include <WiFi.h>

// External configuration
extern const char *firmware;

namespace ImprovHandler
{

static ImprovWiFi improvSerial(&Serial);
static bool active = false;
static bool initialized = false;

static void onImprovWiFiConnected(const char *ssid, const char *password)
{
  Logger::log<Logger::Level::DEBUG, Logger::Topic::WIFI>("Improv: Credentials received, connecting...\n");

  // Stop any existing WiFi connection (WiFiManager's AP)
  WiFi.disconnect();
  delay(100);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection with timeout
  unsigned long startTime = millis();
  constexpr unsigned long connectionTimeout = 15000; // 15 seconds

  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < connectionTimeout)
  {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Logger::log<Logger::Level::INFO, Logger::Topic::WIFI>("Improv: WiFi connected successfully, restarting...\n");
    delay(500);
    ESP.restart();
  }
  else
  {
    Logger::log<Logger::Level::WARNING, Logger::Topic::WIFI>("Improv: WiFi connection failed\n");
  }
}

void begin()
{
  if (active)
    return;

  // Initialize Improv only once
  if (!initialized)
  {
    static String deviceName = String(Board::getBoardType()) + " + " + Display::getDisplayType();
    improvSerial.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32, "ZivyObraz.eu", firmware, deviceName.c_str(), "");
    improvSerial.onImprovConnected(onImprovWiFiConnected);
    initialized = true;
  }

  active = true;
  Logger::log<Logger::Level::DEBUG, Logger::Topic::WIFI>("Improv: Handler started\n");
}

void loop()
{
  if (!active)
    return;

  // Process Improv serial data (non-blocking)
  improvSerial.handleSerial();
}

void end()
{
  active = false;
  Logger::log<Logger::Level::DEBUG, Logger::Topic::WIFI>("Improv: Handler stopped\n");
}

bool isActive() { return active; }

void busyCallback(const void *)
{
  // Process Improv serial data during display busy wait
  if (active)
    improvSerial.handleSerial();
}

} // namespace ImprovHandler
