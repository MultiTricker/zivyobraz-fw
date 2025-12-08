/*
 * Image Handler - Modular Image Processing
 *
 * This module handles decoding and rendering of various image formats
 * to the e-paper display. Formats are processed in separate functions
 * for maintainability and future extensibility.
 *
 * Supported Formats:
 * - PNG: Portable Network Graphics (using pngle - lightweight embedded decoder)
 * - Z1:  ZivyObraz RLE format (1 byte color + 1 byte count)
 * - Z2:  ZivyObraz RLE format (2-bit color + 6-bit count) - Most efficient
 * - Z3:  ZivyObraz RLE format (3-bit color + 5-bit count)
 */

#include "image_handler.h"

#include "display.h"
#include "state_manager.h"
#include "streaming_handler.h"

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

#include <pngle.h>

namespace ImageHandler
{

///////////////////////////////////////////////
// Image Format Types
///////////////////////////////////////////////
enum class ImageFormat : uint16_t
{
  PNG = 0x5089, // PNG signature (first 2 bytes: 0x89 0x50)
  Z1 = 0x315A,  // Z1: 1 byte color + 1 byte count
  Z2 = 0x325A,  // Z2: 2-bit color + 6-bit count
  Z3 = 0x335A   // Z3: 3-bit color + 5-bit count
};

///////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////

static const char *formatToString(ImageFormat format)
{
  switch (format)
  {
    case ImageFormat::PNG:
      return "PNG";
    case ImageFormat::Z1:
      return "Z1";
    case ImageFormat::Z2:
      return "Z2";
    case ImageFormat::Z3:
      return "Z3";
    default:
      return "Unknown";
  }
}

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
// PNG Image Processing
///////////////////////////////////////////////

// Callback: Draw pixel from PNG decoder
static void pngleOnDraw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t rgba[4])
{
  if ((x >= Display::getWidth()) || (y >= Display::getHeight()))
  {
    Serial.println("PNG size exceeds display");
    return;
  }

  // Convert RGBA to grayscale
  uint8_t r = rgba[0];
  uint8_t g = rgba[1];
  uint8_t b = rgba[2];
  uint8_t a = rgba[3];

  // Skip fully transparent pixels
  if (a == 0)
  {
    Serial.println("Skipping transparent pixel");
    return;
  }

  uint16_t color;

#if defined(TYPE_BW)
  // BW displays: Black & White only
  uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
  color = (gray < 128) ? GxEPD_BLACK : GxEPD_WHITE;

#elif defined(TYPE_3C)
  // 3-color displays: Black, White, Red
  // Detect red hue (high red, low green/blue)
  if (r > 128 && r > (g + 80) && r > (b + 80))
  {
    color = GxEPD_RED;
  }
  else
  {
    uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
    color = (gray < 128) ? GxEPD_BLACK : GxEPD_WHITE;
  }

#elif defined(TYPE_4C)
  // 4-color displays: Black, White, Red, Yellow
  // Detect yellow (high red & green, low blue)
  if (r > 128 && g > 128 && b < 80)
  {
    color = GxEPD_YELLOW;
  }
  // Detect red hue
  else if (r > 128 && r > (g + 80) && r > (b + 80))
  {
    color = GxEPD_RED;
  }
  else
  {
    uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
    color = (gray < 128) ? GxEPD_BLACK : GxEPD_WHITE;
  }

#elif defined(TYPE_7C)
  // 7-color displays: Black, White, Red, Yellow, Green, Blue, Orange
  // Detect orange (red-yellow mix)
  if (r > 200 && g > 80 && g < 180 && b < 80)
  {
    color = GxEPD_ORANGE;
  }
  // Detect red
  else if (r > 128 && r > (g + 80) && r > (b + 80))
  {
    color = GxEPD_RED;
  }
  // Detect yellow
  else if (r > 128 && g > 128 && b < 80)
  {
    color = GxEPD_YELLOW;
  }
  // Detect green
  else if (g > 128 && g > (r + 80) && g > (b + 80))
  {
    color = GxEPD_GREEN;
  }
  // Detect blue
  else if (b > 128 && b > (r + 80) && b > (g + 80))
  {
    color = GxEPD_BLUE;
  }
  else
  {
    uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
    color = (gray < 128) ? GxEPD_BLACK : GxEPD_WHITE;
  }

#elif defined(TYPE_GRAYSCALE)
  // Grayscale displays: 4-level grayscale
  uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
  color = (gray < 64) ? GxEPD_BLACK : (gray < 128) ? GxEPD_DARKGREY : (gray < 192) ? GxEPD_LIGHTGREY : GxEPD_WHITE;

#else
  // Fallback: grayscale
  uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
  color = (gray < 128) ? GxEPD_BLACK : GxEPD_WHITE;
#endif

  Display::drawPixel(x, y, color);

  // Yield periodically to prevent watchdog timeout
  static uint32_t pixelCount = 0;
  if (++pixelCount % 1000 == 0)
    yield();
}

