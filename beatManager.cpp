#include "beatManager.h"
#include "soundManager.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr double kFixedStepMs = 5.0;
    constexpr double kMaxDeltaMs  = 100.0;
}

BeatManager* BeatManager::s_instance = nullptr;

void BeatManager::Init()
{
    s_instance = this;
    m_lastUpdateTime = Clock::now();
    m_currentTimeMs = 0.0;
    m_accumulatorMs = 0.0;
    m_hasAudioLock = false;
    m_seekOffsetMs = 0.0;
}

void BeatManager::Uninit()
{
    if (s_instance == this)
    {
        s_instance = nullptr;
    }
}

void BeatManager::Update()
{
    GameObject::Update();

    const auto now = Clock::now();
    double deltaMs = std::chrono::duration<double, std::milli>(now - m_lastUpdateTime).count();
    m_lastUpdateTime = now;

    deltaMs = std::min(deltaMs, kMaxDeltaMs);

    const double audioTime = QueryAudioTimeMilliseconds();
    if (audioTime >= 0.0)
    {
        m_currentTimeMs = std::max(0.0, audioTime + m_manualOffsetMs);
        m_accumulatorMs = 0.0;
        m_hasAudioLock = true;
        return;
    }

    m_accumulatorMs += deltaMs;
    while (m_accumulatorMs >= kFixedStepMs)
    {
        m_currentTimeMs = std::max(0.0, m_currentTimeMs + kFixedStepMs);
        m_accumulatorMs -= kFixedStepMs;
    }
    m_hasAudioLock = false;
}

void BeatManager::SetBPM(int bpm)
{
    m_bpm = std::max(1, bpm);
}

void BeatManager::SetTrackedSoundKey(const std::string& key, bool searchStreaming)
{
    m_trackedSoundKey = key;
    m_searchStreaming = searchStreaming;
    ResetTimeline();
}

void BeatManager::SetManualOffsetMilliseconds(double offsetMs)
{
    m_manualOffsetMs = offsetMs;
}

double BeatManager::GetBeatIntervalMilliseconds() const
{
    if (m_bpm <= 0)
    {
        return 0.0;
    }
    return 60000.0 / static_cast<double>(m_bpm);
}

double BeatManager::GetBeatsSinceStart() const
{
    const double interval = GetBeatIntervalMilliseconds();
    return interval > 0.0 ? m_currentTimeMs / interval : 0.0;
}

uint64_t BeatManager::GetCurrentBeatIndex() const
{
    return static_cast<uint64_t>(GetBeatsSinceStart());
}

double BeatManager::GetBeatPhase() const
{
    double integral = 0.0;
    const double fractional = std::modf(GetBeatsSinceStart(), &integral);
    return fractional < 0.0 ? fractional + 1.0 : fractional;
}

double BeatManager::GetTimeUntilNextBeatMilliseconds() const
{
    const double interval = GetBeatIntervalMilliseconds();
    if (interval <= 0.0)
    {
        return 0.0;
    }

    double remainder = std::fmod(m_currentTimeMs, interval);
    if (remainder < 0.0)
    {
        remainder += interval;
    }
    return interval - remainder;
}

void BeatManager::ForceResetTimeline()
{
    ResetTimeline();
}

void BeatManager::ResetTimeline()
{
    m_currentTimeMs = 0.0;
    m_lastUpdateTime = Clock::now();
    m_hasAudioLock = false;
    m_seekOffsetMs = 0.0;
}

double BeatManager::QueryAudioTimeMilliseconds() const
{
    if (m_trackedSoundKey.empty())
    {
        return -1.0;
    }

    const double playbackMs = SoundManager::GetPlaybackTimeMilliseconds(m_trackedSoundKey, m_searchStreaming);
    const bool active = SoundManager::IsSoundActive(m_trackedSoundKey, m_searchStreaming);

    if (!active && playbackMs <= 0.0)
    {
        return -1.0;
    }

    return playbackMs + m_seekOffsetMs;
}
