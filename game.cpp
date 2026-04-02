#include <algorithm>
#include <cmath>
#include <io.h>

#include "main.h"
#include "manager.h"
#include "renderer.h"

#include "scene.h"
#include "result.h"
#include "input.h"

#include "game.h"
#include "polygon.h"
#include "soundManager.h"
#include "fade.h"
#include "camera.h"
#include "sky.h"
#include "gameField.h"
#include "gameParticle.h"
#include "gameSettings.h"
#include "number.h"
#include "highScoreManager.h"

#include "ontheNightRoad.h"
#include "rollOn.h"
#include "tutorial.h"
#include "theFinal.h"

namespace
{
    constexpr float kSelectionLerpFactor = 0.18f;

    inline const wchar_t* GradeToTexture(char grade)
    {
        switch (grade)
        {
        case 'S': return L"assets\\texture\\Rank_S.png";
        case 'A': return L"assets\\texture\\Rank_A.png";
        case 'B': return L"assets\\texture\\Rank_B.png";
        default:  return L"assets\\texture\\Rank_C.png";
        }
    }
}

void Game::Init()
{
    Scene::Init();

    SetScenes();

    SoundManager::StopAll();

    m_masterVolume = SoundManager::GetMasterVolume();
    SoundManager::SetMasterVolume(m_masterVolume);

    SoundManager::Load("SE_Select", L"assets\\sound\\SE_Select.wav");
    SoundManager::Load("SE_Begin",  L"assets\\sound\\SE_Begin.wav");

    InitGameObjects();

    m_diffSelector.Init(this);
    m_diffStars.Init(this);

    CreatePauseUI();

    m_carousel.PlaySelectionBGM();
    m_carousel.UpdateSongVisuals();
    m_diffSelector.UpdateVisuals();
    m_diffStars.UpdateStars(m_carousel.Current().totalDifficultyStars);
    UpdateNameplate();
    UpdateHighScore();
}

void Game::Update()
{
    Fade* fade = GetGameObject<Fade>();
    if (fade)
    {
        fade->SetColor({ 1.0f,1.0f,1.0f });
        fade->SetFadeSpeed(0.01f);
    }

    Scene::Update();

    if (Input::GetKeyTrigger(VK_ESCAPE))
    {
        TogglePause();
    }

    if (m_paused)
    {
        m_pausePanel.Update();
        return;
    }

    if (Input::GetKeyTrigger('S') || Input::GetKeyTrigger(VK_DOWN))
    {
        m_carousel.UpdateSelection(1);
    }
    else if (Input::GetKeyTrigger('W') || Input::GetKeyTrigger(VK_UP))
    {
        m_carousel.UpdateSelection(-1);
    }

    if (Input::GetKeyTrigger('A') || Input::GetKeyTrigger(VK_LEFT))
    {
        m_diffSelector.Change(-1);
        SoundManager::Play("SE_Select", false, 0.7f);
    }
    else if (Input::GetKeyTrigger('D') || Input::GetKeyTrigger(VK_RIGHT))
    {
        m_diffSelector.Change(1);
        SoundManager::Play("SE_Select", false, 0.7f);
    }

    if (Input::GetKeyTrigger(VK_RETURN))
    {
        StartSelectedScene();
        return;
    }

    m_carousel.TickAnimation();

    m_carousel.UpdateSongVisuals();
    m_diffSelector.UpdateVisuals();
    m_diffStars.UpdateStars(m_carousel.Current().totalDifficultyStars);
    UpdateNameplate();
    UpdateHighScore();
}

void Game::UpdateHighScore()
{
    if (!m_highScoreNumber || m_carousel.Empty()) return;

    const size_t idx = m_carousel.CurrentIndex();
    const int diff = m_diffSelector.Current();

    if (idx == m_lastHSSelection && diff == m_lastHSDifficulty) return;

    const auto& entry = m_carousel.Current();
    const std::string key = HighScoreManager::MakeKey(entry.name, diff);
    const HighScoreManager::Record rec = HighScoreManager::GetRecord(key);

    m_highScoreNumber->SetValue(rec.score);

    if (m_highScoreGrade)
    {
        const wchar_t* tex = GradeToTexture(rec.grade);
        if (rec.score == 0)
            tex = L"assets\\texture\\Rank_C.png";
        m_highScoreGrade->SetTexture(const_cast<wchar_t*>(tex));
    }

    m_lastHSSelection = idx;
    m_lastHSDifficulty = diff;
}