static bool processPNG(HttpClient &http, uint32_t startTime, uint8_t *buffer, uint16_t bufferSize)
{
  Serial.println("Got PNG format, processing");

  // Create pngle decoder
  pngle_t *pngle = pngle_new();
  if (!pngle)
  {
    Serial.println("Failed to create PNG decoder");
    return false;
  }

  // Set draw callback
  pngle_set_draw_callback(pngle, pngleOnDraw);

  // Reconstruct PNG signature: we already read first 2 bytes (0x89 0x50) for format detection
  // PNG signature is 8 bytes: 0x89 0x50 0x4E 0x47 0x0D 0x0A 0x1A 0x0A
  uint8_t pngSignature[8];
  pngSignature[0] = 0x89;
  pngSignature[1] = 0x50;

  // Read remaining 6 bytes of signature
  uint32_t sigBytesRead = http.readBytes(&pngSignature[2], 6);
  if (sigBytesRead != 6)
  {
    printReadError(2 + sigBytesRead);
    pngle_destroy(pngle);
    return false;
  }

  // Feed complete signature
  int fed = pngle_feed(pngle, pngSignature, 8);
  if (fed < 0)
  {
    Serial.print("PNG signature error: ");
    Serial.println(pngle_error(pngle));
    pngle_destroy(pngle);
    return false;
  }

  // Stream PNG data in chunks using passed buffer
  uint32_t bytes_read = 8;
  bool success = true;

  while (http.isConnected() || http.available())
  {
    uint32_t chunkSize = http.readBytes(buffer, bufferSize);
    if (chunkSize == 0)
      break;

    bytes_read += chunkSize;

    // Feed data to decoder
    fed = pngle_feed(pngle, buffer, chunkSize);
    if (fed < 0)
    {
      Serial.print("PNG decode error: ");
      Serial.println(pngle_error(pngle));
      success = false;
      break;
    }

    // Yield periodically
    yield();
  }

  pngle_destroy(pngle);

  if (success)
  {
    Serial.print("bytes read ");
    Serial.println(bytes_read);
    Serial.print("loaded in ");
    Serial.print(millis() - startTime);
    Serial.println(" ms");
  }

  return success;
}

///////////////////////////////////////////////
// Unified RLE Format Processing
///////////////////////////////////////////////

