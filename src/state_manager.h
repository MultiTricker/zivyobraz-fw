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

// ShowNoWifiError setting
uint8_t getShowNoWifiError();
void setShowNoWifiError(uint8_t value);

// Download/display refresh duration tracking
void startDownloadTimer();
void endDownloadTimer();
void startRefreshTimer();
void endRefreshTimer();
unsigned long getTotalCompensation();
unsigned long getLastDownloadDuration();
unsigned long getLastRefreshDuration();
void setLastRefreshDuration(unsigned long duration);

// Default sleep time
static const uint64_t DEFAULT_SLEEP_SECONDS = 120;
} // namespace StateManager

#endif // STATE_MANAGER_H
