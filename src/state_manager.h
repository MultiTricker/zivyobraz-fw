#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>

namespace StateManager
{

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

// Default sleep time
static const uint64_t DEFAULT_SLEEP_SECONDS = 120;
} // namespace StateManager

#endif // STATE_MANAGER_H
