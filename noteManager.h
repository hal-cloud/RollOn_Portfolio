#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include "gameObject.h"
#include "scoreReader.h"

class BeatManager;
class Note;

class NoteManager : public GameObject
{
public:
    void Init(const char* filePath);
    void Uninit() override;
    void Update() override;
    void Draw() override {}

    void ReloadScores();
    void SetSpawnLeadBeats(double beats);
    void SetSongStartOffsetBeats(double beats) { m_songStartOffsetBeats = std::max(0.0, beats); }

private:
    ScoreReader m_scoreReader;
    std::vector<UnifiedNote> m_timeline;

    std::vector<double> m_hitTimesCache;

    size_t m_spawnCursor = 0;
    double m_spawnLeadBeats = 0.0;
    double m_songStartOffsetBeats = 0.0;
    double m_lastObservedBeat = 0.0;
    bool m_ready = false;

    BeatManager* m_beatManagerCache = nullptr;
    bool m_beatManagerResolved = false;

    std::unordered_set<int64_t> m_chordBeatKeys;
    std::unordered_map<int64_t, int> m_chordBeatCounts;

    float GetEffectiveTravelSec() const;

    void BuildDefaultScores(const char* filePath);
    void SpawnNote(const UnifiedNote& noteData, double hitTimeSec, double spawnDelaySec = 0.0) const;
    BeatManager* ResolveBeatManager();
    bool IsSimultaneous(double beatPosition) const;
    int  GetChordCount(double beatPosition) const;
};