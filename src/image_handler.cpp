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
 *
 * Modes:
 * - Paged mode: Traditional page-by-page drawing (backward compatible)
 * - Direct streaming mode: Decode and write rows directly to display controller
 */

#include "image_handler.h"

#include "display.h"
#include "pixel_packer.h"
#include "logger.h"
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
  Logger::log<Logger::Level::ERROR, Logger::Topic::HTTP>("Client got disconnected after bytes: {}\n", bytesRead);
}

// Maximum bytes to scan for image header (4 KB)
static const uint16_t MAX_HEADER_SCAN_BYTES = 4096;

// Check if a 16-bit value is a valid image format header
static bool isValidFormatHeader(uint16_t header)
{
  switch (static_cast<ImageFormat>(header))
  {
    case ImageFormat::PNG:
    case ImageFormat::Z1:
    case ImageFormat::Z2:
    case ImageFormat::Z3:
      return true;
    default:
      return false;
  }
}

// Scan through first KB of data to find a valid image header
// This handles cases where PHP error messages precede the actual image data
// Returns true if valid header found, header value stored in outHeader
static bool scanForImageHeader(HttpClient &http, uint16_t &outHeader)
{
  // Read first byte
  uint8_t firstByte = http.readByte();
  uint8_t secondByte = http.readByte();

  // Build initial header (little-endian: second byte << 8 | first byte)
  uint16_t header = (secondByte << 8) | firstByte;

  // Check if first 2 bytes are already a valid header
  if (isValidFormatHeader(header))
  {
    outHeader = header;
    Logger::log<Logger::Level::DEBUG, Logger::Topic::IMAGE>("Image header found at offset 0\n");
    return true;
  }

  // Not a valid header at start - scan through first KB
  // Print scanned bytes to Serial to help debug server response issues
  Logger::log<Logger::Level::WARNING, Logger::Topic::IMAGE>("Scanning for image header, dumping response:\n");
  Logger::log<Logger::Level::WARNING, Logger::Topic::IMAGE>(">>> ");

  // Buffer for batching output (flush every 64 chars)
  constexpr uint8_t PRINT_BUFFER_SIZE = 64;
  char printBuffer[PRINT_BUFFER_SIZE + 1]; // +1 for null terminator
  uint8_t bufferPos = 0;

  // Helper lambda to add char to buffer and flush when full
  auto bufferChar = [&](char c)
  {
    printBuffer[bufferPos++] = c;
    if (bufferPos >= PRINT_BUFFER_SIZE)
    {
      printBuffer[bufferPos] = '\0';
      Logger::log<Logger::Level::WARNING, Logger::Topic::IMAGE>("{}", printBuffer);
      bufferPos = 0;
    }
  };

  // Helper lambda to flush remaining buffer
  auto flushBuffer = [&]()
  {
    if (bufferPos > 0)
    {
      printBuffer[bufferPos] = '\0';
      Logger::log<Logger::Level::WARNING, Logger::Topic::IMAGE>("{}", printBuffer);
      bufferPos = 0;
    }
  };

  // Print the first two bytes we already read (as characters if printable)
  bufferChar((firstByte >= 32 && firstByte < 127) ? (char)firstByte : '.');
  bufferChar((secondByte >= 32 && secondByte < 127) ? (char)secondByte : '.');

  // We've already read 2 bytes, scan up to MAX_HEADER_SCAN_BYTES - 2 more
  for (uint16_t offset = 2; offset < MAX_HEADER_SCAN_BYTES; offset++)
  {
    if (!http.isConnected() || http.available() <= 0)
    {
      flushBuffer();
      Logger::log<Logger::Level::WARNING, Logger::Topic::IMAGE>(" <<<\n");
      Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("Connection lost while scanning for header\n");
      return false;
    }

    // Shift bytes: previous secondByte becomes firstByte, read new secondByte
    firstByte = secondByte;
    secondByte = http.readByte();
    header = (secondByte << 8) | firstByte;

    // Buffer scanned byte (printable ASCII or dot for non-printable)
    if (secondByte >= 32 && secondByte < 127)
      bufferChar((char)secondByte);
    else if (secondByte == '\n')
    {
      flushBuffer();
      Logger::log<Logger::Level::WARNING, Logger::Topic::IMAGE>("\n>>> ");
    }
    else if (secondByte != '\r') // Skip carriage returns
      bufferChar('.');

    if (isValidFormatHeader(header))
    {
      flushBuffer();
      Logger::log<Logger::Level::WARNING, Logger::Topic::IMAGE>(" <<<\n");
      outHeader = header;
      Logger::log<Logger::Level::INFO, Logger::Topic::IMAGE>("Image header found at offset {}\n", offset - 1);
      return true;
    }
  }

  flushBuffer();
  Logger::log<Logger::Level::WARNING, Logger::Topic::IMAGE>(" <<<\n");

  Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("No valid image header found in first {} bytes\n",
                                                          MAX_HEADER_SCAN_BYTES);
  return false;
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
// Direct Streaming Context
///////////////////////////////////////////////

#if defined(STREAMING_ENABLED) && defined(STREAMING_DIRECT_MODE)

// Context for direct streaming decode
struct DirectStreamContext
{
  StreamingHandler::RowStreamBuffer *buffer;
  uint16_t displayWidth;
  uint16_t displayHeight;
  uint16_t currentRow;       // Current absolute row being decoded
  uint16_t bufferRowIndex;   // Index within the row buffer (0 to bufferRowCount-1)
  uint16_t bufferRowCount;   // Number of rows in buffer
  uint16_t firstRowInBuffer; // First absolute row number in current buffer
  uint32_t pixelsProcessed;
  bool initialized;
};

static DirectStreamContext g_directCtx = {nullptr, 0, 0, 0, 0, 0, 0, 0, false};

// Flush completed rows from buffer to display
static void flushCompletedRows()
{
  if (!g_directCtx.initialized || !g_directCtx.buffer)
    return;

  // Check if current row has any pixels written
  uint16_t currentRowPixels = g_directCtx.buffer->getRowPixelCount(g_directCtx.bufferRowIndex);
  if (currentRowPixels == 0 && g_directCtx.bufferRowIndex == 0)
    return; // Nothing to flush

  uint16_t rowsToFlush = g_directCtx.bufferRowIndex + 1;

  // Get buffer data
  const uint8_t *blackData = g_directCtx.buffer->getRowData(0);
  const uint8_t *colorData = g_directCtx.buffer->getColorRowData(0);

  // Write rows to display
  Display::writeRowsDirect(g_directCtx.firstRowInBuffer, rowsToFlush, blackData, colorData);

  Logger::log<Logger::Level::DEBUG, Logger::Topic::STREAM>("Flushed {} rows starting at y={}\n", rowsToFlush,
                                                           g_directCtx.firstRowInBuffer);

  // Reset buffer for next batch
  for (uint16_t i = 0; i < g_directCtx.bufferRowCount; i++)
  {
    g_directCtx.buffer->resetRow(i);
  }

  // Reset buffer index, caller is responsible for updating firstRowInBuffer
  g_directCtx.bufferRowIndex = 0;
}

// Write a single pixel in direct streaming mode
static void directStreamPixel(uint16_t x, uint16_t y, uint16_t color)
{
  if (!g_directCtx.initialized || !g_directCtx.buffer)
    return;

  // Check if we've moved to a new row
  if (y != g_directCtx.currentRow)
  {
    // Calculate what the buffer row index would be for this new row
    uint16_t newBufferRowIndex = y - g_directCtx.firstRowInBuffer;

    // Check if we need to flush the buffer (new row would exceed buffer capacity)
    if (newBufferRowIndex >= g_directCtx.bufferRowCount)
    {
      flushCompletedRows();
      // After flush, update firstRowInBuffer to the new row we're about to write
      g_directCtx.firstRowInBuffer = y;
      newBufferRowIndex = 0;
    }

    g_directCtx.currentRow = y;
    g_directCtx.bufferRowIndex = newBufferRowIndex;
  }

  // Write pixel to buffer
  g_directCtx.buffer->setPixel(g_directCtx.bufferRowIndex, x, color);
  g_directCtx.pixelsProcessed++;
}

// Initialize direct streaming context
static bool initDirectStreamContext()
{
  StreamingHandler::StreamingManager &streamMgr = StreamingHandler::StreamingManager::getInstance();

  if (!streamMgr.isEnabled() || !streamMgr.isDirectMode())
    return false;

  g_directCtx.buffer = streamMgr.getBuffer();
  g_directCtx.displayWidth = Display::getResolutionX();
  g_directCtx.displayHeight = Display::getResolutionY();
  g_directCtx.currentRow = 0;
  g_directCtx.bufferRowIndex = 0;
  g_directCtx.bufferRowCount = g_directCtx.buffer->getRowCount();
  g_directCtx.firstRowInBuffer = 0;
  g_directCtx.pixelsProcessed = 0;
  g_directCtx.initialized = true;

  return true;
}

// Finalize direct streaming (flush remaining rows)
static void finalizeDirectStream()
{
  if (!g_directCtx.initialized)
    return;

  // Flush any remaining rows in buffer
  // Check if we have any pixels processed in the current buffer batch
  if (g_directCtx.currentRow >= g_directCtx.firstRowInBuffer)
  {
    flushCompletedRows();
  }

  // Log final statistics
  uint32_t totalPixels = (uint32_t)g_directCtx.displayWidth * g_directCtx.displayHeight;
  uint32_t expectedRows = g_directCtx.displayHeight;
  uint32_t lastRowProcessed = g_directCtx.currentRow;

  Logger::log<Logger::Level::DEBUG, Logger::Topic::STREAM>("Finalize: processed {}/{} pixels, last row={}/{}\n",
                                                           g_directCtx.pixelsProcessed, totalPixels, lastRowProcessed,
                                                           expectedRows - 1);

  g_directCtx.initialized = false;
}

#endif // STREAMING_ENABLED && STREAMING_DIRECT_MODE

///////////////////////////////////////////////
// PNG Image Processing
///////////////////////////////////////////////

// Convert RGBA to display color (unified color mapping for all image formats)
static uint16_t rgbaToDisplayColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  if (a == 0)
    return GxEPD_WHITE; // Transparent = white

#if defined(TYPE_BW)
  uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
  return (gray <= 160) ? GxEPD_BLACK : GxEPD_WHITE;

#elif defined(TYPE_3C)
  if (r >= 128 && r > (g + 80) && r > (b + 80))
    return GxEPD_RED;
  uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
  return (gray <= 160) ? GxEPD_BLACK : GxEPD_WHITE;

#elif defined(TYPE_4C)
  if (r > 128 && g > 128 && b < 80)
    return GxEPD_YELLOW;
  if (r > 128 && r > (g + 80) && r > (b + 80))
    return GxEPD_RED;
  uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
  return (gray <= 160) ? GxEPD_BLACK : GxEPD_WHITE;

#elif defined(TYPE_7C)
  if (r > 200 && g > 80 && g < 180 && b < 80)
    return GxEPD_ORANGE;
  if (r > 128 && r > (g + 80) && r > (b + 80))
    return GxEPD_RED;
  if (r > 128 && g > 128 && b < 80)
    return GxEPD_YELLOW;
  if (g > 128 && g > (r + 80) && g > (b + 80))
    return GxEPD_GREEN;
  if (b > 128 && b > (r + 80) && b > (g + 80))
    return GxEPD_BLUE;
  uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
  return (gray <= 160) ? GxEPD_BLACK : GxEPD_WHITE;

#elif defined(TYPE_GRAYSCALE)
  uint8_t gray = (r + g + b) / 3;
  if (gray > 160)
    return GxEPD_WHITE;
  if (gray > 101)
    return GxEPD_LIGHTGREY;
  if (gray > 32)
    return GxEPD_DARKGREY;
  return GxEPD_BLACK;

#else
  uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8;
  return (gray <= 160) ? GxEPD_BLACK : GxEPD_WHITE;
#endif
}

