#include "streaming_handler.h"

#include "utils.h"

#ifdef STREAMING_ENABLED

namespace StreamingHandler
{

// RowStreamBuffer Implementation
RowStreamBuffer::RowStreamBuffer() : m_rowSize(0), m_rowCount(0), m_initialized(false) {}

RowStreamBuffer::~RowStreamBuffer()
{
  m_buffer.clear();
  m_buffer.shrink_to_fit();
  m_rowWritePos.clear();
  m_rowWritePos.shrink_to_fit();
}

bool RowStreamBuffer::init(size_t rowSizeBytes, size_t rowCount)
{
  if (m_initialized)
  {
    Serial.println("[Streaming] RowBuffer already initialized");
    return true;
  }

  if (rowSizeBytes == 0 || rowSizeBytes > MAX_ROW_SIZE)
  {
    Serial.printf("[Streaming] Invalid row size: %zu (max: %zu)\n", rowSizeBytes, MAX_ROW_SIZE);
    return false;
  }

  if (rowCount == 0)
  {
    Serial.println("[Streaming] Invalid row count: 0");
    return false;
  }

  size_t totalSize = rowSizeBytes * rowCount;

  // Check available heap
  size_t freeHeap = Utils::getFreeHeap();
  if (freeHeap < totalSize * 2)
  {
    Serial.printf("[Streaming] Insufficient heap: %zu bytes free, need %zu for row buffer\n", freeHeap, totalSize * 2);
    return false;
  }

  try
  {
    m_buffer.resize(totalSize);
    m_rowWritePos.resize(rowCount, 0);
    m_rowSize = rowSizeBytes;
    m_rowCount = rowCount;
    m_initialized = true;
    Serial.printf("[Streaming] Row buffer initialized: %zu bytes/row × %zu rows = %zu bytes total\n", rowSizeBytes,
                  rowCount, totalSize);
    return true;
  }
  catch (const std::bad_alloc &e)
  {
    Serial.printf("[Streaming] Row buffer allocation failed: %s\n", e.what());
    return false;
  }
}

size_t RowStreamBuffer::writeRow(size_t rowIndex, const uint8_t *data, size_t length)
{
  if (!m_initialized || !data || length == 0)
    return 0;

  if (rowIndex >= m_rowCount)
  {
    Serial.printf("[Streaming] Invalid row index: %zu (max: %zu)\n", rowIndex, m_rowCount - 1);
    return 0;
  }

  // Calculate row offset
  size_t rowOffset = rowIndex * m_rowSize;
  size_t writePos = m_rowWritePos[rowIndex];

  // Don't overflow this row's buffer
  size_t available = m_rowSize - writePos;
  size_t toWrite = (length > available) ? available : length;

  if (toWrite > 0)
  {
    std::copy(data, data + toWrite, m_buffer.begin() + rowOffset + writePos);
    m_rowWritePos[rowIndex] += toWrite;
  }

  return toWrite;
}

const uint8_t *RowStreamBuffer::getRowData(size_t rowIndex) const
{
  if (!m_initialized || rowIndex >= m_rowCount)
    return nullptr;

  size_t rowOffset = rowIndex * m_rowSize;
  return m_buffer.data() + rowOffset;
}

void RowStreamBuffer::clear()
{
  if (m_initialized)
  {
    std::fill(m_rowWritePos.begin(), m_rowWritePos.end(), 0);
    std::fill(m_buffer.begin(), m_buffer.end(), 0);
  }
}

void RowStreamBuffer::resetRow(size_t rowIndex)
{
  if (m_initialized && rowIndex < m_rowCount)
  {
    m_rowWritePos[rowIndex] = 0;
  }
}

// StreamingManager Implementation
bool StreamingManager::init(size_t rowSizeBytes, size_t rowCount)
{
  if (m_enabled)
  {
    Serial.println("[Streaming] Manager already enabled");
    return true;
  }

  if (!m_buffer.init(rowSizeBytes, rowCount))
  {
    Serial.println("[Streaming] Failed to initialize row buffer");
    return false;
  }

  m_enabled = true;
  Serial.println("[Streaming] Manager initialized successfully");
  return true;
}

void StreamingManager::getMemoryStats(size_t &totalHeap, size_t &freeHeap, size_t &bufferUsed) const
{
  totalHeap = Utils::getTotalHeap();
  freeHeap = Utils::getFreeHeap();
  bufferUsed = m_enabled ? m_buffer.getTotalSize() : 0;
}

void StreamingManager::cleanup()
{
  if (m_enabled)
  {
    m_buffer.clear();
    m_enabled = false;
    Serial.println("[Streaming] Manager cleanup complete");
  }
}

} // namespace StreamingHandler

#endif // STREAMING_ENABLED
