#include "soundPlayer.h"
#include "soundManager.h"
#include <cstring>
#include <algorithm>

namespace
{
    uint32_t ResolveBytesPerSecond(const WAVEFORMATEX& format)
    {
        if (format.nAvgBytesPerSec > 0)
        {
            return format.nAvgBytesPerSec;
        }
        return format.nSamplesPerSec * std::max<uint16_t>(1, format.nBlockAlign);
    }
}

SoundPlayer::SoundPlayer(IXAudio2* xAudio2, const SoundData* soundData)
    : m_sourceVoice(nullptr)
    , m_soundData(soundData)
    , m_streamData(nullptr)
{
    if (xAudio2 && m_soundData)
    {
        xAudio2->CreateSourceVoice(&m_sourceVoice, &m_soundData->format, 0, 2.0f, this);
    }

    std::memset(&m_emitter, 0, sizeof(m_emitter));
    m_emitter.OrientFront = X3DAUDIO_VECTOR(0.0f, 0.0f, 1.0f);
    m_emitter.OrientTop = X3DAUDIO_VECTOR(0.0f, 1.0f, 0.0f);
    m_emitter.ChannelCount = 1;

    std::memset(&m_dspSettings, 0, sizeof(m_dspSettings));
    m_dspSettings.SrcChannelCount = 1;
    m_dspSettings.DstChannelCount = 2;
    m_dspSettings.pMatrixCoefficients = m_matrixCoefficients;
}

SoundPlayer::SoundPlayer(IXAudio2* xAudio2, const StreamingSoundData* streamData)
    : m_sourceVoice(nullptr)
    , m_soundData(nullptr)
    , m_streamData(streamData)
    , m_isStreaming(true)
    , m_streamTotalBytes(streamData ? streamData->dataChunkSize : 0)
{
    if (xAudio2 && m_streamData)
    {
        xAudio2->CreateSourceVoice(&m_sourceVoice, &m_streamData->format, 0, 2.0f, this);
    }

    for (int i = 0; i < STREAMING_BUFFER_COUNT; ++i)
    {
        m_streamBuffers[i].resize(STREAMING_BUFFER_SIZE);
        m_streamBufferSizes[i] = 0;
    }

    std::memset(&m_emitter, 0, sizeof(m_emitter));
    m_emitter.OrientFront = X3DAUDIO_VECTOR(0.0f, 0.0f, 1.0f);
    m_emitter.OrientTop = X3DAUDIO_VECTOR(0.0f, 1.0f, 0.0f);
    m_emitter.ChannelCount = 1;

    std::memset(&m_dspSettings, 0, sizeof(m_dspSettings));
    m_dspSettings.SrcChannelCount = 1;
    m_dspSettings.DstChannelCount = 2;
    m_dspSettings.pMatrixCoefficients = m_matrixCoefficients;
}

SoundPlayer::~SoundPlayer()
{
    if (m_streamFile.is_open())
    {
        m_streamFile.close();
    }
    if (m_sourceVoice)
    {
        m_sourceVoice->DestroyVoice();
        m_sourceVoice = nullptr;
    }
}

void SoundPlayer::Play(bool loop, double startTimeSeconds)
{
    if (!m_sourceVoice || !m_soundData)
    {
        return;
    }

    m_sourceVoice->Stop();
    m_sourceVoice->FlushSourceBuffers();

    const auto& format = m_soundData->format;
    const uint32_t align = std::max<uint16_t>(1, format.nBlockAlign);
    const uint32_t bytesPerSecond = ResolveBytesPerSecond(format);

    uint64_t startBytes = static_cast<uint64_t>(std::max<double>(0.0, static_cast<double>(startTimeSeconds)) * static_cast<double>(bytesPerSecond));
    startBytes -= startBytes % align;

    const uint64_t totalBytes = m_soundData->audioData.size();
    if (totalBytes == 0)
    {
        return;
    }

    if (startBytes >= totalBytes)
    {
        if (totalBytes > align)
        {
            startBytes = totalBytes - align;
            startBytes -= startBytes % align;
        }
        else
        {
            startBytes = 0;
        }
    }

    const size_t offset = static_cast<size_t>(startBytes);
    if (offset >= m_soundData->audioData.size())
    {
        return;
    }

    XAUDIO2_BUFFER buffer{};
    buffer.pAudioData = m_soundData->audioData.data() + offset;
    buffer.AudioBytes = static_cast<UINT32>(m_soundData->audioData.size() - offset);
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

    m_sourceVoice->SubmitSourceBuffer(&buffer);
    m_sourceVoice->Start();
}

