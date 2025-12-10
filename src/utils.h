#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

namespace Utils
{

// API key management (stored in NVS, survives power cycles)
void initializeAPIKey();
uint32_t getStoredAPIKey();

} // namespace Utils

#endif // UTILS_H
