#ifndef WIRELESS_H
#define WIRELESS_H

#include <Arduino.h>

namespace Wireless
{
void init(const String &hostname, const String &password, void (*callback)());
String getSSID();
int8_t getStrength();
String getMacAddress();

String getSoftAPSSID();
String getSoftAPIP();

bool isConnected();
void turnOff();
void resetCredentialsAndReboot();
} // namespace Wireless

#endif // WIRELESS_H
