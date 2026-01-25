#include "streaming_handler.h"
#include "pixel_packer.h"

#include "board.h"
#include "logger.h"
#include "utils.h"

#ifdef STREAMING_ENABLED

namespace StreamingHandler
{

// RowStreamBuffer Implementation
RowStreamBuffer::RowStreamBuffer()
    : m_rowSize(0),
      m_rowCount(0),
      m_displayWidth(0),
      m_format(PixelPacker::DisplayFormat::BW),
      m_initialized(false),
      m_directMode(false)
{
}

RowStreamBuffer::~RowStreamBuffer()
{
  m_buffer.clear();
  m_buffer.shrink_to_fit();
  m_colorBuffer.clear();
  m_colorBuffer.shrink_to_fit();
  m_rowWritePos.clear();
  m_rowWritePos.shrink_to_fit();
  m_rowPixelCount.clear();
  m_rowPixelCount.shrink_to_fit();
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
        // Reserve first to ensure single allocation, then resize
        m_buffer.reserve(totalSize);
        m_buffer.resize(totalSize);
        m_rowWritePos.reserve(tryRowCount);
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

  Logger::log<Logger::Level::ERROR, Logger::Topic::STREAM>("Row buffer allocation failed after all attempts\n");
  return false;
}

bool RowStreamBuffer::initDirect(uint16_t displayWidth, size_t rowCount, PixelPacker::DisplayFormat format,
                                 bool needsPngDecoder)
{
  if (m_initialized)
  {
    Logger::log<Logger::Level::WARNING, Logger::Topic::STREAM>("RowBuffer already initialized\n");
    return true;
  }

  m_format = format;
  m_displayWidth = displayWidth;
  m_rowSize = PixelPacker::getRowBufferSize(displayWidth, format);

  if (m_rowSize == 0 || m_rowSize > MAX_ROW_SIZE)
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::STREAM>("Invalid row size: {} (max: {})\n", m_rowSize,
                                                             MAX_ROW_SIZE);
    return false;
  }

