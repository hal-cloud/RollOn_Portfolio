#include "difficultySelector.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kDiffBadgeWidth = 300.0f;
    constexpr float kDiffBadgeHeight = 300.0f;
    constexpr float kDiffBadgeSpacing = 200.0f;
    const wchar_t* kDiffBadgeTextures[3] = {
        L"assets\\texture\\D_Easy.png",
        L"assets\\texture\\D_Normal.png",
        L"assets\\texture\\D_Hard.png"
    };
    constexpr float kSelectionLerpFactor = 0.09f;
}

void DifficultySelector::Init(Scene* scene)
{
    m_scene = scene;
    const float baseX = static_cast<float>(SCREEN_WIDTH * 0.9) - kDiffBadgeWidth * 0.8f;
    const float baseY = static_cast<float>(SCREEN_HEIGHT) * 0.25f;

    for (int i = 0; i < 3; ++i)
    {
        m_badges[i] = m_scene->AddGameObject<Polygon2D>(
            LAYER_UI,
            baseX,
            baseY + kDiffBadgeSpacing * static_cast<float>(i),
            kDiffBadgeWidth,
            kDiffBadgeHeight,
            const_cast<wchar_t*>(kDiffBadgeTextures[i])
        );
        if (m_badges[i])
        {
            m_badges[i]->SetColor(Vector3(1.0f, 1.0f, 1.0f));
            m_badges[i]->SetAlpha(0.4f);
        }
    }
}

void DifficultySelector::Change(int delta)
{
    const int kMin = 0;
    const int kMax = 2;
    int next = m_currentDifficulty + delta;
    if (next < kMin) next = kMax;
    if (next > kMax) next = kMin;
    m_currentDifficulty = next;
}

void DifficultySelector::UpdateVisuals()
{
    const float diffDelta = static_cast<float>(m_currentDifficulty) - m_visualDifficulty;
    m_visualDifficulty += diffDelta * kSelectionLerpFactor;
    if (std::fabs(diffDelta) < 0.001f)
    {
        m_visualDifficulty = static_cast<float>(m_currentDifficulty);
    }

    for (int i = 0; i < 3; ++i)
    {
        auto* badge = m_badges[i];
        if (!badge) continue;

        float distance = static_cast<float>(i) - m_visualDifficulty;
        const float influence = std::max(0.0f, 1.0f - std::min(std::fabs(distance), 1.0f));
        const float scale = 0.9f + 0.15f * influence;
        const float alpha = 0.35f + 0.65f * influence;

        badge->SetScale(Vector3(scale, scale, 1.0f));
        badge->SetAlpha(alpha);
    }
}