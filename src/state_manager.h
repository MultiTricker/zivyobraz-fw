#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>

namespace StateManager
{

enum class ResetReason : uint8_t
{
  UNKNOWN = 0,
  POWERON = 1,   // Power-on reset or flash upload
  EXT = 2,       // External reset (reset button)
  SW = 3,        // Software reset
  PANIC = 4,     // Software panic/exception
  INT_WDT = 5,   // Interrupt watchdog
  TASK_WDT = 6,  // Task watchdog
  WDT = 7,       // Other watchdog
  DEEPSLEEP = 8, // Wake from deep sleep
  BROWNOUT = 9,  // Brownout reset
  SDIO = 10      // Reset over SDIO
};

// Reset reason
ResetReason getResetReason();

// Timestamp management
uint64_t getTimestamp();
void setTimestamp(uint64_t ts);

// WiFi failure tracking
uint8_t getFailureCount();
void incrementFailureCount();
void resetFailureCount();

// Sleep duration management
uint64_t getSleepDuration();
void setSleepDuration(uint64_t seconds);
uint64_t calculateSleepDuration();

// Program runtime compensation
unsigned long getProgramRuntimeCompensationStart();
void setProgramRuntimeCompensationStart(unsigned long compensation);

// Last runtime compensation time (in RTC memory to send as part of a report to server next run)
long getLastCompensationTime();
void setLastCompensationTime(long time);

// Default sleep time
static const uint64_t DEFAULT_SLEEP_SECONDS = 120;
} // namespace StateManager

#endif // STATE_MANAGER_H
