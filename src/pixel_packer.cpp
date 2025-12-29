#include "pixel_packer.h"

namespace PixelPacker
{

size_t getRowBufferSize(uint16_t width, DisplayFormat format)
{
  switch (format)
  {
    case DisplayFormat::BW:
      return (width + 7) / 8; // 1bpp: 8 pixels per byte
    case DisplayFormat::GRAYSCALE:
      return (width + 3) / 4; // 2bpp: 4 pixels per byte
    case DisplayFormat::COLOR_3C:
      return (width + 7) / 8; // 1bpp per plane (need 2 buffers)
    case DisplayFormat::COLOR_4C:
      return (width + 3) / 4; // 2bpp: 4 pixels per byte
    case DisplayFormat::COLOR_7C:
      return (width + 1) / 2; // 4bpp: 2 pixels per byte
    default:
      return (width + 7) / 8;
  }
}

uint8_t getBitsPerPixel(DisplayFormat format)
{
  switch (format)
  {
    case DisplayFormat::BW:
      return 1;
    case DisplayFormat::GRAYSCALE:
      return 2;
    case DisplayFormat::COLOR_3C:
      return 1; // Per plane
    case DisplayFormat::COLOR_4C:
      return 2;
    case DisplayFormat::COLOR_7C:
      return 4;
    default:
      return 1;
  }
}

void packPixelBW(uint8_t *buffer, uint16_t x, bool isBlack)
{
  uint16_t byteIndex = x / 8;
  uint8_t bitPosition = 7 - (x % 8); // MSB first

  if (isBlack)
  {
    buffer[byteIndex] &= ~(1 << bitPosition); // Clear bit for black
  }
  else
  {
    buffer[byteIndex] |= (1 << bitPosition); // Set bit for white
  }
}

void packPixel4G(uint8_t *buffer, uint16_t x, uint8_t grey)
{
  uint16_t byteIndex = x / 4;
  uint8_t pixelInByte = x % 4;

  // Convert 8-bit grey to 2-bit value (0-3)
  uint8_t value = grey >> 6; // Top 2 bits

  // Clear existing bits and set new value
  // Pixel order: pixel 0 in bits 7-6, pixel 1 in bits 5-4, etc.
  uint8_t shift = (3 - pixelInByte) * 2;
  buffer[byteIndex] = (buffer[byteIndex] & ~(0x03 << shift)) | (value << shift);
}

void packPixel3C(uint8_t *blackBuffer, uint8_t *colorBuffer, uint16_t x, uint16_t color)
{
  uint16_t byteIndex = x / 8;
  uint8_t bitPosition = 7 - (x % 8); // MSB first

  // 3C encoding:
  // WHITE:  black=1, color=1 (both bits set)
  // BLACK:  black=0, color=1 (black bit cleared)
  // RED/YELLOW: black=1, color=0 (color bit cleared)

  switch (color)
  {
    case static_cast<uint16_t>(GxEPDColor::BLACK):
      blackBuffer[byteIndex] &= ~(1 << bitPosition); // Clear black bit
      colorBuffer[byteIndex] |= (1 << bitPosition);  // Set color bit
      break;
    case static_cast<uint16_t>(GxEPDColor::RED):
    case static_cast<uint16_t>(GxEPDColor::YELLOW):
      blackBuffer[byteIndex] |= (1 << bitPosition);  // Set black bit
      colorBuffer[byteIndex] &= ~(1 << bitPosition); // Clear color bit
      break;
    default:                                        // WHITE and others
      blackBuffer[byteIndex] |= (1 << bitPosition); // Set black bit
      colorBuffer[byteIndex] |= (1 << bitPosition); // Set color bit
      break;
  }
}

void packPixel4C(uint8_t *buffer, uint16_t x, uint8_t color4)
{
  uint16_t byteIndex = x / 4;
  uint8_t pixelInByte = x % 4;

  // Pixel order: pixel 0 in bits 7-6, pixel 1 in bits 5-4, etc. (MSB first)
  uint8_t shift = (3 - pixelInByte) * 2;
  buffer[byteIndex] = (buffer[byteIndex] & ~(0x03 << shift)) | ((color4 & 0x03) << shift);
}

void packPixel7C(uint8_t *buffer, uint16_t x, uint8_t color7)
{
  uint16_t byteIndex = x / 2;

  if (x & 1)
    buffer[byteIndex] = (buffer[byteIndex] & 0xF0) | (color7 & 0x0F); // Odd pixel: low nibble
  else
    buffer[byteIndex] = (buffer[byteIndex] & 0x0F) | ((color7 & 0x0F) << 4); // Even pixel: high nibble
}


void convertGrayscaleToBW(const uint8_t *src2bpp, uint8_t *dst1bpp, uint16_t width, uint16_t rowCount)
{
  uint16_t srcBytesPerRow = (width + 3) / 4; // 2bpp = 4 pixels per byte
  uint16_t dstBytesPerRow = (width + 7) / 8; // 1bpp = 8 pixels per byte

  for (uint16_t row = 0; row < rowCount; row++)
  {
    const uint8_t *srcRow = src2bpp + row * srcBytesPerRow;
    uint8_t *dstRow = dst1bpp + row * dstBytesPerRow;

    for (uint16_t dstByte = 0; dstByte < dstBytesPerRow; dstByte++)
    {
      uint8_t outByte = 0;

      for (uint8_t bit = 0; bit < 8; bit++)
      {
        uint16_t pixelIndex = dstByte * 8 + bit;
        if (pixelIndex >= width)
          break;

        // Extract 2-bit grayscale value (0=black, 1=dark gray, 2=light gray, 3=white)
        uint16_t srcByteIndex = pixelIndex / 4;
        uint8_t srcBitOffset = (3 - (pixelIndex % 4)) * 2; // MSB first
        uint8_t grayValue = (srcRow[srcByteIndex] >> srcBitOffset) & 0x03;

        // Threshold: >= 2 becomes white (1), < 2 becomes black (0)
        // In e-paper: 1 = white, 0 = black
        if (grayValue >= 2)
        {
          outByte |= (0x80 >> bit); // Set bit for white
        }
      }
      dstRow[dstByte] = outByte;
    }
  }
}

uint8_t gxepdTo4CColor(uint16_t color)
{
  switch (color)
  {
    case static_cast<uint16_t>(GxEPDColor::BLACK):
      return 0;
    case static_cast<uint16_t>(GxEPDColor::YELLOW):
      return 2;
    case static_cast<uint16_t>(GxEPDColor::RED):
      return 3;
    case static_cast<uint16_t>(GxEPDColor::WHITE):
    default:
      return 1;
  }
}

uint8_t gxepdTo7CColor(uint16_t color)
{
  switch (color)
  {
    case static_cast<uint16_t>(GxEPDColor::BLACK):
      return 0;
    case static_cast<uint16_t>(GxEPDColor::WHITE):
      return 1;
    case static_cast<uint16_t>(GxEPDColor::GREEN):
      return 2;
    case static_cast<uint16_t>(GxEPDColor::BLUE):
      return 3;
    case static_cast<uint16_t>(GxEPDColor::RED):
      return 4;
    case static_cast<uint16_t>(GxEPDColor::YELLOW):
      return 5;
    case static_cast<uint16_t>(GxEPDColor::ORANGE):
      return 6;
    default:
      return 1; // Default to white
  }
}

uint8_t gxepdToGrey(uint16_t color)
{
  switch (color)
  {
    case static_cast<uint16_t>(GxEPDColor::BLACK):
      return 0x00; // Black (2-bit: 00)
    case static_cast<uint16_t>(GxEPDColor::DARKGREY):
      return 0x40; // Dark grey (2-bit: 01)
    case static_cast<uint16_t>(GxEPDColor::LIGHTGREY):
      return 0x80; // Light grey (2-bit: 10)
    case static_cast<uint16_t>(GxEPDColor::WHITE):
      return 0xC0; // White (2-bit: 11)
    default:
      return 0xC0; // Default to white
  }
}

void initRowBuffer(uint8_t *buffer, size_t size, DisplayFormat format)
{
  switch (format)
  {
    case DisplayFormat::BW:
    case DisplayFormat::COLOR_3C:
      memset(buffer, WHITE_BYTE_1BPP, size);
      break;
    case DisplayFormat::GRAYSCALE:
      memset(buffer, WHITE_BYTE_2BPP, size);
      break;
    case DisplayFormat::COLOR_7C:
      memset(buffer, WHITE_BYTE_4BPP, size);
      break;
    case DisplayFormat::COLOR_4C:
      memset(buffer, WHITE_BYTE_4C, size);
      break;
    default:
      memset(buffer, WHITE_BYTE_1BPP, size);
      break;
  }
}

} // namespace PixelPacker