// Callback: Draw pixel from PNG decoder
static void pngleOnDraw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t rgba[4])
{
  if ((x >= Display::getWidth()) || (y >= Display::getHeight()))
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("PNG pixel out of bounds: ({}, {})\n", x, y);
    return;
  }

  uint16_t color = rgbaToDisplayColor(rgba[0], rgba[1], rgba[2], rgba[3]);
  Display::drawPixel(x, y, color);

  // Yield periodically to prevent watchdog timeout
  static uint32_t pixelCount = 0;
  if (++pixelCount % 1000 == 0)
    yield();
}

static bool processPNG(HttpClient &http, uint32_t startTime, uint8_t *buffer, uint16_t bufferSize)
{
  Logger::log<Logger::Topic::IMAGE>("Got format PNG, processing\n");

  // Create pngle decoder
  pngle_t *pngle = pngle_new();
  if (!pngle)
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("Failed to create PNG decoder\n");
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
    Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("PNG Signature error: {}\n", pngle_error(pngle));
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
      Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("PNG Decode error: {}\n", pngle_error(pngle));
      success = false;
      break;
    }

    // Yield periodically
    yield();
  }

  pngle_destroy(pngle);

  if (success)
  {
    Logger::log<Logger::Level::DEBUG, Logger::Topic::HTTP>("Bytes read {}\n", bytes_read);
    Logger::log<Logger::Topic::HTTP>("Loaded in {} ms\n", millis() - startTime);
  }

  return success;
}

