#include "difficultyStars.h"

namespace
{
    constexpr float kStarSize    = 125.0f;
    constexpr float kStarSpacing = 90.0f;
    const wchar_t* kStarTexture  = L"assets\\texture\\star.png";

    const Vector3 kStarCountPalette[6] = {
        {0.45f, 0.45f, 0.45f},
        {0.20f, 0.90f, 0.35f},
        {0.95f, 0.85f, 0.25f},
        {0.70f, 0.70f, 0.00f},
        {0.60f, 0.25f, 0.10f},
        {0.50f, 0.00f, 0.50f}
    };

    const Vector3 kStarColorOff = { 0.45f, 0.45f, 0.45f };
}

void DifficultyStars::Init(Scene* scene)
{
    m_scene = scene;
    const float baseX = static_cast<float>(SCREEN_WIDTH) * 0.05f + kStarSize * 0.5f;
    const float baseY = static_cast<float>(SCREEN_HEIGHT) * 0.8f;

    for (int i = 0; i < 5; ++i)
    {
        m_stars[i] = m_scene->AddGameObject<Polygon2D>(
            LAYER_UI,
            baseX + kStarSpacing * static_cast<float>(i),
            baseY,
            kStarSize,
            kStarSize,
            const_cast<wchar_t*>(kStarTexture)
        );
        if (m_stars[i])
        {
            m_stars[i]->SetPriority(6);
            m_stars[i]->SetColor(kStarColorOff);
            m_stars[i]->SetAlpha(0.3f);
        }
    }
}

void DifficultyStars::UpdateStars(int starCount)
{
    if (starCount < 0) starCount = 0;
    if (starCount > 5) starCount = 5;

    const Vector3 onColor = kStarCountPalette[starCount];

    for (int i = 0; i < 5; ++i)
    {
        auto* star = m_stars[i];
        if (!star) continue;

        const bool filled = i < starCount;
        star->SetColor(filled ? onColor : kStarColorOff);
        star->SetAlpha(filled ? 1.0f : 0.25f);
    }
}