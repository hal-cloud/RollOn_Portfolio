#pragma once
#include <functional>
#include <string>
#include <vector>
#include <array>
#include "scene.h"
#include "polygon.h"
#include "songScene.h"
#include "songCarousel.h"
#include "difficultySelector.h"
#include "difficultyStars.h"
#include "uiManager.h"

class Number;

class Game : public Scene
{
public:
    void Init() override;
    void Update() override;

private:
    float m_masterVolume = 1.0f;
    bool  m_paused = false;

    SongCarousel       m_carousel;
    DifficultySelector m_diffSelector;
    DifficultyStars    m_diffStars;

    UIPanel m_pausePanel;

    Polygon2D* m_nameplateBase = nullptr;
    Polygon2D* m_nameplateText = nullptr;
    size_t     m_lastNameplateIndex = static_cast<size_t>(-1);

    Number*    m_highScoreNumber = nullptr;
    Polygon2D* m_highScoreLabel  = nullptr;
    Polygon2D* m_highScoreGrade  = nullptr;
    size_t     m_lastHSSelection = static_cast<size_t>(-1);
    int        m_lastHSDifficulty = -1;

    void SetScenes();
    void StartSelectedScene();
    void UpdateNameplate();
    void InitGameObjects();
    void UpdateHighScore();

    void TogglePause();
    void CreatePauseUI();
};