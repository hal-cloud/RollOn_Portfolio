#pragma once
#include <vector>
#include <array>
#include <functional>
#include <string>
#include "polygon.h"
#include "scene.h"

struct MusicSceneEntry
{
    std::wstring name;
    std::wstring texturePath;
    std::string  bgmKey;
    std::wstring bgmPath;
    std::function<void()> activate;
    Polygon2D* artwork = nullptr;
    double bgmStartTime = 0.0;
    std::array<std::string, 3> scorePaths{};
    int totalDifficultyStars = 0;
    std::wstring nameplatePath;
};

class SongCarousel
{
public:
    void Init(Scene* scene, std::vector<MusicSceneEntry>&& list);
    void UpdateSelection(int delta);
    void TickAnimation();
    void PlaySelectionBGM();
    void UpdateSongVisuals();

    const MusicSceneEntry& Current() const { return m_musicScenes[m_currentSelection]; }
    size_t CurrentIndex() const { return m_currentSelection; }
    bool Empty() const { return m_musicScenes.empty(); }

private:
    Scene* m_scene = nullptr;
    std::vector<MusicSceneEntry> m_musicScenes;
    size_t m_currentSelection = 0;
    float  m_visualSelection = 0.0f;
    size_t m_lastPlayedSelection = static_cast<size_t>(-1);
};