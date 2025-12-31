#ifndef PIXEL_PACKER_H
#define PIXEL_PACKER_H

#include <Arduino.h>
#include <cstdint>

#include "display.h"

namespace PixelPacker
{

// Display format types for streaming, values match CT_* defines from display.h
enum class DisplayFormat : uint8_t
{
  BW = CT_BW,               // 1bpp black/white
  GRAYSCALE = CT_GRAYSCALE, // 2bpp 4-level grayscale
  COLOR_3C = CT_3C,         // Dual 1bpp planes (black + red/yellow)
  COLOR_4C = CT_4C,         // 2bpp native
  COLOR_7C = CT_7C          // 4bpp 7-color
};

// Color values in RGB565 format
enum class GxEPDColor : uint16_t
{
  BLACK = 0x0000,    // R=0, G=0, B=0
  WHITE = 0xFFFF,    // R=31, G=63, B=31
  RED = 0xF800,      // R=31, G=0, B=0
  YELLOW = 0xFFE0,   // R=31, G=63, B=0
  GREEN = 0x07E0,    // R=0, G=63, B=0 (6E/7C displays only)
  BLUE = 0x001F,     // R=0, G=0, B=31 (6E/7C displays only)
  ORANGE = 0xFD20,   // R=31, G=41, B=0 (7C displays only)
  DARKGREY = 0x7BEF, // R=15, G=31, B=15 (Grayscale displays)
  LIGHTGREY = 0xC618 // R=24, G=48, B=24 (Grayscale displays)
};

// White byte patterns for buffer initialization
constexpr uint8_t WHITE_BYTE_1BPP = 0xFF;
constexpr uint8_t WHITE_BYTE_2BPP = 0xFF;
constexpr uint8_t WHITE_BYTE_4BPP = 0x11;
constexpr uint8_t WHITE_BYTE_4C = 0x55;

inline constexpr DisplayFormat getDisplayFormat() { return static_cast<DisplayFormat>(COLOR_ID); }

// Check if direct streaming is supported for the current display format
// 4C displays with hasPartialUpdate=true don't support paged streaming mode
inline constexpr bool supportsDirectStreaming()
{
#if defined(TYPE_4C)
  return false;
#else
  return true;
#endif
}

size_t getRowBufferSize(uint16_t width, DisplayFormat format);
uint8_t getBitsPerPixel(DisplayFormat format);

void packPixelBW(uint8_t *buffer, uint16_t x, bool isBlack);
void packPixel4G(uint8_t *buffer, uint16_t x, uint8_t grey);
void packPixel3C(uint8_t *blackBuffer, uint8_t *colorBuffer, uint16_t x, uint16_t color);
void packPixel4C(uint8_t *buffer, uint16_t x, uint8_t color4);
void packPixel7C(uint8_t *buffer, uint16_t x, uint8_t color7);

void convertGrayscaleToBW(const uint8_t *src2bpp, uint8_t *dst1bpp, uint16_t width, uint16_t rowCount);
uint8_t gxepdToGrey(uint16_t color);
uint8_t gxepdTo4CColor(uint16_t color);
uint8_t gxepdTo7CColor(uint16_t color);

void initRowBuffer(uint8_t *buffer, size_t size, DisplayFormat format);

} // namespace PixelPacker

#endif // PIXEL_PACKER_H