///////////////////////////////////////////////
// Direct Streaming Image Processing
///////////////////////////////////////////////

#if defined(STREAMING_ENABLED) && defined(STREAMING_DIRECT_MODE)

// Direct streaming PNG callback
static void pngleOnDrawDirect(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t rgba[4])
{
  if (!g_directCtx.initialized)
    return;

  if (x >= g_directCtx.displayWidth || y >= g_directCtx.displayHeight)
    return;

  uint16_t color = rgbaToDisplayColor(rgba[0], rgba[1], rgba[2], rgba[3]);
  directStreamPixel(x, y, color);

  // Yield periodically
  if (g_directCtx.pixelsProcessed % 1000 == 0)
    yield();
}

static bool processPNGDirect(HttpClient &http, uint32_t startTime, uint8_t *buffer, uint16_t bufferSize)
{
  Logger::log<Logger::Level::INFO, Logger::Topic::IMAGE>("PNG Processing (direct streaming mode)\n");

  if (!initDirectStreamContext())
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("PNG Failed to init direct stream context\n");
    return false;
  }

  pngle_t *pngle = pngle_new();
  if (!pngle)
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("PNG Failed to create decoder\n");
    return false;
  }

  pngle_set_draw_callback(pngle, pngleOnDrawDirect);

  // Reconstruct PNG signature
  uint8_t pngSignature[8];
  pngSignature[0] = 0x89;
  pngSignature[1] = 0x50;

  uint32_t sigBytesRead = http.readBytes(&pngSignature[2], 6);
  if (sigBytesRead != 6)
  {
    printReadError(2 + sigBytesRead);
    pngle_destroy(pngle);
    return false;
  }

  int fed = pngle_feed(pngle, pngSignature, 8);
  if (fed < 0)
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("PNG Signature error: {}\n", pngle_error(pngle));
    pngle_destroy(pngle);
    return false;
  }

  uint32_t bytes_read = 8;
  bool success = true;

  while (http.isConnected() || http.available())
  {
    uint32_t chunkSize = http.readBytes(buffer, bufferSize);
    if (chunkSize == 0)
      break;

    bytes_read += chunkSize;

    fed = pngle_feed(pngle, buffer, chunkSize);
    if (fed < 0)
    {
      Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("PNG Decode error: {}\n", pngle_error(pngle));
      success = false;
      break;
    }

    yield();
  }

  pngle_destroy(pngle);

  // Validate that we received enough pixels before finalizing
  uint32_t totalPixels = (uint32_t)g_directCtx.displayWidth * g_directCtx.displayHeight;
  uint32_t processedPixels = g_directCtx.pixelsProcessed;

  if (processedPixels < totalPixels)
  {
    // Calculate completion percentage
    uint8_t completionPct = (processedPixels * 100) / totalPixels;
    Logger::log<Logger::Level::WARNING, Logger::Topic::IMAGE>("PNG Incomplete: {}/{} pixels ({}%)\n", processedPixels,
                                                              totalPixels, completionPct);

    // Accept if 95%+ complete (some PNGs may have transparent edges)
    if (completionPct < 95)
    {
      Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("PNG Image incomplete, aborting\n");
      finalizeDirectStream(); // Still flush what we have to clean up
      return false;
    }
    Logger::log<Logger::Level::INFO, Logger::Topic::IMAGE>("PNG Image is 95%+ complete, accepting as valid\n");
  }

  finalizeDirectStream();

  if (success)
  {
    Logger::log<Logger::Level::INFO, Logger::Topic::HTTP>("Bytes read {}, pixels processed {}\n", bytes_read,
                                                          g_directCtx.pixelsProcessed);
    Logger::log<Logger::Level::INFO, Logger::Topic::HTTP>("Loaded in {} ms\n", millis() - startTime);
  }

  return success;
}

