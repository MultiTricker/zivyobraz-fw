#include "utils.h"

#include <Preferences.h>
#include <esp_random.h>

// RTC persistent data for PIN cache (survives deep sleep)
RTC_DATA_ATTR uint32_t rtc_cachedPIN = 0;

// NVS storage for PIN (survives power cycles)
static Preferences prefs;

namespace Utils
{

size_t getTotalHeap() { return ESP.getHeapSize(); }

size_t getFreeHeap() { return ESP.getFreeHeap(); }

size_t getLargestFreeBlock() { return ESP.getMaxAllocHeap(); }

void printMemoryStats()
{
  Serial.println("[Memory Stats]");
  Serial.printf("  Total Heap:  %zu bytes\n", getTotalHeap());
  Serial.printf("  Free Heap:   %zu bytes\n", getFreeHeap());
  Serial.printf("  Largest Block: %zu bytes\n", getLargestFreeBlock());
  Serial.printf("  Usage:       %.1f%%\n", 100.0 * (1.0 - (float)getFreeHeap() / (float)getTotalHeap()));
}

void initializeAPIKey()
{
  prefs.begin("zivyobraz");

  if (!prefs.isKey("apikey"))
  {
    // Generate 8-digit PIN without leading zeros (range: 10000000 - 99999999)
    rtc_cachedPIN = (esp_random() % 90000000) + 10000000;
    prefs.putULong("apikey", rtc_cachedPIN);
    Serial.print("Generated new device API key: ");
    Serial.println(rtc_cachedPIN);
  }
  else
  {
    rtc_cachedPIN = prefs.getULong("apikey");
    Serial.print("Device API key: ");
    Serial.println(rtc_cachedPIN);
  }

  prefs.end();
}

uint32_t getStoredAPIKey()
{
  if (rtc_cachedPIN == 0)
  {
    prefs.begin("zivyobraz", true); // read-only
    rtc_cachedPIN = prefs.getULong("apikey", 0);
    prefs.end();
  }
  return rtc_cachedPIN;
}

} // namespace Utils
