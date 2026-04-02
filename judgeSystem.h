#pragma once
#include <array>
#include <vector>
#include <functional>
#include <limits>
#include <cmath>

#include "scene.h"
#include "lane.h"
#include "note.h"
#include "judgeUtil.h"
#include "gameScore.h"
#include "judgeEffect.h"
#include "particleInstance.h"
#include "tapEffect.h"
#include "laneEffect.h"

class JudgeSystem
{
public:
    using NotesByLane = std::array<std::vector<Note*>, 4>;

    static const JudgeWindow& GetJudgeWindow()
    {
        static const JudgeWindow window{ 0.045f, 0.06f, 0.075f };
        return window;
    }

    static NotesByLane CollectNotesByLane(Scene* scene)
    {
        NotesByLane notesByLane{};
        if (!scene) return notesByLane;

        const auto allNotes = scene->GetGameObjects<Note>();
        for (auto* n : allNotes)
        {
            if (!n || n->IsJudged()) continue;
            const int idx = NoteTypeToIndex(n->GetType());
            notesByLane[idx].push_back(n);
        }
        return notesByLane;
    }

    static void HandleAutoPlay(Scene* scene,
        Lane* laneObj,
        double now,
        const NotesByLane& notesByLane,
        const std::function<void(float)>& playTapSe)
    {
        if (!scene || !laneObj) return;

        constexpr std::array<LaneType, 4> lanes = {
            LaneType::LOW, LaneType::MID, LaneType::HIGH, LaneType::CRASH
        };
        const auto& judgeWindow = GetJudgeWindow();

        for (auto laneType : lanes) {
            Field* lane = laneObj->GetLane(laneType);
            if (!lane) continue;

            Note* best = nullptr;
            float bestDiff = std::numeric_limits<float>::max();
            const int laneIdx = LaneToIndex(laneType);

            for (auto* n : notesByLane[laneIdx]) {
                if (!n) continue;

                const float diff = static_cast<float>(now - n->GetHitTime());
                const float ad = std::fabs(diff);

                if (ad > judgeWindow.perfect * 0.4f) continue;

                if (ad < bestDiff) {
                    bestDiff = ad;
                    best = n;
                }
            }

            if (!best) continue;

            const float vol = 0.8f;
            GameScore::Instance().AddJudgePerfect(kPerfectPoint);
            SpawnJudgeEffect(scene, L"assets\\texture\\J_Perfect.png");

            if (playTapSe) playTapSe(vol);

            if (auto* p = scene->AddGameObject<ParticleInstance>(LAYER_WORLD_EFFECT)) {
                p->SetType(ParticleType::Perfect);
                p->SetPosition(lane->GetPosition());
            }

            best->MarkJudged();
            best->ReturnToPool();

            const auto& pos = lane->GetPosition();
            SpawnTapEffect(scene, pos);
            SpawnLaneEffect(scene, pos, lane->GetWidth(), lane->GetHeight() / 2, lane->GetColor());
        }
    }