void Game::TogglePause()
{
    m_paused = !m_paused;
    m_pausePanel.SetVisible(m_paused);

    if (m_paused)
    {
        m_pausePanel.ResetSelection();
    }
    else
    {
        if (!m_carousel.Empty())
        {
            SoundManager::Resume(m_carousel.Current().bgmKey, true);
        }
    }
}

void Game::CreatePauseUI()
{
    m_pausePanel.SetBackground(
        SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f,
        (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT,
        L"assets\\texture\\white.png");
    m_pausePanel.SetBackgroundAlpha(0.4f);

    m_pausePanel.SetTitle(
        SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT * 0.15f,
        400, 400,
        L"assets\\texture\\Setting.png");

    float menuY = SCREEN_HEIGHT * 0.38f;
    float spacing = 120.0f;

    m_pausePanel.AddSliderItem(
        SCREEN_WIDTH / 2.0f, menuY,
        200, 200,
        L"assets\\texture\\S_Volume.png",
        m_masterVolume,
        0.05f,
        [this](float val) {
            m_masterVolume = val;
            SoundManager::SetMasterVolume(val);
        });

    float initialSpeedNorm = (GameSettings::GetNoteSpeed() - 0.25f) / 1.0f;
    initialSpeedNorm = std::max(0.0f, std::min(1.0f, initialSpeedNorm));

    m_pausePanel.AddSliderItem(
        SCREEN_WIDTH / 2, menuY + spacing,
        200, 200,
        L"assets\\texture\\S_Speed.png",
        initialSpeedNorm,
        0.05f,
        [](float val) {
            float speed = (0.25f + val * 1.0f);
            GameSettings::SetNoteSpeed(speed);
        });

    m_pausePanel.AddItem(
        SCREEN_WIDTH / 2.0f, menuY + spacing * 2.5f,
        300, 300,
        L"assets\\texture\\Resume.png",
        [this]() { TogglePause(); });

    m_pausePanel.SetUnselectedAlpha(0.5f);
    m_pausePanel.Create(this, 100);
}

void Game::SetScenes()
{
    std::vector<MusicSceneEntry> list;

    list.push_back({
        L"Tutorial",
        L"assets\\texture\\C_Tutorial.png",
        "BGM_Tutorial",
        L"assets\\sound\\BG_Tutorial.wav",
        []() { Manager::SetScene<Tutorial>(); },
        nullptr,
        3.7,
        { "assets/score/Tutorial_Easy.json", "assets/score/Tutorial_Normal.json", "assets/score/Tutorial_Hard.json" },
        1,
        L"assets\\texture\\N_Tutorial.png"
    });
    list.push_back({
        L"OntheNightRoad",
        L"assets\\texture\\C_OtNR.png",
        "BGM_OtNR",
        L"assets\\sound\\BG_OntheNightRoad.wav",
        []() { Manager::SetScene<OtNR>(); },
        nullptr,
        27.0,
        { "assets/score/OtNR_Easy.json", "assets/score/OtNR_Normal.json", "assets/score/OtNR_Hard.json" },
        3,
        L"assets\\texture\\N_OtNR.png"
    });
    list.push_back({
        L"RollOn",
        L"assets\\texture\\C_RO.png",
        "BGM_RO",
        L"assets\\sound\\BG_RollOn.wav",
        []() { Manager::SetScene<RO>(); },
        nullptr,
        46.0,
        { "assets/score/RO_Easy.json", "assets/score/RO_Normal.json", "assets/score/RO_Hard.json" },
        4,
        L"assets\\texture\\N_RO.png"
    });
    list.push_back({
        L"TheFinal",
        L"assets\\texture\\C_TF.png",
        "BGM_TF",
        L"assets\\sound\\BG_TheFinal.wav",
        []() { Manager::SetScene<TF>(); },
        nullptr,
        35.75,
        { "assets/score/TF_Easy.json", "assets/score/TF_Normal.json", "assets/score/TF_Hard.json" },
        5,
        L"assets\\texture\\N_TF.png"
    });

    m_carousel.Init(this, std::move(list));
}

void Game::UpdateNameplate()
{
    if (!m_nameplateText || m_carousel.Empty()) return;

    const size_t idx = m_carousel.CurrentIndex();
    if (idx == m_lastNameplateIndex) return;

    const auto& entry = m_carousel.Current();
    const std::wstring& path = entry.nameplatePath.empty() ? entry.texturePath : entry.nameplatePath;

    const wchar_t* resolved =
        (!path.empty() && _waccess(path.c_str(), 0) == 0)
            ? path.c_str()
            : L"assets\\texture\\white.png";

    m_nameplateText->SetTexture(const_cast<wchar_t*>(resolved));
    m_lastNameplateIndex = idx;
}

void Game::StartSelectedScene()
{
    if (m_carousel.Empty()) return;

    const auto& entry = m_carousel.Current();
    const std::string& scorePath = entry.scorePaths[m_diffSelector.Current()];

    DifficultyState::Set(entry.name,
        static_cast<DifficultyLevel>(m_diffSelector.Current()),
        scorePath,
        entry.texturePath,
        entry.nameplatePath,
        entry.activate);

    SoundManager::StopAll();
    SoundManager::SetMasterVolume(m_masterVolume);
    SoundManager::Play("SE_Begin", false, 0.5f);
    entry.activate();
}

void Game::InitGameObjects()
{
    AddGameObject<Camera>(LAYER_SYSTEM);

    AddGameObject<Sky>(LAYER_WORLD)->SetScale(Vector3(500, 500, 500));
    AddGameObject<GameField>(LAYER_WORLD);

    m_nameplateBase = AddGameObject<Polygon2D>(LAYER_UI, 325, 400, 525, 15, (wchar_t*)L"assets\\texture\\white.png");
    m_nameplateText = AddGameObject<Polygon2D>(LAYER_UI, 325, 250, 525, 300, (wchar_t*)L"assets\\texture\\white.png");
    if (m_nameplateBase) { m_nameplateBase->SetColor(Vector3(0.1f, 0.5f, 1.0f)); }
    if (m_nameplateText) { m_nameplateText->SetPriority(2); }

    AddGameObject<Polygon2D>(LAYER_UI, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 600, SCREEN_HEIGHT, (wchar_t*)L"assets\\texture\\white.png")
        ->SetColor(Vector3(0.1f, 0.5f, 1.0f));
    AddGameObject<Polygon2D>(LAYER_UI, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2,
        600 * 1.3f, (600 * 0.6f) * 1.3f, (wchar_t*)L"assets\\texture\\white.png")
        ->SetColor(Vector3(0.1f, 0.5f, 1.0f));
    AddGameObject<Polygon2D>(LAYER_UI, SCREEN_WIDTH * 0.15, SCREEN_HEIGHT * 0.7,
        300, 300, (wchar_t*)L"assets\\texture\\Grade.png");

    m_highScoreLabel = AddGameObject<Polygon2D>(LAYER_UI,
        (int)(SCREEN_WIDTH * 0.85f), (int)(SCREEN_HEIGHT * 0.85f),
        200, 200,
        (wchar_t*)L"assets\\texture\\HighScore.png");
    if (m_highScoreLabel) m_highScoreLabel->SetPriority(5);

    m_highScoreNumber = AddGameObject<Number>(LAYER_UI,
        (int)(SCREEN_WIDTH * 0.85f), (int)(SCREEN_HEIGHT * 0.9f),
        50, 50,
        (wchar_t*)L"assets\\texture\\number2.png");
    if (m_highScoreNumber)
    {
        m_highScoreNumber->SetAlign(Number::Align::Right);
        m_highScoreNumber->SetPriority(5);
    }

    m_highScoreGrade = AddGameObject<Polygon2D>(LAYER_UI,
        (int)(SCREEN_WIDTH * 0.76f), (int)(SCREEN_HEIGHT * 0.85f),
        175, 175,
        (wchar_t*)L"assets\\texture\\Rank_C.png");
    if (m_highScoreGrade) m_highScoreGrade->SetPriority(5);

    AddGameObject<GameParticle>(LAYER_WORLD_EFFECT)->SetType(ParticleType::Default);
    AddGameObject<Fade>(LAYER_UI);
}