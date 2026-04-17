#pragma once

#include <string>
#include <chrono>
#include <cstdint>
#include "gameObject.h"

class BeatManager : public GameObject
{
public:
    BeatManager() = default;

    void Init();
    void Uninit() override;
    void Update() override;

    static BeatManager* GetInstance() { return s_instance; }

    void SetBPM(int bpm);
    int GetBPM() const { return m_bpm; }

    void SetTrackedSoundKey(const std::string& key, bool searchStreaming = true);
    const std::string& GetTrackedSoundKey() const { return m_trackedSoundKey; }

    void SetManualOffsetMilliseconds(double offsetMs);
    double GetManualOffsetMilliseconds() const { return m_manualOffsetMs; }

    double GetCurrentTimeMilliseconds() const { return m_currentTimeMs; }
    double GetCurrentTimeSeconds() const { return m_currentTimeMs * 0.001; }

    double GetBeatIntervalMilliseconds() const;
    double GetBeatsSinceStart() const;

    uint64_t GetCurrentBeatIndex() const;

    double GetBeatPhase() const;
    double GetTimeUntilNextBeatMilliseconds() const;

    bool HasAudioLock() const { return m_hasAudioLock; }

    void ForceResetTimeline();

private:
    using Clock = std::chrono::steady_clock;

    static BeatManager* s_instance;

    std::string m_trackedSoundKey;
    bool m_searchStreaming = true;

    int m_bpm = 120;
    double m_manualOffsetMs = 0.0;
    double m_currentTimeMs = 0.0;
    double m_accumulatorMs{ 0.0 };
    bool m_hasAudioLock = false;
    double m_seekOffsetMs = 0.0;

    Clock::time_point m_lastUpdateTime{};

    void ResetTimeline();
    double QueryAudioTimeMilliseconds() const;
};