static bool processRLEDirect(HttpClient &http, uint32_t startTime, ImageFormat format, uint8_t *buffer,
                             uint16_t bufferSize)
{
  Logger::log<Logger::Level::INFO, Logger::Topic::IMAGE>("Processing {} (direct streaming mode)\n",
                                                         formatToString(format));

  if (!initDirectStreamContext())
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("Z-format Failed to init direct stream context\n");
    return false;
  }

  uint32_t bytes_read = 2; // Already read header
  uint16_t w = g_directCtx.displayWidth;
  uint16_t h = g_directCtx.displayHeight;
  uint32_t totalPixels = w * h;

  uint16_t color2 = getSecondColor();
  uint16_t color3 = getThirdColor();

  uint16_t row = 0;
  uint16_t col = 0;

  uint32_t bufferPos = 0;
  uint32_t bufferAvailable = 0;
  bool bufferEmpty = true;

  while (g_directCtx.pixelsProcessed < totalPixels)
  {
    // Refill buffer if needed
    if (bufferEmpty || bufferPos >= bufferAvailable)
    {
      if (!http.isConnected() && !http.available())
      {
        if (g_directCtx.pixelsProcessed >= (totalPixels * 95 / 100))
        {
          Logger::log<Logger::Level::INFO, Logger::Topic::IMAGE>(
            "Z-format Image is 95%+ complete, accepting as valid\n");
          break;
        }
        Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("Z-format Incomplete: {}/{} pixels\n",
                                                                g_directCtx.pixelsProcessed, totalPixels);
        finalizeDirectStream();
        return false;
      }

      uint32_t bytesRead = http.readBytes(buffer, bufferSize);
      if (bytesRead == 0)
        break;

      bufferPos = 0;
      bufferEmpty = false;
      bytes_read += bytesRead;
      bufferAvailable = bytesRead;
    }

    uint8_t pixelColor, count;

    if (format == ImageFormat::Z1)
    {
      if (bufferPos + 1 >= bufferAvailable)
      {
        bufferEmpty = true;
        continue;
      }
      pixelColor = buffer[bufferPos++];
      count = buffer[bufferPos++];
    }
    else
    {
      uint8_t compressed = buffer[bufferPos++];

      if (format == ImageFormat::Z2)
      {
        count = compressed & 0b00111111;
        pixelColor = (compressed & 0b11000000) >> 6;
      }
      else // Z3
      {
        count = compressed & 0b00011111;
        pixelColor = (compressed & 0b11100000) >> 5;
      }
    }

    uint16_t color = mapColorValue(pixelColor, color2, color3);

    // Draw pixels using direct streaming
    for (uint8_t i = 0; i < count && g_directCtx.pixelsProcessed < totalPixels; i++)
    {
      directStreamPixel(col, row, color);

      if (++col >= w)
      {
        col = 0;
        row++;
      }
    }

    if (g_directCtx.pixelsProcessed % 10000 == 0)
      yield();
  }

  finalizeDirectStream();

  Logger::log<Logger::Level::INFO, Logger::Topic::HTTP>("Bytes read {}, pixels processed {}\n", bytes_read,
                                                        g_directCtx.pixelsProcessed);
  Logger::log<Logger::Level::INFO, Logger::Topic::HTTP>("Loaded in {} ms\n", millis() - startTime);

  return (g_directCtx.pixelsProcessed >= totalPixels * 95 / 100);
}

