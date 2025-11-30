#include "wireless.h"

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
  Serial.println();
  Serial.print("Connecting... ");
  WiFi.mode(WIFI_STA);
  wm.setWiFiAutoReconnect(true);
  wm.setConnectRetries(5);
  wm.setDarkMode(true);
  wm.setConnectTimeout(5);
  wm.setSaveConnectTimeout(5);

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
  Serial.println("Wifi SSID: " + in);
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
  Serial.println("Wifi Strength: " + String(rssi) + " dB");
  return rssi;
}

String getMacAddress() { return WiFi.macAddress(); }

String getSoftAPSSID() { return WiFi.softAPSSID(); }

String getSoftAPIP() { return WiFi.softAPIP().toString(); }

bool isConnected() { return WiFi.status() == WL_CONNECTED; }

void turnOff()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(20);
  Serial.println("WiFi turned off");
}
} // namespace Wireless
