#include "wireless.h"

#include "improv_handler.h"
#include "logger.h"

#include <array>
#include <esp_wifi.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiManager.h>

extern const char *const NVS_NAMESPACE;

static WiFiManager wm;
static Preferences prefs;
static bool configPortalActive = false;

namespace Wireless
{

static void (*userlCallback)() = nullptr;

static const unsigned long FAST_CONNECT_TIMEOUT_MS = 4000;
static constexpr size_t BSSID_LEN = sizeof(wifi_sta_config_t::bssid);

struct WifiCache
{
  std::array<uint8_t, BSSID_LEN> bssid;
  uint8_t channel;

  bool isValid() const { return channel != 0; }

  bool operator==(const WifiCache &other) const { return channel == other.channel && bssid == other.bssid; }
};

static void formatBssid(const uint8_t *bssid, char *buffer, size_t bufferSize)
{
  snprintf(buffer, bufferSize, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4],
           bssid[5]);
}

static WifiCache loadWifiCache()
{
  WifiCache cache{};
  prefs.begin(NVS_NAMESPACE, true);
  cache.channel = prefs.getUChar("channel", 0);
  if (!cache.isValid())
  {
    prefs.end();
    return cache;
  }

  size_t len = prefs.getBytes("bssid", cache.bssid.data(), cache.bssid.size());
  prefs.end();

  if (len != cache.bssid.size())
    cache = {};
  return cache;
}

static void saveWifiCache(const WifiCache &cache)
{
  prefs.begin(NVS_NAMESPACE);
  prefs.putBytes("bssid", cache.bssid.data(), cache.bssid.size());
  prefs.putUChar("channel", cache.channel);
  prefs.end();
}

static void invalidateWifiCache()
{
  prefs.begin(NVS_NAMESPACE);
  prefs.remove("channel");
  prefs.remove("bssid");
  prefs.end();
}

void invalidateConnectionCache() { invalidateWifiCache(); }

// This is called if the WifiManager is in config mode (AP open)
void APCallback(WiFiManager *wm)
{
  configPortalActive = true;

  // Start Improv handler immediately when AP mode starts
  ImprovHandler::begin();

  if (userlCallback != nullptr)
    userlCallback();
}

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

  // Start autoConnect (non-blocking)
  if (wm.autoConnect(hostname.c_str(), password.c_str()))
    Logger::log<Logger::Topic::WIFI>("Connected to WiFi\n");
  else
    Logger::log<Logger::Topic::WIFI>("Config portal started (non-blocking)\n");
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

bool tryFastConnect()
{
  auto cached = loadWifiCache();
  if (!cached.isValid())
  {
    Logger::log<Logger::Level::DEBUG, Logger::Topic::WIFI>("No cached BSSID/channel, skipping fast connect\n");
    return false;
  }

  char bssidStr[18];
  formatBssid(cached.bssid.data(), bssidStr, sizeof(bssidStr));
  Logger::log<Logger::Level::DEBUG, Logger::Topic::WIFI>("Fast connect: channel {} BSSID {}\n", cached.channel,
                                                         bssidStr);

  // Initialize WiFi in STA mode and start the driver to access NVS credentials
  WiFi.mode(WIFI_STA);

  // Read stored SSID and password from NVS via ESP-IDF API
  wifi_config_t conf;
  esp_wifi_get_config(WIFI_IF_STA, &conf);

  const char *ssid = reinterpret_cast<const char *>(conf.sta.ssid);
  const char *pass = reinterpret_cast<const char *>(conf.sta.password);

  if (strlen(ssid) == 0)
  {
    Logger::log<Logger::Level::DEBUG, Logger::Topic::WIFI>("No stored SSID in NVS, skipping fast connect\n");
    invalidateWifiCache();
    return false;
  }

  // Attempt direct connection with cached BSSID and channel (skips scan)
  WiFi.begin(ssid, pass, cached.channel, cached.bssid.data());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < FAST_CONNECT_TIMEOUT_MS)
  {
    delay(10);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Logger::log<Logger::Level::INFO, Logger::Topic::WIFI>("Fast connect successful in {} ms\n", millis() - start);
    return true;
  }

  // Fast connect failed — invalidate cache and let caller fall back to WiFiManager
  Logger::log<Logger::Level::WARNING, Logger::Topic::WIFI>("Fast connect failed after {} ms, invalidating cache\n",
                                                           millis() - start);
  WiFi.disconnect(true);
  invalidateWifiCache();
  return false;
}

void saveConnectionCache()
{
  if (WiFi.status() != WL_CONNECTED)
    return;

  const uint8_t *bssid = WiFi.BSSID();
  uint8_t channel = WiFi.channel();
  if (bssid == nullptr || channel == 0)
    return;

  // Create current cache
  WifiCache current;
  current.channel = channel;
  std::copy(bssid, bssid + BSSID_LEN, current.bssid.begin());

  // Compare with cached version
  auto cached = loadWifiCache();
  if (cached.isValid() && cached == current)
    return;

  saveWifiCache(current);
  char bssidStr[18];
  formatBssid(bssid, bssidStr, sizeof(bssidStr));
  Logger::log<Logger::Level::DEBUG, Logger::Topic::WIFI>("Saved BSSID/channel cache: ch {} BSSID {}\n", channel,
                                                         bssidStr);
}

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

  // Invalidate BSSID/channel cache
  invalidateWifiCache();

  // Reset WiFi settings (erase stored credentials)
  Logger::log<Logger::Topic::WIFI>("Erasing stored credentials...\n");
  wm.resetSettings();

  // Restart ESP to start configuration portal
  Logger::log<Logger::Topic::SYSTEM>("Rebooting ESP...\n");
  ESP.restart();
}
} // namespace Wireless