#endif // STREAMING_ENABLED && STREAMING_DIRECT_MODE

///////////////////////////////////////////////
// Unified RLE Format Processing
///////////////////////////////////////////////

static bool processRLE(HttpClient &http, uint32_t startTime, ImageFormat format, uint8_t *buffer, uint16_t bufferSize)
{
  Logger::log<Logger::Level::DEBUG, Logger::Topic::IMAGE>("Z Got format {}, processing\n", formatToString(format));

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
        Logger::log<Logger::Level::WARNING, Logger::Topic::IMAGE>(
          "Z Incomplete image received. Pixels processed: {}/{}\n", pixelsProcessed, totalPixels);

        // If we're close to complete (95%+), consider it a success
        if (pixelsProcessed >= (totalPixels * 95 / 100))
        {
          Logger::log<Logger::Level::WARNING, Logger::Topic::IMAGE>("Z Image is 95%+ complete, accepting as valid\n");
          return true;
        }
        return false;
      }

      uint32_t bytesRead = http.readBytes(buffer, bufferSize);
      if (bytesRead == 0)
      {
        Logger::log<Logger::Level::DEBUG, Logger::Topic::IMAGE>("Z No more data available. Pixels processed: {}/{}\n",
                                                                pixelsProcessed, totalPixels);
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

  Logger::log<Logger::Level::DEBUG, Logger::Topic::HTTP>("Bytes read {}\n", bytes_read);
  Logger::log<Logger::Topic::HTTP>("Loaded in {} ms\n", millis() - startTime);

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
      Logger::log<Logger::Topic::IMAGE>("Streaming enabled\n");
    else
      Logger::log<Logger::Level::WARNING, Logger::Topic::IMAGE>("Streaming init failed, falling back to direct mode\n");
  }

  // Print memory stats
  if (streamMgr.isEnabled())
  {
    size_t totalHeap, freeHeap, bufferUsed;
    streamMgr.getMemoryStats(totalHeap, freeHeap, bufferUsed);
    Logger::log<Logger::Level::DEBUG, Logger::Topic::IMAGE>("Memory - Total: {}, Free: {}, Buffer: {}\n", totalHeap,
                                                            freeHeap, bufferUsed);
  }