void SoundPlayer::Stop()
{
    if (m_sourceVoice)
    {
        m_sourceVoice->Stop();
        m_sourceVoice->FlushSourceBuffers();
    }
}

bool SoundPlayer::IsPlaying() const
{
    if (!m_sourceVoice)
    {
        return false;
    }

    XAUDIO2_VOICE_STATE state;
    m_sourceVoice->GetState(&state);
    return state.BuffersQueued > 0;
}

void SoundPlayer::SetVolume(float volume)
{
    if (m_sourceVoice)
    {
        m_sourceVoice->SetVolume(volume);
    }
}

void SoundPlayer::SetPitch(float pitch)
{
    if (m_sourceVoice)
    {
        m_sourceVoice->SetFrequencyRatio(pitch);
    }
}

uint64_t SoundPlayer::GetSamplesPlayed() const
{
    if (!m_sourceVoice)
    {
        return 0ULL;
    }

    XAUDIO2_VOICE_STATE state;
    m_sourceVoice->GetState(&state, 0);
    return state.SamplesPlayed;
}

double SoundPlayer::GetPlaybackTimeSeconds() const
{
    const WAVEFORMATEX* format = GetFormat();
    if (!format || format->nSamplesPerSec == 0)
    {
        return 0.0;
    }
    return static_cast<double>(GetSamplesPlayed()) / format->nSamplesPerSec;
}

double SoundPlayer::GetPlaybackTimeMilliseconds() const
{
    return GetPlaybackTimeSeconds() * 1000.0;
}

void SoundPlayer::SetPosition(const Vector3& pos)
{
    m_position = pos;
    m_emitter.Position = X3DAUDIO_VECTOR(pos.x, pos.y, pos.z);
}

void SoundPlayer::SetVelocity(const Vector3& vel)
{
    m_velocity = vel;
    m_emitter.Velocity = X3DAUDIO_VECTOR(vel.x, vel.y, vel.z);
}

void SoundPlayer::Enable3D(bool enable)
{
    m_is3DEnabled = enable;
}

void SoundPlayer::Update3DAudio(const X3DAUDIO_LISTENER& listener)
{
    if (!m_sourceVoice || !m_is3DEnabled)
    {
        return;
    }

    X3DAudioCalculate(
        SoundManager::GetX3DInstance(),
        &listener,
        &m_emitter,
        X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER,
        &m_dspSettings);

    m_sourceVoice->SetOutputMatrix(
        nullptr,
        m_dspSettings.SrcChannelCount,
        m_dspSettings.DstChannelCount,
        m_dspSettings.pMatrixCoefficients);
    m_sourceVoice->SetFrequencyRatio(m_dspSettings.DopplerFactor);
}

void SoundPlayer::PlayStreaming(bool loop, double startTimeSeconds)
{
    if (!m_sourceVoice || !m_streamData)
    {
        return;
    }

    if (m_streamFile.is_open())
    {
        m_streamFile.close();
    }

    m_sourceVoice->Stop();
    m_sourceVoice->FlushSourceBuffers();

    m_streamFile.open(m_streamData->filename, std::ios::binary);
    if (!m_streamFile)
    {
        return;
    }

    const auto& format = m_streamData->format;
    const uint32_t align = std::max<uint16_t>(1, format.nBlockAlign);
    const uint32_t bytesPerSecond = ResolveBytesPerSecond(format);

    uint64_t startBytes = static_cast<uint64_t>(std::max<double>(0.0, static_cast<double>(startTimeSeconds)) * static_cast<double>(bytesPerSecond));
    startBytes -= startBytes % align;
    const uint32_t chunkSize = m_streamData->dataChunkSize;
    if (chunkSize == 0)
    {
        return;
    }

    if (startBytes >= chunkSize)
    {
        if (chunkSize > align)
        {
            startBytes = chunkSize - align;
            startBytes -= startBytes % align;
        }
        else
        {
            startBytes = 0;
        }
    }

    m_streamStartOffsetBytes = static_cast<uint32_t>(startBytes);
    m_streamTotalBytes = chunkSize - m_streamStartOffsetBytes;
    if (m_streamTotalBytes == 0)
    {
        return;
    }

    m_streamFile.clear();
    m_streamFile.seekg(m_streamData->dataChunkOffset + m_streamStartOffsetBytes, std::ios::beg);
    if (!m_streamFile)
    {
        return;
    }

    m_streamPosition = 0;
    m_streamEnded = false;
    m_isLooping = loop;
    m_currentBufferIndex = 0;

    for (int i = 0; i < STREAMING_BUFFER_COUNT; ++i)
    {
        if (LoadNextStreamBuffer(i))
        {
            SubmitStreamBuffer(i);
        }
    }

    m_sourceVoice->Start();
}

