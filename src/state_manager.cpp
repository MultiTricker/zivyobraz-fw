#include "state_manager.h"

// RTC persistent data (survives deep sleep)
RTC_DATA_ATTR uint64_t rtc_timestamp = 0;
RTC_DATA_ATTR uint8_t rtc_failureCount = 0;

namespace StateManager
{

uint64_t sleepDuration = DEFAULT_SLEEP_SECONDS;

uint64_t getTimestamp()
{
  return rtc_timestamp;
}

void setTimestamp(uint64_t ts)
{
  rtc_timestamp = ts;
}

uint8_t getFailureCount()
{
  return rtc_failureCount;
}

void incrementFailureCount()
{
  if (rtc_failureCount < 255)
  {
    rtc_failureCount++;
  }
}

void resetFailureCount()
{
  rtc_failureCount = 0;
}

uint64_t getSleepDuration()
{
  return sleepDuration;
}

void setSleepDuration(uint64_t seconds)
{
  sleepDuration = seconds;
}

uint64_t calculateSleepDuration()
{
  // Calculate sleep based on failure count
  if (rtc_failureCount <= 3)
  {
    sleepDuration = DEFAULT_SLEEP_SECONDS; // 2 minutes
  }
  else if (rtc_failureCount <= 10)
  {
    sleepDuration = 600; // 10 minutes
  }
  else if (rtc_failureCount <= 20)
  {
    sleepDuration = 1800; // 30 minutes
  }
  else if (rtc_failureCount <= 50)
  {
    sleepDuration = 3600; // 1 hour
  }
  else
  {
    sleepDuration = 43200; // 12 hours
  }
  return sleepDuration;
}
} // namespace StateManager
