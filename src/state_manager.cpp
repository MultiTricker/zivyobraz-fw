#include "state_manager.h"

// RTC persistent data (survives deep sleep)
RTC_DATA_ATTR uint64_t rtc_timestamp = 0;
RTC_DATA_ATTR uint8_t rtc_failureCount = 0;
RTC_DATA_ATTR long rtc_lastCompensationTime = 0;

namespace StateManager
{

uint64_t sleepDuration = DEFAULT_SLEEP_SECONDS;
unsigned long programRuntimeCompensation = 0; // in milliseconds

uint64_t getTimestamp() { return rtc_timestamp; }

void setTimestamp(uint64_t ts) { rtc_timestamp = ts; }

unsigned long getProgramRuntimeCompensationStart() { return programRuntimeCompensation; }

void setProgramRuntimeCompensationStart(unsigned long compensation)
{
  // Only set once if not already set
  if (programRuntimeCompensation == 0)
    programRuntimeCompensation = compensation;
}

long getLastCompensationTime() { return rtc_lastCompensationTime; }

void setLastCompensationTime(long time) { rtc_lastCompensationTime = time; }

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
} // namespace StateManager
