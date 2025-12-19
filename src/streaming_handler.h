#ifndef STREAMING_HANDLER_H
#define STREAMING_HANDLER_H

// Streaming feature control - enabled by default
#ifndef STREAMING_DISABLED
  #define STREAMING_ENABLED
#endif

#ifdef STREAMING_ENABLED
  #ifndef STREAMING_BUFFER_ROWS_COUNT
    #define STREAMING_BUFFER_ROWS_COUNT 1 // Default: buffer 1 row at a time
  #endif

  #include <Arduino.h>
  #include <cstdint>
  #include <vector>

namespace StreamingHandler
{

// Row buffer configuration
constexpr size_t MAX_ROW_SIZE = 4096; // Maximum size for a single row (safety limit)

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

  size_t writeRow(size_t rowIndex, const uint8_t *data, size_t length);

  const uint8_t *getRowData(size_t rowIndex) const;
  size_t getRowSize() const { return m_rowSize; }
  size_t getRowCount() const { return m_rowCount; }
  size_t getTotalSize() const { return m_rowSize * m_rowCount; }

  void clear();
  void resetRow(size_t rowIndex);

  bool isInitialized() const { return m_initialized; }

private:
  std::vector<uint8_t> m_buffer;
  std::vector<size_t> m_rowWritePos;
  size_t m_rowSize;
  size_t m_rowCount;
  bool m_initialized;
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

  RowStreamBuffer *getBuffer() { return m_enabled ? &m_buffer : nullptr; }

  void getMemoryStats(size_t &totalHeap, size_t &freeHeap, size_t &bufferUsed) const;

  bool isEnabled() const { return m_enabled; }

  void cleanup();

  // Delete copy/move constructors for singleton
  StreamingManager(const StreamingManager &) = delete;
  StreamingManager &operator=(const StreamingManager &) = delete;
  StreamingManager(StreamingManager &&) = delete;
  StreamingManager &operator=(StreamingManager &&) = delete;

private:
  StreamingManager() : m_enabled(false) {}
  ~StreamingManager() { cleanup(); }

  RowStreamBuffer m_buffer;
  bool m_enabled;
};

} // namespace StreamingHandler

#endif // STREAMING_ENABLED

#endif // STREAMING_HANDLER_H
