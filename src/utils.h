#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

// Helper macros (used in board.h and display.h for board/display type strings)
#define STR(x) #x
#define XSTR(x) STR(x)

#define CAT(a, b) a##b
#define XCAT(a, b) CAT(a, b)

namespace Utils
{
// Memory statistics
size_t getTotalHeap();
size_t getFreeHeap();
size_t getLargestFreeBlock();
void printMemoryStats();

// API key management (stored in NVS, survives power cycles)
void initializeAPIKey();
uint32_t getStoredAPIKey();
} // namespace Utils

#endif // UTILS_H
