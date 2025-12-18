#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>

class HttpClient
{
public:
  HttpClient();

  // Check if server has new content
  bool checkForUpdate(const String &sensorData = "");

  // Start downloading image (call after checkForUpdate returns true)
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

  // Data reading methods - no WiFiClient exposure!
  uint32_t readBytes(uint8_t *buf, int32_t bytes);

  uint32_t skip(int32_t bytes) { return readBytes(nullptr, bytes); }

  uint8_t readByte();
  uint8_t readByteValid(bool *valid);
  uint16_t read16();

private:
  WiFiClient m_client;

  // Server response data
  uint64_t m_sleepDuration;
  uint64_t m_serverTimestamp;
  uint8_t m_displayRotation;
  bool m_hasRotation;
  bool m_partialRefresh;

  // Internal helpers
  bool sendRequest(bool timestampCheckOnly, const String &extraParams);
  bool parseHeaders(bool checkTimestampOnly, uint64_t storedTimestamp);
};

#endif // HTTP_CLIENT_H
