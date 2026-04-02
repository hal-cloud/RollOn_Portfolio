#include "note.h"
#include "manager.h"
#include "scene.h"
#include "field.h"
#include "lane.h"
#include "beatManager.h"
#include "gameScore.h"
#include "particleInstance.h"
#include <algorithm>
#include "judgeEffect.h"
#include "judgeSystem.h"

void Note::Init(NoteType type)
{
    m_type = type;
    m_targetZ = 0.0f;
    m_judged = false;
    m_beatCache = nullptr;
    m_beatCacheResolved = false;

    auto* scene = Manager::GetScene();
    if (!scene) return;

    m_noteVisual = scene->AddGameObject<Field>(
        LAYER_WORLD_EFFECT, Vector3(0, 0.05f, 10.0f), 2.0f, 0.5f, (wchar_t*)L"assets\\texture\\white.png");
    if (!m_noteVisual) return;

    auto* laneObj = scene->GetGameObject<Lane>();
    if (!laneObj) { SetDestroy(); return; }

    LaneType laneType = LaneType::LOW;
    switch (type)
    {
    case NoteType::LOW:   laneType = LaneType::LOW;   break;
    case NoteType::MID:   laneType = LaneType::MID;   break;
    case NoteType::HIGH:  laneType = LaneType::HIGH;  break;
    case NoteType::CRASH: laneType = LaneType::CRASH; break;
    default: break;
    }

    Field* lane = laneObj->GetLane(laneType);
    if (!lane) { SetDestroy(); return; }

    constexpr float kStartZ = 15.0f;
    constexpr float kStartY = 0.05f;
    m_startZ = kStartZ;

    m_noteVisual->SetPosition(lane->GetPosition() + Vector3(0, kStartY, kStartZ));
}

void Note::Reset()
{
    GameObject::Reset();
    m_targetZ = 0.0f;
    m_hitTime = 0.0f;
    m_travelTime = 1.0f;
    m_startZ = 15.0f;
    m_type = NoteType::LOW;
    m_judged = false;
    m_beatCache = nullptr;
    m_beatCacheResolved = false;
}

void Note::Reinit(NoteType type)
{
    m_type = type;
    m_targetZ = 0.0f;
    m_judged = false;
    m_beatCache = nullptr;
    m_beatCacheResolved = false;

    auto* scene = Manager::GetScene();
    if (!scene) return;

    if (!m_noteVisual)
    {
        m_noteVisual = scene->AddGameObject<Field>(
            LAYER_WORLD_EFFECT, Vector3(0, 0.05f, 10.0f), 2.0f, 0.5f, (wchar_t*)L"assets\\texture\\white.png");
    }
    if (!m_noteVisual) return;

    m_noteVisual->Activate();

    auto* laneObj = scene->GetGameObject<Lane>();
    if (!laneObj) { ReturnToPool(); return; }

    LaneType laneType = LaneType::LOW;
    switch (type)
    {
    case NoteType::LOW:   laneType = LaneType::LOW;   break;
    case NoteType::MID:   laneType = LaneType::MID;   break;
    case NoteType::HIGH:  laneType = LaneType::HIGH;  break;
    case NoteType::CRASH: laneType = LaneType::CRASH; break;
    default: break;
    }

    Field* lane = laneObj->GetLane(laneType);
    if (!lane) { ReturnToPool(); return; }

    constexpr float kStartZ = 15.0f;
    constexpr float kStartY = 0.05f;
    m_startZ = kStartZ;

    m_noteVisual->SetPosition(lane->GetPosition() + Vector3(0, kStartY, kStartZ));
    m_noteVisual->SetColor(Vector3(1.0f, 1.0f, 1.0f));
    m_noteVisual->SetGamma(1.0f);

}

void Note::ReturnToPool()
{
    if (m_noteVisual)
    {
        m_noteVisual->Deactivate();
    }
    // Note自身を非アクティブにする
    Deactivate();
}

void Note::Uninit()
{
    if (m_noteVisual)
    {
        m_noteVisual->SetDestroy();
        m_noteVisual = nullptr;
    }
}

BeatManager* Note::GetBeatManager()
{
    if (!m_beatCacheResolved)
    {
        if (auto* scene = Manager::GetScene())
        {
            m_beatCache = scene->GetGameObject<BeatManager>();
        }
        m_beatCacheResolved = true;
    }
    return m_beatCache;
}

void Note::Update()
{
    if (!m_active) return;
    if (!m_noteVisual) return;

    auto* beat = GetBeatManager();
    if (!beat) return;

    const double nowSec = beat->GetCurrentTimeSeconds();

    const float elapsed = static_cast<float>(nowSec) - (m_hitTime - m_travelTime);
    const float progress = elapsed / m_travelTime;
    const float newZ = m_startZ - (m_startZ - m_targetZ) * progress;

    Vector3 pos = m_noteVisual->GetPosition();
    pos.z = newZ;
    m_noteVisual->SetPosition(pos);

    // 判定済みで判定ラインを過ぎたらプールに返却
    if (m_judged && pos.z <= m_targetZ)
    {
        ReturnToPool();
        return;
    }

    // Miss判定
    const float missWindow = JudgeSystem::GetJudgeWindow().miss;
    if (!m_judged && (nowSec - m_hitTime) > missWindow)
    {
        GameScore::Instance().AddMiss();
        m_judged = true;

        auto* scene = Manager::GetScene();
        if (scene)
        {
            if (auto* p = scene->AddGameObject<ParticleInstance>(LAYER_WORLD_EFFECT))
            {
                p->SetType(ParticleType::Miss);
                p->SetPosition(m_noteVisual->GetPosition());
            }

            for (auto* eff : scene->GetGameObjects<JudgeEffect>())
            {
                if (eff) eff->SetDestroy();
            }
            scene->AddGameObject<JudgeEffect>(
                LAYER_UI,
                SCREEN_WIDTH / 2,
                static_cast<int>(SCREEN_HEIGHT * 0.7f),
                350, 200,
                const_cast<wchar_t*>(L"assets\\texture\\J_Miss.png"));
        }
        ReturnToPool();
        return;
    }
}

void Note::Draw()
{
}

void Note::SetColor(const Vector3& color, float gamma)
{
    if (m_noteVisual)
    {
        m_noteVisual->SetColor(color);
        m_noteVisual->SetGamma(gamma);
    }
}