static bool processRLE(HttpClient &http, uint32_t startTime, ImageFormat format, uint8_t *buffer, uint16_t bufferSize)
{
  Serial.print("Got format ");
  Serial.print(formatToString(format));
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

  // Use passed buffer for efficient reading
  uint32_t bufferPos = 0;
  uint32_t bufferAvailable = 0;
  bool bufferEmpty = true;

  while (pixelsProcessed < totalPixels)
  {
    // Refill buffer if needed
    if (bufferEmpty || bufferPos >= bufferAvailable)
    {
      if (!http.isConnected() && !http.available())
      {
        Serial.print("Incomplete image received. Pixels processed: ");

        // If we're close to complete (95%+), consider it a success
        if (pixelsProcessed >= (totalPixels * 95 / 100))
        {
          Serial.println("Image is 95%+ complete, accepting as valid");
          return true;
        }
        return false;
      }

      uint32_t bytesRead = http.readBytes(buffer, bufferSize);
      if (bytesRead == 0)
      {
        Serial.print("No more data available. Pixels processed: ");
        Serial.print(pixelsProcessed);
        Serial.print("/");
        Serial.println(totalPixels);
        break;
      }

      bufferPos = 0;
      bufferEmpty = false;
      bytes_read += bytesRead;
      bufferAvailable = bytesRead;
    }

    uint8_t pixelColor, count;

    // Read and decode based on format
    if (format == ImageFormat::Z1)
    {
      // Z1: 1 byte color + 1 byte count
      if (bufferPos + 1 >= bufferAvailable)
      {
        // Need more data for Z1 (2 bytes)
        bufferEmpty = true;
        continue;
      }
      pixelColor = buffer[bufferPos++];
      count = buffer[bufferPos++];
    }
    else
    {
      // Z2 and Z3: compressed into single byte
      uint8_t compressed = buffer[bufferPos++];

      if (format == ImageFormat::Z2)
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
    if (pixelsProcessed % 10000 == 0)
      yield();
  }

  Serial.print("bytes read ");
  Serial.println(bytes_read);
  Serial.print("loaded in ");
  Serial.print(millis() - startTime);
  Serial.println(" ms");

  return (pixelsProcessed == totalPixels);
}

///////////////////////////////////////////////
// Main Image Reader
///////////////////////////////////////////////

void readImageData(HttpClient &http)
{
  uint32_t startTime = millis();
  bool success = false;

#ifdef STREAMING_ENABLED
  // Initialize streaming if enabled
  StreamingHandler::StreamingManager &streamMgr = StreamingHandler::StreamingManager::getInstance();
  if (!streamMgr.isEnabled())
  {
    // Calculate row size: width * bytes_per_pixel (for most formats, 1 byte per pixel)
    // For safety, use display width as row size estimate
    size_t rowSize = Display::getWidth();

    if (streamMgr.init(rowSize))
      Serial.println("[Image] Streaming enabled");
    else
      Serial.println("[Image] Streaming init failed, falling back to direct mode");
  }

  // Print memory stats
  if (streamMgr.isEnabled())
  {
    size_t totalHeap, freeHeap, bufferUsed;
    streamMgr.getMemoryStats(totalHeap, freeHeap, bufferUsed);
    Serial.printf("[Image] Memory - Total: %zu, Free: %zu, Buffer: %zu\n", totalHeap, freeHeap, bufferUsed);
  }
#endif

  // Read format header (2 bytes)
  uint16_t header = http.read16();

  Serial.print("Header: 0x");
  Serial.println(header, HEX);

  // Dynamic buffer for PNG/RLE processing
  // BMP handles its own buffer allocation
  const uint16_t STREAM_BUFFER_SIZE = 512;
  uint8_t *buffer = new (std::nothrow) uint8_t[STREAM_BUFFER_SIZE];

  if (!buffer)
  {
    Serial.println("[Image] Failed to allocate processing buffer");
    return;
  }

  // Route to appropriate format handler
  switch (static_cast<ImageFormat>(header))
  {
    case ImageFormat::PNG:
      success = processPNG(http, startTime, buffer, STREAM_BUFFER_SIZE);
      break;

    case ImageFormat::Z1:
      success = processRLE(http, startTime, ImageFormat::Z1, buffer, STREAM_BUFFER_SIZE);
      break;

    case ImageFormat::Z2:
      success = processRLE(http, startTime, ImageFormat::Z2, buffer, STREAM_BUFFER_SIZE);
      break;

    case ImageFormat::Z3:
      success = processRLE(http, startTime, ImageFormat::Z3, buffer, STREAM_BUFFER_SIZE);
      break;

    default:
      Serial.print("Unknown format header: 0x");
      Serial.println(header, HEX);
      success = false;
      break;
  }

  // Cleanup processing buffer
  delete[] buffer;
  buffer = nullptr;

  // Handle errors
  if (!success)
  {
    Serial.println("ERROR: Image processing failed");
    StateManager::setSleepDuration(StateManager::DEFAULT_SLEEP_SECONDS);
    StateManager::setTimestamp(0);
  }

#ifdef STREAMING_ENABLED
  // Cleanup streaming resources after processing
  if (streamMgr.isEnabled())
    streamMgr.cleanup();
#endif
}

} // namespace ImageHandler