  if (rowCount == 0)
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::STREAM>("Invalid row count: 0\n");
    return false;
  }

  bool needs3CColorBuffer = (format == PixelPacker::DisplayFormat::COLOR_3C);
  size_t buffersNeeded = needs3CColorBuffer ? 2 : 1;

  // Check available heap and dynamically adjust row count if needed
  // IMPORTANT: Use largest contiguous block, not total free heap, to avoid fragmentation issues
  size_t freeHeap = Utils::getFreeHeap();
  size_t largestBlock = Utils::getLargestFreeBlock();

  // Reserve memory based on what decoder will be used
  // PNG decoder (pngle) needs: ~1KB base + width*4 for RGBA scanline + zlib state (~32KB) ≈ 40KB total
  // Z format (RLE) only needs a small HTTP buffer (512 bytes) + general overhead
  constexpr size_t PNG_DECODER_RESERVE = 50 * 1024; // 40KB for PNG decoder + 10KB margin
  constexpr size_t MIN_FREE_HEAP = 10 * 1024;       // 10KB minimum free heap after allocation

  size_t memoryReserve = needsPngDecoder ? (PNG_DECODER_RESERVE + MIN_FREE_HEAP) : MIN_FREE_HEAP;

  size_t bytesPerRow = m_rowSize * buffersNeeded;
  // Also account for rowWritePos (size_t) and rowPixelCount (uint16_t) vectors
  size_t overheadPerRow = sizeof(size_t) + sizeof(uint16_t);
  size_t totalBytesPerRow = bytesPerRow + overheadPerRow;

  // Calculate how much we can safely allocate
  size_t maxBufferAllocation = (largestBlock > memoryReserve) ? (largestBlock - memoryReserve) : 0;

  // Calculate maximum rows that can fit
  size_t maxAffordableRows = maxBufferAllocation / totalBytesPerRow;

  // Minimum of 8 rows required for reasonable operation
  constexpr size_t MIN_ROW_COUNT = 8;

  Logger::log<Logger::Level::DEBUG, Logger::Topic::STREAM>(
    "Memory: heap={}, largest={}, reserve={} (png={}), max_alloc={}, bytes/row={} ({}x buf)\n", freeHeap, largestBlock,
    memoryReserve, needsPngDecoder ? 1 : 0, maxBufferAllocation, totalBytesPerRow, buffersNeeded);

  if (maxAffordableRows < MIN_ROW_COUNT)
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::STREAM>(
      "Insufficient heap: {} bytes free, can only fit {} rows (min: {})\n", freeHeap, maxAffordableRows, MIN_ROW_COUNT);
    return false;
  }

  // Use the smaller of requested or affordable row count
  size_t actualRowCount = (rowCount < maxAffordableRows) ? rowCount : maxAffordableRows;

  if (actualRowCount < rowCount)
  {
    Logger::log<Logger::Level::WARNING, Logger::Topic::STREAM>("Reducing row buffer: {} -> {} rows (heap limited)\n",
                                                               rowCount, actualRowCount);
  }

  // Try to allocate with actualRowCount, fall back to smaller counts if allocation fails
  size_t tryRowCount = actualRowCount;

  while (tryRowCount >= MIN_ROW_COUNT)
  {
    size_t totalSize = m_rowSize * tryRowCount;

    try
    {
      // Clear any previous failed allocation attempts
      m_buffer.clear();
      m_buffer.shrink_to_fit();
      m_rowWritePos.clear();
      m_rowWritePos.shrink_to_fit();
      m_rowPixelCount.clear();
      m_rowPixelCount.shrink_to_fit();
      if (needs3CColorBuffer)
      {
        m_colorBuffer.clear();
        m_colorBuffer.shrink_to_fit();
      }

      // Reserve first to ensure single allocation, then resize
      m_buffer.reserve(totalSize);
      m_buffer.resize(totalSize);
      m_rowWritePos.reserve(tryRowCount);
      m_rowWritePos.resize(tryRowCount, 0);
      m_rowPixelCount.reserve(tryRowCount);
      m_rowPixelCount.resize(tryRowCount, 0);
      m_rowCount = tryRowCount;

      // Allocate color buffer for 3C displays
      if (needs3CColorBuffer)
      {
        m_colorBuffer.reserve(totalSize);
        m_colorBuffer.resize(totalSize);
      }

      // Initialize buffers to white
      for (size_t i = 0; i < tryRowCount; i++)
      {
        clearRow(i);
      }

      m_directMode = true;
      m_initialized = true;

      if (tryRowCount < actualRowCount)
      {
        Logger::log<Logger::Level::WARNING, Logger::Topic::STREAM>(
          "Direct mode initialized with fallback: {} bytes/row × {} rows (requested {})\n", m_rowSize, tryRowCount,
          rowCount);
      }
      else
      {
        Logger::log<Logger::Level::DEBUG, Logger::Topic::STREAM>(
          "Direct mode initialized: {}x{} format={}, {} bytes/row × {} rows\n", displayWidth, tryRowCount,
          static_cast<int>(format), m_rowSize, tryRowCount);
      }
      if (needs3CColorBuffer)
      {
        size_t dualBufferTotal = totalSize * 2;
        Logger::log<Logger::Level::DEBUG, Logger::Topic::STREAM>(
          "3C mode: dual buffers allocated (black + color), total {} bytes\n", dualBufferTotal);
      }
      return true;
    }
    catch (const std::bad_alloc &e)
    {
      Logger::log<Logger::Level::WARNING, Logger::Topic::STREAM>(
        "Allocation failed for {} rows: {}, trying smaller buffer...\n", tryRowCount, e.what());
      // Try with half the rows
      tryRowCount = tryRowCount / 2;
    }
  }

  Logger::log<Logger::Level::ERROR, Logger::Topic::STREAM>(
    "Direct buffer allocation failed after all attempts (min rows: {})\n", MIN_ROW_COUNT);
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
    std::fill(m_rowPixelCount.begin(), m_rowPixelCount.end(), 0);
    std::fill(m_buffer.begin(), m_buffer.end(), 0);
    if (!m_colorBuffer.empty())
      std::fill(m_colorBuffer.begin(), m_colorBuffer.end(), 0);
  }
}

void RowStreamBuffer::resetRow(size_t rowIndex)
{
  if (m_initialized && rowIndex < m_rowCount)
  {
    m_rowWritePos[rowIndex] = 0;
    m_rowPixelCount[rowIndex] = 0;
    if (m_directMode)
      clearRow(rowIndex);
  }
}

const uint8_t *RowStreamBuffer::getColorRowData(size_t rowIndex) const
{
  if (!m_initialized || rowIndex >= m_rowCount || m_colorBuffer.empty())
    return nullptr;

  size_t rowOffset = rowIndex * m_rowSize;
  return m_colorBuffer.data() + rowOffset;
}

uint8_t *RowStreamBuffer::getColorRowDataMutable(size_t rowIndex)
{
  if (!m_initialized || rowIndex >= m_rowCount || m_colorBuffer.empty())
    return nullptr;

  size_t rowOffset = rowIndex * m_rowSize;
  return m_colorBuffer.data() + rowOffset;
}

void RowStreamBuffer::setPixel(size_t rowIndex, uint16_t x, uint16_t color)
{
  if (!m_initialized || !m_directMode || rowIndex >= m_rowCount || x >= m_displayWidth)
    return;

  size_t rowOffset = rowIndex * m_rowSize;
  uint8_t *rowData = m_buffer.data() + rowOffset;

  switch (m_format)
  {
    case PixelPacker::DisplayFormat::BW:
      PixelPacker::packPixelBW(rowData, x, color == 0x0000); // GxEPD_BLACK
      break;

    case PixelPacker::DisplayFormat::GRAYSCALE:
      PixelPacker::packPixel4G(rowData, x, PixelPacker::gxepdToGrey(color));
      break;

    case PixelPacker::DisplayFormat::COLOR_3C:
    {
      uint8_t *colorData = m_colorBuffer.data() + rowOffset;
      PixelPacker::packPixel3C(rowData, colorData, x, color);
    }
    break;

    case PixelPacker::DisplayFormat::COLOR_7C:
      PixelPacker::packPixel7C(rowData, x, PixelPacker::gxepdTo7CColor(color));
      break;

    case PixelPacker::DisplayFormat::COLOR_4C:
      PixelPacker::packPixel4C(rowData, x, PixelPacker::gxepdTo4CColor(color));
      break;

    default:
      break;
  }

  incrementRowPixelCount(rowIndex);
}