#endif

  // Scan for format header (handles potential error messages at start)
  uint16_t header;
  if (!scanForImageHeader(http, header))
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("Failed to find valid image format header\n");
    return;
  }

  Logger::log<Logger::Level::DEBUG, Logger::Topic::IMAGE>("Image format header: 0x{}\n", String(header, HEX).c_str());

  // Dynamic buffer for PNG/RLE processing
  // BMP handles its own buffer allocation
  const uint16_t STREAM_BUFFER_SIZE = 512;
  uint8_t *buffer = new (std::nothrow) uint8_t[STREAM_BUFFER_SIZE];

  if (!buffer)
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("Failed to allocate processing buffer\n");
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
      Logger::log<Logger::Topic::IMAGE>("Unknown image format header: 0x{}\n", String(header, HEX).c_str());
      success = false;
      break;
  }

  // Cleanup processing buffer
  delete[] buffer;
  buffer = nullptr;

  // Handle errors
  if (!success)
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("Image processing failed\n");
    StateManager::setSleepDuration(StateManager::DEFAULT_SLEEP_SECONDS);
    StateManager::setTimestamp(0);
  }

#ifdef STREAMING_ENABLED
  // Cleanup streaming resources after processing
  if (streamMgr.isEnabled())
    streamMgr.cleanup();
#endif
}

///////////////////////////////////////////////
// Direct Streaming Public Interface
///////////////////////////////////////////////

bool isDirectStreamingAvailable()
{
#if defined(STREAMING_ENABLED) && defined(STREAMING_DIRECT_MODE)
  bool displaySupport = Display::supportsDirectStreaming();
  bool packerSupport = PixelPacker::supportsDirectStreaming();
  Logger::log<Logger::Level::DEBUG, Logger::Topic::IMAGE>("Direct streaming check: display={}, packer={}\n",
                                                          displaySupport, packerSupport);
  return displaySupport && packerSupport;
#else
  Logger::log<Logger::Level::INFO, Logger::Topic::IMAGE>("Direct streaming disabled at compile time\n");
  return false;
#endif
}

