#include "wireless.h"

#include "improv_handler.h"
#include "logger.h"

#include <WiFi.h>
#include <WiFiManager.h>

static WiFiManager wm;
static bool configPortalActive = false;

namespace Wireless
{

static void (*userlCallback)() = nullptr;

// This is called if the WifiManager is in config mode (AP open)
void APCallback(WiFiManager *wm)
{
  configPortalActive = true;

  // Start Improv handler immediately when AP mode starts
  ImprovHandler::begin();

  if (userlCallback != nullptr)
    userlCallback();
}

#ifdef USE_EPDIY_DRIVER
// Reboot after saving WiFi credentials. The captive portal leaves the
// TPS65185 PMIC / I2C bus in an inconsistent state that causes epdiy
// display artefacts. A clean restart re-initialises everything correctly.
void saveConfigCallback()
{
  Logger::log<Logger::Topic::WIFI>("WiFi credentials saved, rebooting for clean display...\n");
  delay(500);
  ESP.restart();
}
#endif

void init(const String &hostname, const String &password, void (*callback)())
{
  // Connecting to WiFi
  Logger::log<Logger::Topic::WIFI>("Connecting...\n");
  WiFi.mode(WIFI_STA);
  wm.setWiFiAutoReconnect(true);
  wm.setConnectRetries(5);
  wm.setDarkMode(true);
  wm.setConnectTimeout(5);
  wm.setSaveConnectTimeout(5);

  // Redirect captive portal directly to WiFi setup page
  wm.setCustomHeadElement("<script>if(location.pathname==='/'){location.replace('/wifi');}</script>");

  // Non-blocking mode - allows Improv to process alongside portal
  wm.setConfigPortalBlocking(false);

  // Set callback
  userlCallback = callback;
  wm.setConfigPortalTimeout(240); // set portal time to 4 min, then sleep/try again.
  wm.setAPCallback(APCallback);

#ifdef USE_EPDIY_DRIVER
  // Reboot after saving credentials for clean display state
  wm.setSaveConfigCallback(saveConfigCallback);
#endif

  // Start autoConnect (non-blocking)
  if (wm.autoConnect(hostname.c_str(), password.c_str()))
  {
    Logger::log<Logger::Topic::WIFI>("Connected to WiFi\n");
  }
  else
  {
    Logger::log<Logger::Topic::WIFI>("Config portal started (non-blocking)\n");
  }
}

void process()
{
  // Process WiFiManager portal
  wm.process();

  // Process Improv handler if config portal is active
  if (configPortalActive)
  {
    ImprovHandler::loop();

    // Check if we got connected or portal closed (timeout, etc.)
    if (WiFi.status() == WL_CONNECTED || !wm.getConfigPortalActive())
    {
      configPortalActive = false;
      ImprovHandler::end();
      Logger::log<Logger::Topic::WIFI>("Config portal closed{}\n",
                                       WiFi.status() == WL_CONNECTED ? ", WiFi connected" : "");
    }
  }
}

bool isConfigPortalActive() { return configPortalActive; }

String getSSID()
{
  String in = WiFi.SSID();
  Logger::log<Logger::Topic::WIFI>("SSID: {}\n", in);
  if (in.length() == 0)
    return in;

  String out;
  out.reserve(in.length() * 3);
  const char *hex = "0123456789ABCDEF";

  for (size_t i = 0; i < in.length(); ++i)
  {
    char c = in[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
        c == '.' || c == '~')
    {
      out += c;
    }
    else
    {
      char buf[4];
      buf[0] = '%';
      buf[1] = hex[(c >> 4) & 0x0F];
      buf[2] = hex[c & 0x0F];
      buf[3] = '\0';
      out += buf;
    }
  }
  return out;
}

int8_t getStrength()
{
  int8_t rssi = WiFi.RSSI();
  Logger::log<Logger::Topic::WIFI>("Strength: {} dB\n", rssi);
  return rssi;
}

String getMacAddress() { return WiFi.macAddress(); }

String getSoftAPSSID() { return WiFi.softAPSSID(); }

String getSoftAPIP() { return WiFi.softAPIP().toString(); }

String getIPAddress() { return WiFi.localIP().toString(); }

bool isConnected() { return WiFi.status() == WL_CONNECTED; }

void turnOff()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(20);
  Logger::log<Logger::Topic::WIFI>("WiFi turned off\n");
}

void resetCredentialsAndReboot()
{
  // Disconnect WiFi
  turnOff();

  // Reset WiFi settings (erase stored credentials)
  Logger::log<Logger::Topic::WIFI>("Erasing stored credentials...\n");
  wm.resetSettings();

  // Restart ESP to start configuration portal
  Logger::log<Logger::Topic::SYSTEM>("Rebooting ESP...\n");
  ESP.restart();
}
} // namespace Wireless
