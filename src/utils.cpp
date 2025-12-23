#include "utils.h"

#include "logger.h"

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
  Logger::log<Logger::Topic::SYSTEM>("  Total Heap:  {} bytes\n"
                                     "  Free Heap:   {} bytes\n"
                                     "  Largest Block: {} bytes\n"
                                     "  Usage:       {:.1f}%\n",
                                     getTotalHeap(), getFreeHeap(), getLargestFreeBlock(),
                                     100.0 * (1.0 - (float)getFreeHeap() / (float)getTotalHeap()));
}

void initializeAPIKey()
{
  prefs.begin("zivyobraz");

  if (!prefs.isKey("apikey"))
  {
    // Generate 8-digit PIN without leading zeros (range: 10000000 - 99999999)
    rtc_cachedPIN = (esp_random() % 90000000) + 10000000;
    prefs.putULong("apikey", rtc_cachedPIN);
    Logger::log<Logger::Topic::APIKEY>("Generated new device API key: {}\n", rtc_cachedPIN);
  }
  else
  {
    rtc_cachedPIN = prefs.getULong("apikey");
    Logger::log<Logger::Topic::APIKEY>("Loaded stored device API key: {}\n", rtc_cachedPIN);
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
