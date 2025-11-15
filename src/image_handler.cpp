/*
 * Image Handler - Modular Image Processing
 *
 * This module handles decoding and rendering of various image formats
 * to the e-paper display. Formats are processed in separate functions
 * for maintainability and future extensibility.
 *
 * Supported Formats:
 * - BMP: Standard bitmap format (1, 4, 8, 24, 32-bit depths)
 * - Z1:  ZivyObraz RLE format (1 byte color + 1 byte count)
 * - Z2:  ZivyObraz RLE format (2-bit color + 6-bit count) - Most efficient
 * - Z3:  ZivyObraz RLE format (3-bit color + 5-bit count)
 *
 * Future Improvements:
 * - Add PNG support (libpng or similar)
 * - Optimize with display.writeLine() for row-based rendering
 * - Add progressive loading for large images
 * - Implement image caching/partial updates
 */

#include "image_handler.h"

#include "display.h"
#include "state_manager.h"

#ifdef TYPE_BW
  #include <GxEPD2_BW.h>
#elif defined TYPE_3C
  #include <GxEPD2_3C.h>
#elif defined TYPE_4C
  #include <GxEPD2_4C.h>
#elif defined TYPE_GRAYSCALE
  #include "GxEPD2_4G_4G.h"
  #include "GxEPD2_4G_BW.h"
#elif defined TYPE_7C
  #include <GxEPD2_7C.h>
#endif

