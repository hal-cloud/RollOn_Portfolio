#pragma once
#include <xaudio2.h>
#include <X3DAudio.h>
#include <memory>
#include <fstream>
#include <vector>
#include <cstdint>
#include "soundLoader.h"
#include "vector3.h"

class SoundPlayer : public IXAudio2VoiceCallback
{
public:
    SoundPlayer(IXAudio2* xAudio2, const SoundData* soundData);
    SoundPlayer(IXAudio2* xAudio2, const StreamingSoundData* streamData);
    ~SoundPlayer();

    void Play(bool loop = false, double startTimeSeconds = 0.0);
    void PlayStreaming(bool loop = false, double startTimeSeconds = 0.0);
    void Stop();
    void Pause();
    void Resume();
    bool IsPlaying() const;
    void UpdateStreaming();

    void SetVolume(float volume);
    void SetPitch(float pitch);

    uint64_t GetSamplesPlayed() const;
    double GetPlaybackTimeSeconds() const;
    double GetPlaybackTimeMilliseconds() const;

    // 3Dçƒê∂óp
    void SetPosition(const Vector3& pos);
    void SetVelocity(const Vector3& vel);
    void Enable3D(bool enable);
    bool Is3DEnabled() const { return m_is3DEnabled; }
    void Update3DAudio(const X3DAUDIO_LISTENER& listener);

    void OnBufferEnd(void* pBufferContext) override;
    void OnVoiceProcessingPassStart(UINT32) override {}
    void OnVoiceProcessingPassEnd() override {}
    void OnStreamEnd() override;
    void OnBufferStart(void*) override {}
    void OnLoopEnd(void*) override {}
    void OnVoiceError(void*, HRESULT) override {}

private:
    IXAudio2SourceVoice* m_sourceVoice = nullptr;
    const SoundData* m_soundData = nullptr;
    const StreamingSoundData* m_streamData = nullptr;
    bool m_is3DEnabled = false;
    bool m_isStreaming = false;
    bool m_isLooping = false;
    bool m_streamEnded = false;

    static const int STREAMING_BUFFER_COUNT = 3;
    static const int STREAMING_BUFFER_SIZE = 65536;
    std::vector<BYTE> m_streamBuffers[STREAMING_BUFFER_COUNT];
    uint32_t m_streamBufferSizes[STREAMING_BUFFER_COUNT]{};
    std::ifstream m_streamFile;
    uint32_t m_streamPosition = 0;
    uint32_t m_currentBufferIndex = 0;
    uint32_t m_streamStartOffsetBytes = 0;
    uint32_t m_streamTotalBytes = 0;

    X3DAUDIO_EMITTER m_emitter{};
    X3DAUDIO_DSP_SETTINGS m_dspSettings{};
    float m_matrixCoefficients[8]{};
    Vector3 m_position{};
    Vector3 m_velocity{};

    const WAVEFORMATEX* GetFormat() const;
    bool LoadNextStreamBuffer(int bufferIndex);
    void SubmitStreamBuffer(int bufferIndex);
};