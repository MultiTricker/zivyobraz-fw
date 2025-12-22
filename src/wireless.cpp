#include "wireless.h"

#include "logger.h"

#include <WiFi.h>
#include <WiFiManager.h>

static WiFiManager wm;

namespace Wireless
{

static void (*userlCallback)() = nullptr;

// This is called if the WifiManager is in config mode (AP open)
void APCallback(WiFiManager *wm)
{
  if (userlCallback != nullptr)
    userlCallback();
}

void init(const String &hostname, const String &password, void (*callback)())
{
  // Connecting to WiFi
  Logger::log(Logger::Topic::WIFI, "Connecting...\n");
  WiFi.mode(WIFI_STA);
  wm.setWiFiAutoReconnect(true);
  wm.setConnectRetries(5);
  wm.setDarkMode(true);
  wm.setConnectTimeout(5);
  wm.setSaveConnectTimeout(5);

  // Redirect captive portal directly to WiFi setup page
  wm.setCustomHeadElement("<script>if(location.pathname==='/'){location.replace('/wifi');}</script>");

  // reset settings - wipe stored credentials for testing
  // wm.resetSettings();

  // Set callback
  userlCallback = callback;
  wm.setConfigPortalTimeout(240); // set portal time to 4 min, then sleep/try again.
  wm.setAPCallback(APCallback);
  wm.autoConnect(hostname.c_str(), password.c_str());
}

String getSSID()
{
  String in = WiFi.SSID();
  Logger::log(Logger::Topic::WIFI, "SSID: {}\n", in);
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
  Logger::log(Logger::Topic::WIFI, "Strength: {} dB\n", rssi);
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
  Logger::log(Logger::Topic::WIFI, "WiFi turned off\n");
}

void resetCredentialsAndReboot()
{
  // Disconnect WiFi
  turnOff();

  // Reset WiFi settings (erase stored credentials)
  Logger::log(Logger::Topic::WIFI, "Erasing stored credentials...\n");
  wm.resetSettings();

  // Restart ESP to start configuration portal
  Logger::log(Logger::Topic::SYSTEM, "Rebooting ESP...\n");
  ESP.restart();
}
} // namespace Wireless