namespace ImageHandler
{

///////////////////////////////////////////////
// Image Format Constants
///////////////////////////////////////////////

constexpr uint16_t FORMAT_BMP = 0x4D42; // "BM"
constexpr uint16_t FORMAT_Z1 = 0x315A;  // Z1: 1 byte color + 1 byte count
constexpr uint16_t FORMAT_Z2 = 0x325A;  // Z2: 2-bit color + 6-bit count
constexpr uint16_t FORMAT_Z3 = 0x335A;  // Z3: 3-bit color + 5-bit count

///////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////

static void printReadError(uint32_t bytesRead)
{
  Serial.print("Client got disconnected after bytes: ");
  Serial.println(bytesRead);
}

static uint16_t getSecondColor()
{
#if (defined TYPE_BW) || (defined TYPE_GRAYSCALE)
  return GxEPD_LIGHTGREY;
#else
  return GxEPD_RED;
#endif
}

static uint16_t getThirdColor()
{
#if (defined TYPE_BW) || (defined TYPE_GRAYSCALE)
  return GxEPD_DARKGREY;
#else
  return GxEPD_YELLOW;
#endif
}

static uint16_t mapColorValue(uint8_t pixelColor, uint16_t color2, uint16_t color3)
{
  switch (pixelColor)
  {
    case 0x0:
      return GxEPD_WHITE;
    case 0x1:
      return GxEPD_BLACK;
    case 0x2:
      return color2;
    case 0x3:
      return color3;
#ifdef TYPE_7C
    case 0x4:
      return GxEPD_GREEN;
    case 0x5:
      return GxEPD_BLUE;
    case 0x6:
      return GxEPD_ORANGE;
#endif
    default:
      return GxEPD_WHITE;
  }
}

///////////////////////////////////////////////
// BMP Image Processing
///////////////////////////////////////////////

static bool processBMP(HttpClient &http, uint32_t startTime)
{
  // Read BMP header
  uint32_t fileSize = http.read32();
  uint32_t creatorBytes = http.read32();
  (void)creatorBytes; // unused
  uint32_t imageOffset = http.read32();
  uint32_t headerSize = http.read32();
  uint32_t width = http.read32();
  int32_t height = (int32_t)http.read32();
  uint16_t planes = http.read16();
  uint16_t depth = http.read16();
  uint32_t format = http.read32();

  Serial.print("BMP format: ");
  Serial.print(width);
  Serial.print("x");
  Serial.print(abs(height));
  Serial.print(", depth=");
  Serial.println(depth);

  uint32_t bytes_read = 34; // BMP header: 2-byte signature + 32-byte info header (BITMAPINFOHEADER)

  // Check if format is supported
  if ((planes != 1) || ((format != 0) && (format != 3)))
  {
    Serial.println("Unsupported BMP format");
    return false;
  }

  bool flip = height < 0;
  if (flip)
    height = -height;

  uint16_t w = Display::getWidth();
  uint16_t h = Display::getHeight();

  // BMP rows are padded to 4-byte boundary
  uint32_t rowSize = (width * depth / 8 + 3) & ~3;
  if (depth < 8)
    rowSize = ((width * depth + 8 - depth) / 8 + 3) & ~3;

  if ((width > w) || (height > h))
  {
    Serial.println("BMP size exceeds display");
    return false;
  }

  // Static buffer for one row - conservative size for embedded systems
  // 800 bytes = 800px at 1-bit, 400px at 2-bit, 200px at 4-bit, or ~260px at 24-bit
  // This handles most common e-paper displays without excessive stack usage
  static const uint16_t MAX_ROW_BUFFER = 800;
  uint8_t rowBuffer[MAX_ROW_BUFFER];

  if (rowSize > MAX_ROW_BUFFER)
  {
    Serial.printf("ERROR: Row size %d exceeds buffer %d (reduce color depth or use smaller image)\n", rowSize,
                  MAX_ROW_BUFFER);
    return false;
  }

  // Skip to image data
  bytes_read += http.skip((int32_t)(imageOffset - bytes_read));

  // Process each row
  for (uint16_t row = 0; row < height; row++)
  {
    if (!http.isConnected())
    {
      Serial.printf("Connection lost at row %d/%d\n", row, height);
      return false;
    }

    // Read one row
    uint32_t bytesRead = http.readBytes(rowBuffer, rowSize);
    bytes_read += bytesRead;

    if (bytesRead != rowSize)
    {
      Serial.printf("Row %d incomplete: got %d/%d bytes\n", row, bytesRead, rowSize);
      return false;
    }

    // Determine display row (BMP is bottom-up unless negative height)
    uint16_t displayRow = flip ? row : (height - 1 - row);

    // Process pixels in this row
    uint16_t bufIdx = 0;
    for (uint16_t col = 0; col < width && col < w; col++)
    {
      uint16_t color = GxEPD_WHITE;

      switch (depth)
      {
        case 1: // 1-bit BW
        {
          uint8_t bit = 7 - (col % 8);
          if (col % 8 == 0 && col > 0)
            bufIdx++;
          color = (rowBuffer[bufIdx] & (1 << bit)) ? GxEPD_WHITE : GxEPD_BLACK;
          break;
        }

        case 4: // 4-bit indexed
        {
          uint8_t nibble = (col & 1) ? (rowBuffer[bufIdx] & 0x0F) : (rowBuffer[bufIdx] >> 4);
          if (col & 1)
            bufIdx++;
          // Simple grayscale mapping
          color = (nibble < 4)    ? GxEPD_BLACK
                  : (nibble < 8)  ? GxEPD_DARKGREY
                  : (nibble < 12) ? GxEPD_LIGHTGREY
                                  : GxEPD_WHITE;
          break;
        }

        case 8: // 8-bit grayscale
        {
          uint8_t gray = rowBuffer[bufIdx++];
          color = (gray < 64)    ? GxEPD_BLACK
                  : (gray < 128) ? GxEPD_DARKGREY
                  : (gray < 192) ? GxEPD_LIGHTGREY
                                 : GxEPD_WHITE;
          break;
        }

        case 24: // 24-bit RGB
        {
          uint8_t b = rowBuffer[bufIdx++];
          uint8_t g = rowBuffer[bufIdx++];
          uint8_t r = rowBuffer[bufIdx++];

          // Convert to grayscale
          uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
          color = (gray < 64)    ? GxEPD_BLACK
                  : (gray < 128) ? GxEPD_DARKGREY
                  : (gray < 192) ? GxEPD_LIGHTGREY
                                 : GxEPD_WHITE;
          break;
        }

        case 32: // 32-bit RGBA
        {
          uint8_t b = rowBuffer[bufIdx++];
          uint8_t g = rowBuffer[bufIdx++];
          uint8_t r = rowBuffer[bufIdx++];
          bufIdx++; // skip alpha

          uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
          color = (gray < 64)    ? GxEPD_BLACK
                  : (gray < 128) ? GxEPD_DARKGREY
                  : (gray < 192) ? GxEPD_LIGHTGREY
                                 : GxEPD_WHITE;
          break;
        }

        default:
          break;
      }

      Display::drawPixel(col, displayRow, color);
    }

    // Yield periodically
    if (row % 50 == 0)
    {
      yield();
    }
  }

  Serial.print("bytes read ");
  Serial.println(bytes_read);
  Serial.print("loaded in ");
  Serial.print(millis() - startTime);
  Serial.println(" ms");

  return true;
}

///////////////////////////////////////////////
// Unified RLE Format Processing
///////////////////////////////////////////////

static bool processRLE(HttpClient &http, uint32_t startTime, uint16_t format)
{
  const char *formatName = (format == FORMAT_Z1) ? "Z1" : (format == FORMAT_Z2) ? "Z2" : "Z3";
  Serial.print("Got format ");
  Serial.print(formatName);
  Serial.println(", processing");

  uint32_t bytes_read = 2; // Already read header
  uint16_t w = Display::getResolutionX();
  uint16_t h = Display::getResolutionY();
  uint32_t totalPixels = w * h;

  uint16_t color2 = getSecondColor();
  uint16_t color3 = getThirdColor();

  uint16_t row = 0;
  uint16_t col = 0;
  uint32_t pixelsProcessed = 0;

  while (pixelsProcessed < totalPixels && http.isConnected())
  {
    uint8_t pixelColor, count;

    // Read and decode based on format
    if (format == FORMAT_Z1)
    {
      // Z1: 1 byte color + 1 byte count
      pixelColor = http.readByte();
      count = http.readByte();
      bytes_read += 2;
    }
    else
    {
      // Z2 and Z3: compressed into single byte
      uint8_t compressed = http.readByte();
      bytes_read++;

      if (format == FORMAT_Z2)
      {
        // Z2: 2-bit color + 6-bit count
        count = compressed & 0b00111111;
        pixelColor = (compressed & 0b11000000) >> 6;
      }
      else // FORMAT_Z3
      {
        // Z3: 3-bit color + 5-bit count
        count = compressed & 0b00011111;
        pixelColor = (compressed & 0b11100000) >> 5;
      }
    }

    uint16_t color = mapColorValue(pixelColor, color2, color3);

    // Draw pixels
    for (uint8_t i = 0; i < count && pixelsProcessed < totalPixels; i++)
    {
      Display::drawPixel(col, row, color);
      pixelsProcessed++;

      if (++col >= w)
      {
        col = 0;
        row++;
      }
    }

    // Yield periodically
    if (bytes_read % 1000 == 0)
    {
      yield();
    }
  }

  Serial.print("bytes read ");
  Serial.println(bytes_read);
  Serial.print("loaded in ");
  Serial.print(millis() - startTime);
  Serial.println(" ms");

  return (pixelsProcessed == totalPixels);
}

// Wrapper functions for backward compatibility
static bool processZ1(HttpClient &http, uint32_t startTime)
{
  return processRLE(http, startTime, FORMAT_Z1);
}

static bool processZ2(HttpClient &http, uint32_t startTime)
{
  return processRLE(http, startTime, FORMAT_Z2);
}

static bool processZ3(HttpClient &http, uint32_t startTime)
{
  return processRLE(http, startTime, FORMAT_Z3);
}

///////////////////////////////////////////////
// Main Image Reader
///////////////////////////////////////////////

void readImageData(HttpClient &http)
{
  uint32_t startTime = millis();
  bool success = false;

  // Read format header (2 bytes)
  uint16_t header = http.read16();

  Serial.print("Header: 0x");
  Serial.println(header, HEX);

  // Route to appropriate format handler
  switch (header)
  {
    case FORMAT_BMP:
      success = processBMP(http, startTime);
      break;

    case FORMAT_Z1:
      success = processZ1(http, startTime);
      break;

    case FORMAT_Z2:
      success = processZ2(http, startTime);
      break;

    case FORMAT_Z3:
      success = processZ3(http, startTime);
      break;

    default:
      Serial.print("Unknown format header: 0x");
      Serial.println(header, HEX);
      success = false;
      break;
  }

  // Handle errors
  if (!success)
  {
    Serial.println("ERROR: Image processing failed");
    StateManager::setSleepDuration(StateManager::DEFAULT_SLEEP_SECONDS);
    StateManager::setTimestamp(0);
  }
}

} // namespace ImageHandler