void RowStreamBuffer::setPixelGrey(size_t rowIndex, uint16_t x, uint8_t grey)
{
  if (!m_initialized || !m_directMode || rowIndex >= m_rowCount || x >= m_displayWidth)
    return;

  size_t rowOffset = rowIndex * m_rowSize;
  uint8_t *rowData = m_buffer.data() + rowOffset;

  if (m_format == PixelPacker::DisplayFormat::GRAYSCALE)
    PixelPacker::packPixel4G(rowData, x, grey);
  else if (m_format == PixelPacker::DisplayFormat::BW)
    PixelPacker::packPixelBW(rowData, x, grey < 128);

  incrementRowPixelCount(rowIndex);
}

void RowStreamBuffer::clearRow(size_t rowIndex)
{
  if (!m_initialized || rowIndex >= m_rowCount)
    return;

  size_t rowOffset = rowIndex * m_rowSize;
  uint8_t *rowData = m_buffer.data() + rowOffset;

  PixelPacker::initRowBuffer(rowData, m_rowSize, m_format);

  if (m_format == PixelPacker::DisplayFormat::COLOR_3C && !m_colorBuffer.empty())
  {
    uint8_t *colorData = m_colorBuffer.data() + rowOffset;
    PixelPacker::initRowBuffer(colorData, m_rowSize, m_format);
  }

  m_rowPixelCount[rowIndex] = 0;
}

bool RowStreamBuffer::isRowComplete(size_t rowIndex, uint16_t expectedPixels) const
{
  if (!m_initialized || rowIndex >= m_rowCount)
    return false;

  return m_rowPixelCount[rowIndex] >= expectedPixels;
}

uint16_t RowStreamBuffer::getRowPixelCount(size_t rowIndex) const
{
  if (!m_initialized || rowIndex >= m_rowCount)
    return 0;

  return m_rowPixelCount[rowIndex];
}

void RowStreamBuffer::incrementRowPixelCount(size_t rowIndex)
{
  if (m_initialized && rowIndex < m_rowCount)
    m_rowPixelCount[rowIndex]++;
}

// StreamingManager Implementation
bool StreamingManager::init(size_t rowSizeBytes, size_t rowCount)
{
  if (m_buffer)
  {
    Logger::log<Logger::Topic::STREAM>("Manager already enabled\n");
    return true;
  }

  m_buffer.reset(new RowStreamBuffer());
  if (!m_buffer->init(rowSizeBytes, rowCount))
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::STREAM>("Failed to initialize row buffer\n");
    m_buffer.reset();
    return false;
  }

  Logger::log<Logger::Level::DEBUG, Logger::Topic::STREAM>("Manager initialized successfully\n");
  return true;
}

bool StreamingManager::initDirect(uint16_t displayWidth, size_t rowCount, bool needsPngDecoder)
{
  if (m_buffer)
  {
    Logger::log<Logger::Level::WARNING, Logger::Topic::STREAM>("Manager already enabled\n");
    return true;
  }

  PixelPacker::DisplayFormat format = PixelPacker::getDisplayFormat();

  if (!PixelPacker::supportsDirectStreaming())
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::STREAM>("Direct streaming not supported for this display type\n");
    return false;
  }

  m_buffer.reset(new RowStreamBuffer());
  if (!m_buffer->initDirect(displayWidth, rowCount, format, needsPngDecoder))
  {
    Logger::log<Logger::Level::ERROR, Logger::Topic::STREAM>("Failed to initialize direct row buffer\n");
    m_buffer.reset();
    return false;
  }

  Logger::log<Logger::Level::INFO, Logger::Topic::STREAM>("Manager initialized in direct mode\n");
  return true;
}

void StreamingManager::getMemoryStats(size_t &totalHeap, size_t &freeHeap, size_t &bufferUsed) const
{
  totalHeap = Utils::getTotalHeap();
  freeHeap = Utils::getFreeHeap();
  bufferUsed = m_buffer ? m_buffer->getTotalSize() : 0;
}

void StreamingManager::cleanup()
{
  if (m_buffer)
  {
    m_buffer.reset();
    Logger::log<Logger::Level::DEBUG, Logger::Topic::STREAM>("Manager cleanup complete\n");
  }
}

} // namespace StreamingHandler

#endif // STREAMING_ENABLED