ImageStreamingResult readImageDataDirect(HttpClient &http)
{
#if defined(STREAMING_ENABLED) && defined(STREAMING_DIRECT_MODE)
  if (!isDirectStreamingAvailable())
  {
    Logger::log<Logger::Level::WARNING, Logger::Topic::IMAGE>("Direct streaming not available, use paged mode\n");
    return ImageStreamingResult::FallbackToPaged;
  }

  uint32_t startTime = millis();
  bool success = false;

  // Scan for format header (handles potential error messages at start)
  uint16_t headerValue;
  if (!scanForImageHeader(http, headerValue))
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("Failed to find valid image format header\n");
    return ImageStreamingResult::FatalError;
  }

  ImageFormat format = static_cast<ImageFormat>(headerValue);
  Logger::log<Logger::Level::DEBUG, Logger::Topic::IMAGE>("Header: 0x{} (direct mode)\n",
                                                          String(static_cast<uint16_t>(format), HEX).c_str());

  // Initialize streaming manager in direct mode with appropriate memory reserve
  StreamingHandler::StreamingManager &streamMgr = StreamingHandler::StreamingManager::getInstance();

  if (!streamMgr.isEnabled())
  {
    uint16_t displayWidth = Display::getResolutionX();

    bool needsPngDecoder = (format == ImageFormat::PNG);
    if (!streamMgr.initDirect(displayWidth, STREAMING_BUFFER_ROWS_COUNT, needsPngDecoder))
    {
      Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("Failed to initialize direct streaming\n");
      return ImageStreamingResult::FallbackToPaged;
    }

    size_t totalHeap, freeHeap, bufferUsed;
    streamMgr.getMemoryStats(totalHeap, freeHeap, bufferUsed);
    Logger::log<Logger::Level::DEBUG, Logger::Topic::IMAGE>("Direct streaming - Total: {}, Free: {}, Buffer: {}\n",
                                                            totalHeap, freeHeap, bufferUsed);
  }

  // Allocate processing buffer
  const uint16_t STREAM_BUFFER_SIZE = 512;
  uint8_t *buffer = new (std::nothrow) uint8_t[STREAM_BUFFER_SIZE];

  if (!buffer)
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("Failed to allocate processing buffer\n");
    streamMgr.cleanup();
    return ImageStreamingResult::FallbackToPaged;
  }

  // Route to direct streaming format handlers
  switch (format)
  {
    case ImageFormat::PNG:
      success = processPNGDirect(http, startTime, buffer, STREAM_BUFFER_SIZE);
      break;

    case ImageFormat::Z1:
      success = processRLEDirect(http, startTime, ImageFormat::Z1, buffer, STREAM_BUFFER_SIZE);
      break;

    case ImageFormat::Z2:
      success = processRLEDirect(http, startTime, ImageFormat::Z2, buffer, STREAM_BUFFER_SIZE);
      break;

    case ImageFormat::Z3:
      success = processRLEDirect(http, startTime, ImageFormat::Z3, buffer, STREAM_BUFFER_SIZE);
      break;

    default:
      Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("Unknown format header: 0x{}\n",
                                                              String(static_cast<uint16_t>(format), HEX).c_str());
      success = false;
      break;
  }

  delete[] buffer;

  if (!success)
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::IMAGE>("Direct streaming failed\n");
    StateManager::setSleepDuration(StateManager::DEFAULT_SLEEP_SECONDS);
    StateManager::setTimestamp(0);
  }

  streamMgr.cleanup();
  return success ? ImageStreamingResult::Success : ImageStreamingResult::FatalError;

#else
  // Direct streaming not compiled in
  Logger::log<Logger::Level::INFO, Logger::Topic::IMAGE>("Direct streaming not enabled at compile time\n");
  return ImageStreamingResult::FallbackToPaged;
#endif
}

} // namespace ImageHandler