    static bool HandleLaneInput(Scene* scene,
        Lane* laneObj,
        LaneType laneType,
        double now,
        const NotesByLane& notesByLane,
        const std::function<void(float)>& playTapSe)
    {
        if (!scene || !laneObj) return false;

        Field* lane = laneObj->GetLane(laneType);
        if (!lane) return false;

        Note* best = nullptr;
        float bestDiff = std::numeric_limits<float>::max();
        const int laneIdx = LaneToIndex(laneType);
        const auto& judgeWindow = GetJudgeWindow();

        for (auto* n : notesByLane[laneIdx]) {
            if (!n) continue;
            float diff = static_cast<float>(now - n->GetHitTime());
            float ad = std::fabs(diff);

            if (ad > judgeWindow.miss) continue;

            if (ad < bestDiff) {
                bestDiff = ad;
                best = n;
            }
        }

        const auto& pos = lane->GetPosition();
        SpawnTapEffect(scene, pos);
        SpawnLaneEffect(scene, pos, lane->GetWidth(), lane->GetHeight() / 2, lane->GetColor());

        if (!best) return false;

        JudgeType jt = JudgeByTimeDiff(static_cast<float>(now - best->GetHitTime()), judgeWindow);
        float vol = 0.8f;
        switch (jt) {
        case JudgeType::Perfect:
            GameScore::Instance().AddJudgePerfect(kPerfectPoint);
            SpawnJudgeEffect(scene, L"assets\\texture\\J_Perfect.png");
            if (playTapSe) playTapSe(vol);
            break;
        case JudgeType::Good:
            GameScore::Instance().AddJudgeGood(kGoodPoint);
            SpawnJudgeEffect(scene, L"assets\\texture\\J_Good.png");
            if (playTapSe) playTapSe(vol);
            break;
        case JudgeType::Miss:
            GameScore::Instance().AddMiss();
            SpawnJudgeEffect(scene, L"assets\\texture\\J_Miss.png");
            if (playTapSe) playTapSe(vol);
            break;
        }
        if (auto* p = scene->AddGameObject<ParticleInstance>(LAYER_WORLD_EFFECT)) {
            p->SetType(ToParticle(jt));
            p->SetPosition(lane->GetPosition());
        }

        best->MarkJudged();
        best->ReturnToPool();

        return true;
    }

private:
    static constexpr int kPerfectPoint = 1000;
    static constexpr int kGoodPoint = 500;

    static void SpawnJudgeEffect(Scene* scene, const wchar_t* texturePath)
    {
        if (!scene) return;
        for (auto* eff : scene->GetGameObjects<JudgeEffect>()) {
            if (eff) eff->SetDestroy();
        }
        scene->AddGameObject<JudgeEffect>(
            LAYER_UI,
            SCREEN_WIDTH / 2,
            SCREEN_HEIGHT * 0.7f,
            350, 200,
            const_cast<wchar_t*>(texturePath));
    }

    static ParticleType ToParticle(JudgeType jt)
    {
        switch (jt)
        {
        case JudgeType::Perfect: return ParticleType::Perfect;
        case JudgeType::Good:    return ParticleType::Good;
        case JudgeType::Miss:    return ParticleType::Miss;
        default:                 return ParticleType::Default;
        }
    }

    static NoteType LaneToNote(LaneType lane)
    {
        switch (lane)
        {
        case LaneType::LOW:   return NoteType::LOW;
        case LaneType::MID:   return NoteType::MID;
        case LaneType::HIGH:  return NoteType::HIGH;
        case LaneType::CRASH: return NoteType::CRASH;
        default:              return NoteType::LOW;
        }
    }

    static int LaneToIndex(LaneType lane)
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

    static int NoteTypeToIndex(NoteType type)
    {
        switch (type)
        {
        case NoteType::LOW:   return 0;
        case NoteType::MID:   return 1;
        case NoteType::HIGH:  return 2;
        case NoteType::CRASH: return 3;
        default:              return 0;
        }
    }

    static void SpawnTapEffect(Scene* scene, const Vector3& pos)
    {
        TapEffect* e = scene->FindInactive<TapEffect>(LAYER_WORLD_EFFECT);
        if (e) { e->Reset(); e->Reinit(pos); }
        else   { scene->AddGameObject<TapEffect>(LAYER_WORLD_EFFECT, pos); }
    }

    static void SpawnLaneEffect(Scene* scene, const Vector3& pos, float width, float height, const Vector3& color)
    {
        LaneEffect* e = scene->FindInactive<LaneEffect>(LAYER_WORLD_EFFECT);
        if (e) { e->Reset(); e->Reinit(pos, width, height, color); }
        else   { scene->AddGameObject<LaneEffect>(LAYER_WORLD_EFFECT, pos, width, height, color); }
    }
};
