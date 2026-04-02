#include "manager.h"
#include "scene.h"
#include "input.h"
#include "tapEffect.h"
#include "laneEffect.h"
#include "field.h"
#include "lane.h"
#include "playerInput.h"
#include "soundManager.h"
#include "note.h"
#include "beatManager.h"
#include "judgeUtil.h"
#include "gameScore.h"
#include "judgeEffect.h"
#include "songScene.h"

#include <limits>
#include <deque>
#include <array>
#include <vector>

#include "particleInstance.h"
#include "judgeSystem.h"

namespace
{
    constexpr double kInputBufferSec = 0.05;

    int LaneToIndex(LaneType lane)
    {
        switch (lane)
        {
        case LaneType::LOW:   return 0;
        case LaneType::MID:   return 1;
        case LaneType::HIGH:  return 2;
        case LaneType::CRASH: return 3;
        default:              return 0;
        }
    }
}

void PlayerInput::Init() {
    SoundManager::Load("Tap", L"assets\\sound\\SE_Tap.wav");

}

void PlayerInput::Update() {
    struct KeyBind {
        std::initializer_list<int> keys;
        LaneType lane;
    };

    static const std::array<KeyBind, 4> keyBinds = { {
        { { 'D', 'D' }, LaneType::LOW,   },
        { { 'F', 'F' }, LaneType::MID,   },
        { { 'J', 'J' }, LaneType::HIGH,  },
        { { 'K', 'K' }, LaneType::CRASH, }
    } };

    auto* scene = Manager::GetScene();
    if (!scene) {
        m_cachedScene = nullptr;
        m_cachedSongScene = nullptr;
        return;
    }

    if (scene != m_cachedScene) {
        m_cachedScene = scene;
        m_cachedSongScene = dynamic_cast<SongScene*>(scene);
    }

    auto playTapSe = [&](float vol)
    {
        if (m_cachedSongScene && m_isAutoPlay)
        {
            m_cachedSongScene->RequestTapSe(vol);
        }
        else
        {
            SoundManager::Play("Tap", false, vol);
        }
    };

    BeatManager* beat = scene->GetGameObject<BeatManager>();
    const double now = beat ? beat->GetCurrentTimeSeconds() : (m_fallbackTimeSec += FRAME_TIME);

    const auto notesByLane = JudgeSystem::CollectNotesByLane(scene);

    for (const auto& bind : keyBinds) {
        bool triggered = false;
        for (int key : bind.keys) {
            if (Input::GetKeyTrigger(key)) {
                triggered = true;
                break;
            }
        }
        if (triggered) {
            m_inputBuffer[LaneToIndex(bind.lane)].push_back(now);
        }
    }

    if (m_isAutoPlay) {
        Lane* laneObj = scene->GetGameObject<Lane>();
        if (!laneObj) return;

        JudgeSystem::HandleAutoPlay(scene, laneObj, now, notesByLane, playTapSe);
        return;
    }

    Lane* laneObj = scene->GetGameObject<Lane>();
    if (!laneObj) return;

    auto consumeBufferedInput = [&](LaneType laneType) -> bool
    {
        auto& q = m_inputBuffer[LaneToIndex(laneType)];
        while (!q.empty() && (now - q.front()) > kInputBufferSec) {
            q.pop_front();
        }
        if (!q.empty()) {
            q.pop_front();
            return true;
        }
        return false;
    };

    constexpr std::array<LaneType, 4> lanes = {
        LaneType::LOW, LaneType::MID, LaneType::HIGH, LaneType::CRASH
    };
    for (auto laneType : lanes) {
        if (consumeBufferedInput(laneType)) {
            const bool judged = JudgeSystem::HandleLaneInput(scene, laneObj, laneType, now, notesByLane, playTapSe);
            if (!judged) {
                SoundManager::Play_RandVP("Tap", false, 0.2f, 0.21f, 0.19f);
            }
        }
    }
}