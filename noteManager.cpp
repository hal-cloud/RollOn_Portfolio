#include "noteManager.h"

#include <algorithm>
#include <memory>
#include <cmath>
#include "manager.h"
#include "scene.h"
#include "note.h"
#include "beatManager.h"
#include "score.h"
#include "gameScore.h"
#include "gameSettings.h"

namespace
{
    constexpr double kBeatResetEpsilon = 0.01;
    constexpr double kChordDetectEpsilon = 1e-6;
    constexpr double kBeatQuantizeScale = 1000.0;

    constexpr float kNoteStartZ = 15.0f;
    constexpr float kNoteTargetZ = 0.0f;
    constexpr float kBaseTravelSec = 0.5f;

    constexpr int kNotePoolSize = 64;

    struct ChordColor { Vector3 color; float gamma; };
    constexpr int kMaxChordColors = 5;
    const ChordColor kChordColors[kMaxChordColors] = {
        { Vector3(0.0f, 0.0f, 0.0f), 1.0f },
        { Vector3(0.3f, 0.5f, 0.7f), 1.0f },
        { Vector3(0.8f, 0.7f, 0.2f), 1.0f },
        { Vector3(0.2f, 0.8f, 1.0f), 1.0f },
        { Vector3(0.6f, 0.5f, 0.8f), 1.0f },
    };

    inline int64_t QuantizeBeat(double beat)
    {
        return static_cast<int64_t>(std::llround(beat * kBeatQuantizeScale));
    }
}

float NoteManager::GetEffectiveTravelSec() const
{
    const float speed = GameSettings::GetNoteSpeed();
    return kBaseTravelSec / std::max(0.25f, speed);
}

void NoteManager::Init(const char* filePath)
{
    SetSpawnLeadBeats(3.0);
    SetSongStartOffsetBeats(4.0);
    BuildDefaultScores(filePath);
    ReloadScores();

    auto* scene = Manager::GetScene();
    if (scene)
    {
        scene->PrewarmPool<Note>(LAYER_WORLD_EFFECT, kNotePoolSize, NoteType::LOW);
    }
}

void NoteManager::Uninit()
{
    m_timeline.clear();
    m_hitTimesCache.clear();
    m_chordBeatKeys.clear();
    m_chordBeatCounts.clear();
    m_spawnCursor = 0;
    m_lastObservedBeat = 0.0;
    m_ready = false;
    m_beatManagerCache = nullptr;
    m_beatManagerResolved = false;
}

void NoteManager::ReloadScores()
{
    m_timeline = m_scoreReader.BuildUnifiedNotes();

    for (auto& note : m_timeline)
    {
        const int64_t key = QuantizeBeat(note.beatPosition);
        note.beatPosition = static_cast<double>(key) / kBeatQuantizeScale;
    }

    m_chordBeatKeys.clear();
    m_chordBeatCounts.clear();

    for (size_t i = 0; i < m_timeline.size(); ++i)
    {
        const int64_t key = QuantizeBeat(m_timeline[i].beatPosition);
        m_chordBeatCounts[key]++;
        if (i + 1 < m_timeline.size())
        {
            const double diff = std::fabs(m_timeline[i + 1].beatPosition - m_timeline[i].beatPosition);
            if (diff <= kChordDetectEpsilon)
            {
                m_chordBeatKeys.insert(key);
                m_chordBeatKeys.insert(QuantizeBeat(m_timeline[i + 1].beatPosition));
            }
        }
    }

    m_hitTimesCache.clear();

    GameScore::Instance().SetExpectedTotalNotes(static_cast<int>(m_timeline.size()));
    m_spawnCursor = 0;
    m_lastObservedBeat = 0.0;
    m_ready = !m_timeline.empty();
    m_beatManagerCache = nullptr;
    m_beatManagerResolved = false;
}

void NoteManager::SetSpawnLeadBeats(double beats)
{
    m_spawnLeadBeats = std::max(0.0, beats);
}