void SoundPlayer::UpdateStreaming()
{
    if (!m_isStreaming || !m_sourceVoice)
    {
        return;
    }

    XAUDIO2_VOICE_STATE state;
    m_sourceVoice->GetState(&state);

    if (state.BuffersQueued < STREAMING_BUFFER_COUNT && !m_streamEnded)
    {
        if (LoadNextStreamBuffer(m_currentBufferIndex))
        {
            SubmitStreamBuffer(m_currentBufferIndex);
            m_currentBufferIndex = (m_currentBufferIndex + 1) % STREAMING_BUFFER_COUNT;
        }
    }
}

bool SoundPlayer::LoadNextStreamBuffer(int bufferIndex)
{
    if (!m_streamFile.is_open() || m_streamEnded || m_streamTotalBytes == 0)
    {
        return false;
    }

    uint32_t remainingBytes = (m_streamTotalBytes > m_streamPosition)
        ? (m_streamTotalBytes - m_streamPosition)
        : 0;

    if (remainingBytes == 0)
    {
        if (m_isLooping)
        {
            m_streamFile.clear();
            m_streamFile.seekg(m_streamData->dataChunkOffset + m_streamStartOffsetBytes, std::ios::beg);
            m_streamPosition = 0;
            remainingBytes = m_streamTotalBytes;
        }
        else
        {
            m_streamEnded = true;
            return false;
        }
    }

    uint32_t bytesToRead = std::min<uint32_t>(STREAMING_BUFFER_SIZE, remainingBytes);
    m_streamFile.read(reinterpret_cast<char*>(m_streamBuffers[bufferIndex].data()), bytesToRead);
    std::streamsize bytesReadStream = m_streamFile.gcount();

    if (bytesReadStream <= 0 || bytesReadStream > static_cast<std::streamsize>(bytesToRead))
    {
        m_streamEnded = true;
        return false;
    }

    uint32_t bytesRead = static_cast<uint32_t>(bytesReadStream);
    m_streamPosition += bytesRead;
    m_streamBufferSizes[bufferIndex] = bytesRead;

    return true;
}

const WAVEFORMATEX* SoundPlayer::GetFormat() const
{
    if (m_soundData)
    {
        return &m_soundData->format;
    }
    if (m_streamData)
    {
        return &m_streamData->format;
    }
    return nullptr;
}

void SoundPlayer::SubmitStreamBuffer(int bufferIndex)
{
    if (!m_sourceVoice)
    {
        return;
    }

    XAUDIO2_BUFFER buffer{};
    buffer.pAudioData = m_streamBuffers[bufferIndex].data();
    buffer.AudioBytes = m_streamBufferSizes[bufferIndex];

    if (!m_isLooping && m_streamTotalBytes > 0 && m_streamPosition >= m_streamTotalBytes)
    {
        buffer.Flags = XAUDIO2_END_OF_STREAM;
    }

    m_sourceVoice->SubmitSourceBuffer(&buffer);
}

void SoundPlayer::OnBufferEnd(void*)
{
}

void SoundPlayer::OnStreamEnd()
{
    m_streamEnded = true;
}

void SoundPlayer::Pause()
{
    if (m_sourceVoice)
    {
        m_sourceVoice->Stop();
    }
}

void SoundPlayer::Resume()
{
    if (m_sourceVoice)
    {
        m_sourceVoice->Start();
    }
}