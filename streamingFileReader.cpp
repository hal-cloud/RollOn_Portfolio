#include "streamingFileReader.h"
#include <algorithm>

bool StreamingFileReader::Open(const StreamingSoundData& streamData, uint32_t startOffsetBytes)
{
    Close();

    m_file.open(streamData.filename, std::ios::binary);
    if (!m_file) return false;

    const uint32_t chunkSize = streamData.dataChunkSize;
    if (chunkSize == 0 || startOffsetBytes >= chunkSize)
    {
        Close();
        return false;
    }

    m_dataStartOffset = streamData.dataChunkOffset;
    m_startOffset     = startOffsetBytes;
    m_totalBytes      = chunkSize - startOffsetBytes;
    m_position        = 0;

    m_file.seekg(m_dataStartOffset + m_startOffset, std::ios::beg);
    if (!m_file)
    {
        Close();
        return false;
    }

    return true;
}

void StreamingFileReader::Close()
{
    if (m_file.is_open()) m_file.close();
    m_position = 0;
}

uint32_t StreamingFileReader::ReadChunk(std::vector<BYTE>& buffer, uint32_t bufferSize)
{
    if (!IsOpen() || IsEndOfData()) return 0;

    const uint32_t remaining  = m_totalBytes - m_position;
    const uint32_t bytesToRead = std::min<uint32_t>(bufferSize, remaining);

    m_file.read(reinterpret_cast<char*>(buffer.data()), bytesToRead);
    const uint32_t bytesRead = static_cast<uint32_t>(m_file.gcount());

    if (bytesRead == 0)
    {
        Close();
        return 0;
    }

    m_position += bytesRead;
    return bytesRead;
}

void StreamingFileReader::SeekToStart()
{
    if (!IsOpen()) return;
    m_file.clear();
    m_file.seekg(m_dataStartOffset + m_startOffset, std::ios::beg);
    m_position = 0;
}