#include "songCarousel.h"
#include "soundManager.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr int   kSongArtworkWidth = 600;
    constexpr int   kSongArtworkHeight = kSongArtworkWidth * 0.6f;
    constexpr float kSongSlotSpacing = kSongArtworkHeight * 1.1f;
    constexpr float kSongSelectedScale = 1.15f;
    constexpr float kSongDimmedScale = 0.9f;
    constexpr float kSongSelectedAlpha = 1.0f;
    constexpr float kSongDimmedAlpha = 0.5f;
    constexpr float kSongHiddenAlpha = 0.0f;
    constexpr float kSelectionLerpFactor = 0.09f;
    constexpr float kSongVisibilityThreshold = 1.5f;
}

void SongCarousel::Init(Scene* scene, std::vector<MusicSceneEntry>&& list)
{
    m_scene = scene;
    m_musicScenes = std::move(list);
    m_currentSelection = 0;
    m_visualSelection = 0.0f;
    m_lastPlayedSelection = static_cast<size_t>(-1);

    for (auto& entry : m_musicScenes)
    {
        if (entry.texturePath.empty())
        {
            entry.artwork = nullptr;
            continue;
        }

        entry.artwork = m_scene->AddGameObject<Polygon2D>(
            LAYER_UI,
            SCREEN_WIDTH / 2,
            SCREEN_HEIGHT / 2,
            kSongArtworkWidth,
            kSongArtworkHeight,
            const_cast<wchar_t*>(entry.texturePath.c_str()));

        if (entry.artwork)
        {
            entry.artwork->SetPriority(5);
            entry.artwork->SetGamma(kSongHiddenAlpha);
        }
    }
}

void SongCarousel::UpdateSelection(int delta)
{
    if (m_musicScenes.empty() || delta == 0) return;

    SoundManager::Play("SE_Select", false, 0.7f);

    const int sceneCount = static_cast<int>(m_musicScenes.size());
    int nextIndex = static_cast<int>(m_currentSelection) + delta;
    nextIndex %= sceneCount;
    if (nextIndex < 0) nextIndex += sceneCount;

    m_currentSelection = static_cast<size_t>(nextIndex);
    PlaySelectionBGM();
}

void SongCarousel::TickAnimation()
{
    if (m_musicScenes.empty()) return;

    const float total = static_cast<float>(m_musicScenes.size());
    const float target = static_cast<float>(m_currentSelection);
    float diff = target - m_visualSelection;

    if (diff > total * 0.5f)  diff -= total;
    else if (diff < -total * 0.5f) diff += total;

    m_visualSelection += diff * kSelectionLerpFactor;
    if (std::fabs(diff) < 0.001f) m_visualSelection = target;

    if (m_visualSelection < 0.0f)      m_visualSelection += total;
    else if (m_visualSelection >= total) m_visualSelection -= total;
}

void SongCarousel::UpdateSongVisuals()
{
    if (m_musicScenes.empty()) return;

    const size_t total = m_musicScenes.size();
    const float totalF = static_cast<float>(total);
    const float centerX = static_cast<float>(SCREEN_WIDTH) * 0.5f;
    const float centerY = static_cast<float>(SCREEN_HEIGHT) * 0.5f;

    for (size_t i = 0; i < total; ++i)
    {
        auto& entry = m_musicScenes[i];
        if (!entry.artwork) continue;

        float distance = static_cast<float>(i) - m_visualSelection;
        if (distance > totalF * 0.5f) distance -= totalF;
        else if (distance < -totalF * 0.5f) distance += totalF;

        if (std::fabs(distance) <= kSongVisibilityThreshold)
        {
            Vector3 position = entry.artwork->GetPosition();
            position.x = centerX;
            position.y = centerY + distance * kSongSlotSpacing;
            entry.artwork->SetPosition(position);

            const float influence = std::max(0.0f, 1.0f - std::min(std::fabs(distance), 1.0f));
            const float scale = kSongDimmedScale + (kSongSelectedScale - kSongDimmedScale) * influence;
            const float alpha = kSongDimmedAlpha + (kSongSelectedAlpha - kSongDimmedAlpha) * influence;

            entry.artwork->SetScale(Vector3(scale, scale, 1.0f));
            entry.artwork->SetGamma(alpha);
        }
        else
        {
            entry.artwork->SetGamma(kSongHiddenAlpha);
        }
    }
}

void SongCarousel::PlaySelectionBGM()
{
    if (m_musicScenes.empty()) return;
    if (m_lastPlayedSelection == m_currentSelection) return;

    if (m_lastPlayedSelection != static_cast<size_t>(-1))
    {
        const auto& prev = m_musicScenes[m_lastPlayedSelection];
        if (!prev.bgmKey.empty()) SoundManager::Stop(prev.bgmKey);
    }

    const auto& entry = m_musicScenes[m_currentSelection];
    if (entry.bgmKey.empty() || entry.bgmPath.empty()) return;

    SoundManager::LoadStreaming(entry.bgmKey, entry.bgmPath);
    SoundManager::PlayStreaming(entry.bgmKey, true, 1.0f, entry.bgmStartTime);

    m_lastPlayedSelection = m_currentSelection;
}