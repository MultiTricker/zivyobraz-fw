#include "state_manager.h"

#include "Preferences.h"
#include "esp_random.h"

// RTC persistent data (survives deep sleep)
RTC_DATA_ATTR uint64_t rtc_timestamp = 0;
RTC_DATA_ATTR uint8_t rtc_failureCount = 0;
RTC_DATA_ATTR uint32_t rtc_cachedPIN = 0;

// NVS storage for PIN (survives power cycles)
static Preferences prefs;

namespace StateManager
{

uint64_t sleepDuration = DEFAULT_SLEEP_SECONDS;

uint64_t getTimestamp() { return rtc_timestamp; }

void setTimestamp(uint64_t ts) { rtc_timestamp = ts; }

uint8_t getFailureCount() { return rtc_failureCount; }

void incrementFailureCount()
{
  if (rtc_failureCount < 255)
    rtc_failureCount++;
}

void resetFailureCount() { rtc_failureCount = 0; }

uint64_t getSleepDuration() { return sleepDuration; }

void setSleepDuration(uint64_t seconds) { sleepDuration = seconds; }

uint64_t calculateSleepDuration()
{
  // Calculate sleep based on failure count
  if (rtc_failureCount <= 3)
    sleepDuration = DEFAULT_SLEEP_SECONDS; // 2 minutes
  else if (rtc_failureCount <= 10)
    sleepDuration = 600; // 10 minutes
  else if (rtc_failureCount <= 20)
    sleepDuration = 1800; // 30 minutes
  else if (rtc_failureCount <= 50)
    sleepDuration = 3600; // 1 hour
  else
    sleepDuration = 43200; // 12 hours
  return sleepDuration;
}

ResetReason getResetReason()
{
  esp_reset_reason_t reason = esp_reset_reason();

  switch (reason)
  {
    case ESP_RST_POWERON:
      return ResetReason::POWERON;
    case ESP_RST_EXT:
      return ResetReason::EXT;
    case ESP_RST_SW:
      return ResetReason::SW;
    case ESP_RST_PANIC:
      return ResetReason::PANIC;
    case ESP_RST_INT_WDT:
      return ResetReason::INT_WDT;
    case ESP_RST_TASK_WDT:
      return ResetReason::TASK_WDT;
    case ESP_RST_WDT:
      return ResetReason::WDT;
    case ESP_RST_DEEPSLEEP:
      return ResetReason::DEEPSLEEP;
    case ESP_RST_BROWNOUT:
      return ResetReason::BROWNOUT;
    case ESP_RST_SDIO:
      return ResetReason::SDIO;
    default:
      return ResetReason::UNKNOWN;
  }
}

void initPIN()
{
  prefs.begin("zivyobraz", false);

  if (!prefs.isKey("pin"))
  {
    // Generate 8-digit PIN without leading zeros (range: 10000000 - 99999999)
    rtc_cachedPIN = (esp_random() % 90000000) + 10000000;
    prefs.putULong("pin", rtc_cachedPIN);
    Serial.print("Generated new device PIN: ");
    Serial.println(rtc_cachedPIN);
  }
  else
  {
    rtc_cachedPIN = prefs.getULong("pin", 0);
    Serial.print("Device PIN: ");
    Serial.println(rtc_cachedPIN);
  }

  prefs.end();
}

uint32_t getStoredPIN()
{
  if (rtc_cachedPIN == 0)
  {
    prefs.begin("zivyobraz", true); // read-only
    rtc_cachedPIN = prefs.getULong("pin", 0);
    prefs.end();
  }
  return rtc_cachedPIN;
}
} // namespace StateManager
