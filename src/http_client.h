#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

// Select http/https connection type (default: https)
#ifndef USE_CLIENT_HTTP
  #define USE_CLIENT_HTTPS
#endif

#ifdef USE_CLIENT_HTTP
  #include <WiFiClient.h>
  #define CONNECTION_PORT 80
  #define CONNECTION_URL_PREFIX "http://"
#else
  #include <WiFiClientSecure.h>
  #define CONNECTION_PORT 443
  #define CONNECTION_URL_PREFIX "https://"
#endif

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

class HttpClient
{
public:
  HttpClient();

  // Check if server has new content
  // If keepConnectionOpen is true and update is available, connection stays open for immediate image reading
  bool checkForUpdate(bool timestampCheck = true, bool keepConnectionOpen = false);

  // Check if image data is ready to be read (connection kept open after checkForUpdate)
  bool hasImageDataReady() const { return m_imageDataReady; }

  // Start downloading image (call after checkForUpdate returns true)
  // Not needed if checkForUpdate was called with keepConnectionOpen=true
  bool startImageDownload();

  // Connection status
  bool isConnected();
  int available();
  void stop();

  // Server response values
  uint64_t getSleepDuration() const { return m_sleepDuration; }

  uint64_t getServerTimestamp() const { return m_serverTimestamp; }

  uint8_t getDisplayRotation() const { return m_displayRotation; }

  bool hasRotation() const { return m_hasRotation; }

  bool hasPartialRefresh() const { return m_partialRefresh; }

  bool hasOTAUpdate() const { return m_otaRequired; }

  String getOTAUrl() const { return m_otaUrl; }

  // Data reading methods - no WiFiClient exposure!
  uint32_t readBytes(uint8_t *buf, int32_t bytes);

  uint32_t skip(int32_t bytes) { return readBytes(nullptr, bytes); }

  uint8_t readByte();
  uint8_t readByteValid(bool *valid);
  uint16_t read16();

private:
#ifdef USE_CLIENT_HTTP
  WiFiClient m_client;
#else
  WiFiClientSecure m_client;
#endif

  // Server response data
  uint64_t m_sleepDuration;
  uint64_t m_serverTimestamp;
  uint8_t m_displayRotation;
  bool m_hasRotation;
  bool m_partialRefresh;
  bool m_otaRequired;
  String m_otaUrl;
  bool m_imageDataReady;
  String m_jsonPayload;
  JsonDocument m_jsonDoc;

  // Internal helpers
  void buildJsonPayload();
  bool sendRequest(bool timestampCheck);
  bool parseHeaders(bool checkTimestampOnly, uint64_t storedTimestamp);
};

#endif // HTTP_CLIENT_H
