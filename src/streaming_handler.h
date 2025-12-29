#ifndef STREAMING_HANDLER_H
#define STREAMING_HANDLER_H

// Streaming feature control - enabled by default
#ifndef STREAMING_DISABLED
  #define STREAMING_ENABLED
#endif

// Direct streaming mode - bypass paged output and write rows directly to display controller
// Disable for 4C displays (not supported) or when paged mode is preferred
#ifndef STREAMING_DIRECT_DISABLED
  #define STREAMING_DIRECT_MODE
#endif

#ifdef STREAMING_ENABLED
  #ifndef STREAMING_BUFFER_ROWS_COUNT
    #define STREAMING_BUFFER_ROWS_COUNT 48 // Default row buffer size (will be reduced if heap is limited)
  #endif

  #include <Arduino.h>
  #include <cstdint>
  #include <memory>
  #include <vector>

  #include "pixel_packer.h"

namespace StreamingHandler
{

// Row buffer configuration
constexpr size_t MAX_ROW_SIZE = 1200; // Maximum size for a single row (safety limit)

// Row-based streaming buffer for direct display writing
class RowStreamBuffer
{
public:
  RowStreamBuffer();
  ~RowStreamBuffer();

  // Prevent copying
  RowStreamBuffer(const RowStreamBuffer &) = delete;
  RowStreamBuffer &operator=(const RowStreamBuffer &) = delete;

  bool init(size_t rowSizeBytes, size_t rowCount);

  // Initialize for direct streaming mode with display format
  bool initDirect(uint16_t displayWidth, size_t rowCount, PixelPacker::DisplayFormat format);

  size_t writeRow(size_t rowIndex, const uint8_t *data, size_t length);

  const uint8_t *getRowData(size_t rowIndex) const;
  size_t getRowSize() const { return m_rowSize; }
  size_t getRowCount() const { return m_rowCount; }
  size_t getTotalSize() const { return m_rowSize * m_rowCount; }

  // Get secondary buffer for 3C color plane
  const uint8_t *getColorRowData(size_t rowIndex) const;
  uint8_t *getColorRowDataMutable(size_t rowIndex);

  // Direct pixel packing methods
  void setPixel(size_t rowIndex, uint16_t x, uint16_t color);
  void setPixelGrey(size_t rowIndex, uint16_t x, uint8_t grey);

  // Row management
  void clearRow(size_t rowIndex);
  bool isRowComplete(size_t rowIndex, uint16_t expectedPixels) const;
  uint16_t getRowPixelCount(size_t rowIndex) const;
  void incrementRowPixelCount(size_t rowIndex);

  void clear();
  void resetRow(size_t rowIndex);

  bool isInitialized() const { return m_initialized; }
  bool isDirectMode() const { return m_directMode; }
  PixelPacker::DisplayFormat getFormat() const { return m_format; }
  uint16_t getDisplayWidth() const { return m_displayWidth; }

private:
  std::vector<uint8_t> m_buffer;
  std::vector<uint8_t> m_colorBuffer; // Secondary buffer for 3C color plane
  std::vector<size_t> m_rowWritePos;
  std::vector<uint16_t> m_rowPixelCount; // Track pixels written per row
  size_t m_rowSize;
  size_t m_rowCount;
  uint16_t m_displayWidth;
  PixelPacker::DisplayFormat m_format;
  bool m_initialized;
  bool m_directMode;
};

// Singleton manager for row streaming
class StreamingManager
{
public:
  static StreamingManager &getInstance()
  {
    static StreamingManager instance;
    return instance;
  }

  bool init(size_t rowSizeBytes, size_t rowCount = STREAMING_BUFFER_ROWS_COUNT);

  // Initialize for direct streaming mode
  bool initDirect(uint16_t displayWidth, size_t rowCount = STREAMING_BUFFER_ROWS_COUNT);

  RowStreamBuffer *getBuffer() { return m_buffer.get(); }

  void getMemoryStats(size_t &totalHeap, size_t &freeHeap, size_t &bufferUsed) const;

  bool isEnabled() const { return m_buffer != nullptr; }
  bool isDirectMode() const { return m_buffer && m_buffer->isDirectMode(); }

  void cleanup();

  // Delete copy/move constructors for singleton
  StreamingManager(const StreamingManager &) = delete;
  StreamingManager &operator=(const StreamingManager &) = delete;
  StreamingManager(StreamingManager &&) = delete;
  StreamingManager &operator=(StreamingManager &&) = delete;

private:
  StreamingManager() = default;
  ~StreamingManager() { cleanup(); }

  std::unique_ptr<RowStreamBuffer> m_buffer;
};

} // namespace StreamingHandler

#endif // STREAMING_ENABLED

#endif // STREAMING_HANDLER_H
