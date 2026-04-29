//=========================================================
// beat.cpp
// ビート可視化クラス実装
//=========================================================
#include "beat.h"
#include "beatManager.h"
#include "manager.h"
#include "scene.h"
#include "polygon.h"
#include <cmath>
#include <algorithm>

//=========================================================
// ライフサイクル
//=========================================================

void Beat::Init(int bpm)
{
    if (BeatManager* bm = BeatManager::GetInstance())
    {
        bm->SetBPM(bpm);
    }

    m_beatVisual = Manager::GetScene()->AddGameObject<Polygon2D>(
        LAYER_UI, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 150, 200, 200,
        (wchar_t*)L"assets\\texture\\star.png");
}

void Beat::Uninit()
{
    if (m_beatVisual)
    {
        m_beatVisual->SetDestroy();
        m_beatVisual = nullptr;
    }
}

//-----------------------------------------------------
// 毎フレーム更新
// BeatManager のビートインデックスが変化したら m_currentScale を
// m_expandScale に設定し、指数減衰で m_targetScale へ収束させる。
// std::exp を使った減衰はフレームレート非依存になる。
//-----------------------------------------------------
void Beat::Update()
{
    BeatManager* bm = BeatManager::GetInstance();
    if (!bm) return;

    const uint64_t currentIndex = bm->GetCurrentBeatIndex();
    if (currentIndex != m_lastBeatIndex)
    {
        m_lastBeatIndex  = currentIndex;
        m_currentScale   = m_expandScale;
    }

    const float intervalSec = static_cast<float>(bm->GetBeatIntervalMilliseconds() * 0.001);
    const float elapsed     = static_cast<float>(bm->GetBeatPhase()) * intervalSec;
    const float dt          = std::min(elapsed, 0.1f); // 過大な dt によるオーバーシュートを防ぐ

    const float decay  = 1.0f - std::exp(-m_shrinkSpeed * dt);
    m_currentScale     = m_currentScale + (m_targetScale - m_currentScale) * decay;
    m_currentScale     = std::max(kMinScale, std::min(m_currentScale, kMaxScale));

    if (m_beatVisual)
    {
        m_beatVisual->SetScale(Vector3(m_currentScale, m_currentScale, 1.0f));
    }
}