void NoteManager::Update()
{
    GameObject::Update();
    if (!m_ready) return;

    BeatManager* beatManager = ResolveBeatManager();
    if (!beatManager) return;

    const double currentTimeSec = beatManager->GetCurrentTimeSeconds();
    const double bpm = static_cast<double>(beatManager->GetBPM());
    const double beatInterval = 60.0 / bpm;

    if (m_hitTimesCache.empty() && !m_timeline.empty())
    {
        m_hitTimesCache.resize(m_timeline.size());
        for (size_t i = 0; i < m_timeline.size(); ++i)
        {
            m_hitTimesCache[i] = (m_timeline[i].beatPosition + m_songStartOffsetBeats) * beatInterval;
        }
    }

    const double currentBeat = currentTimeSec / beatInterval;
    const double effectiveBeat = currentBeat - m_songStartOffsetBeats;

    if (effectiveBeat + kBeatResetEpsilon < m_lastObservedBeat)
    {
        m_spawnCursor = 0;
    }
    m_lastObservedBeat = effectiveBeat;

    const float travelSec = GetEffectiveTravelSec();

    while (m_spawnCursor < m_timeline.size())
    {
        const double noteHitTimeSec = m_hitTimesCache[m_spawnCursor];
        const double spawnTimeSec = noteHitTimeSec - static_cast<double>(travelSec);

        if (currentTimeSec >= spawnTimeSec)
        {
            const double spawnDelaySec = currentTimeSec - spawnTimeSec;
            SpawnNote(m_timeline[m_spawnCursor], noteHitTimeSec, spawnDelaySec);
            ++m_spawnCursor;
        }
        else
        {
            break;
        }
    }
}

void NoteManager::BuildDefaultScores(const char* filePath)
{
    m_scoreReader.Clear();
    m_scoreReader.RegisterScore(std::make_shared<LowScore>(filePath));
    m_scoreReader.RegisterScore(std::make_shared<MidScore>(filePath));
    m_scoreReader.RegisterScore(std::make_shared<HighScore>(filePath));
    m_scoreReader.RegisterScore(std::make_shared<CrashScore>(filePath));
}

void NoteManager::SpawnNote(const UnifiedNote& noteData, double hitTimeSec, double spawnDelaySec) const
{
    auto* scene = Manager::GetScene();
    if (!scene) return;

    Note* n = scene->FindInactive<Note>(LAYER_WORLD_EFFECT);
    if (n)
    {
        n->Reset();
        n->Reinit(noteData.lane);
    }
    else
    {
        n = scene->AddGameObject<Note>(LAYER_WORLD_EFFECT, noteData.lane);
    }
    if (!n) return;

    const float travelSec = GetEffectiveTravelSec();
    const float noteSpeed = (kNoteStartZ - kNoteTargetZ) / travelSec;

    n->SetHitTime(static_cast<float>(hitTimeSec));
    n->SetTargetZ(kNoteTargetZ);
    n->SetTravelTime(travelSec);

    if (spawnDelaySec > 0.0)
    {
        Vector3 pos = n->GetNotePosition();
        pos.z -= noteSpeed * static_cast<float>(spawnDelaySec);
        n->SetNotePosition(pos);
    }

    const int chordCount = GetChordCount(noteData.beatPosition);
    if (chordCount > 0 && chordCount < kMaxChordColors)
    {
        const auto& cc = kChordColors[chordCount];
        n->SetColor(cc.color, cc.gamma);
    }
}

BeatManager* NoteManager::ResolveBeatManager()
{
    if (!m_beatManagerResolved)
    {
        if (auto* scene = Manager::GetScene())
        {
            m_beatManagerCache = scene->GetGameObject<BeatManager>();
        }
        m_beatManagerResolved = true;
    }
    return m_beatManagerCache;
}

bool NoteManager::IsSimultaneous(double beatPosition) const
{
    return m_chordBeatKeys.find(QuantizeBeat(beatPosition)) != m_chordBeatKeys.end();
}

int NoteManager::GetChordCount(double beatPosition) const
{
    const int64_t key = QuantizeBeat(beatPosition);
    auto it = m_chordBeatCounts.find(key);
    return (it != m_chordBeatCounts.end()) ? it->second : 1;
}