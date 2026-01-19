#ifndef WIRELESS_H
#define WIRELESS_H

#include <Arduino.h>

namespace Wireless
{
void init(const String &hostname, const String &password, void (*callback)());
void process();
bool isConfigPortalActive();

String getSSID();
int8_t getStrength();
String getMacAddress();

String getSoftAPSSID();
String getSoftAPIP();
String getIPAddress();

bool isConnected();
void turnOff();
void resetCredentialsAndReboot();
} // namespace Wireless

#endif // WIRELESS_H
