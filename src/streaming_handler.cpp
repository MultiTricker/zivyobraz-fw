#include "streaming_handler.h"

#include "board.h"
#include "logger.h"
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
    Logger::log<Logger::Level::DEBUG, Logger::Topic::STREAM>("RowBuffer already initialized\n");
    return true;
  }

  if (rowSizeBytes == 0 || rowSizeBytes > MAX_ROW_SIZE)
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::STREAM>("Invalid row size: {} (max: {})\n", rowSizeBytes,
                                                             MAX_ROW_SIZE);
    return false;
  }

  if (rowCount == 0)
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::STREAM>("Invalid row count: 0\n");
    return false;
  }

  // Respect board's maximum page buffer size limit
  size_t maxAllowedSize = BOARD_MAX_PAGE_BUFFER_SIZE;
  size_t maxRowCount = maxAllowedSize / rowSizeBytes;

  if (maxRowCount == 0)
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::STREAM>("Row size {} exceeds board buffer limit {}\n",
                                                             rowSizeBytes, maxAllowedSize);
    return false;
  }

  // Cap requested row count to what the board can handle
  if (rowCount > maxRowCount)
  {
    Logger::log<Logger::Level::WARNING, Logger::Topic::STREAM>(
      "Requested {} rows exceeds board limit, capping to {} rows\n", rowCount, maxRowCount);
    rowCount = maxRowCount;
  }

  // Try to allocate with requested rowCount, fall back to smaller counts if needed
  size_t tryRowCount = rowCount;
  size_t freeHeap = Utils::getFreeHeap();

  while (tryRowCount > 0)
  {
    size_t totalSize = rowSizeBytes * tryRowCount;

    // Check if we have enough heap (need 2x for safety margin)
    if (freeHeap >= totalSize * 2)
    {
      try
      {
        m_buffer.resize(totalSize);
        m_rowWritePos.resize(tryRowCount, 0);
        m_rowSize = rowSizeBytes;
        m_rowCount = tryRowCount;
        m_initialized = true;

        if (tryRowCount < rowCount)
        {
          Logger::log<Logger::Level::DEBUG, Logger::Topic::STREAM>(
            "Row buffer initialized with fallback: {} bytes/row x {} rows = {} bytes total (requested {} rows)\n",
            rowSizeBytes, tryRowCount, totalSize, rowCount);
        }
        else
        {
          Logger::log<Logger::Level::DEBUG, Logger::Topic::STREAM>(
            "Row buffer initialized: {} bytes/row x {} rows = {} bytes total\n", rowSizeBytes, tryRowCount, totalSize);
        }
        return true;
      }
      catch (const std::bad_alloc &e)
      {
        Logger::log<Logger::Level::WARNING, Logger::Topic::STREAM>(
          "Allocation failed for {} rows: {}, trying smaller buffer...\n", tryRowCount, e.what());
        // Fall through to try with fewer rows
      }
    }
    else
    {
      Logger::log<Logger::Level::DEBUG, Logger::Topic::STREAM>(
        "Insufficient heap for {} rows: {} bytes free, need {}\n", tryRowCount, freeHeap, totalSize * 2);
    }

    // Try with half the rows (or 1 if already at 2)
    tryRowCount = (tryRowCount > 2) ? (tryRowCount / 2) : (tryRowCount - 1);
  }

  Logger::log<Logger::Level::ERROR, Logger::Topic::STREAM>("Failed to allocate row buffer with any row count\n");
  return false;
}

size_t RowStreamBuffer::writeRow(size_t rowIndex, const uint8_t *data, size_t length)
{
  if (!m_initialized || !data || length == 0)
    return 0;

  if (rowIndex >= m_rowCount)
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::STREAM>("Invalid row index: {} (max: {})\n", rowIndex,
                                                             m_rowCount - 1);
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
    Logger::log<Logger::Topic::STREAM>("Manager already enabled\n");
    return true;
  }

  if (!m_buffer.init(rowSizeBytes, rowCount))
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::STREAM>("Failed to initialize row buffer\n");
    return false;
  }

  m_enabled = true;
  Logger::log<Logger::Level::DEBUG, Logger::Topic::STREAM>("Manager initialized successfully\n");
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
    Logger::log<Logger::Level::DEBUG, Logger::Topic::STREAM>("Manager cleanup complete\n");
  }
}

} // namespace StreamingHandler

#endif // STREAMING_ENABLED
