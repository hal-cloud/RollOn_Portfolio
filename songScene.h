#pragma once
#include "scene.h"
#include "soundManager.h"
#include "gameScore.h"
#include "beatManager.h"
#include "beat.h"
#include "lane.h"
#include "tapEffect.h"
#include "laneEffect.h"
#include "judgeLine.h"
#include "noteManager.h"
#include "camera.h"
#include "player.h"
#include "sky.h"
#include "fade.h"
#include "field.h"
#include "gameParticle.h"
#include "gameScoreVisual.h"
#include "result.h"
#include "judgeEffect.h"
#include "quantizedSePlayer.h"
#include "polygon.h"
#include "uiManager.h"
#include <string>
#include <io.h>
#include <algorithm>
#include "title.h"

// 難易度と譜面パスを一時保持する簡易ステート（ゲームシーンで設定、SongSceneで参照）
enum class DifficultyLevel { Easy = 0, Normal = 1, Hard = 2 };

namespace DifficultyState
{
    struct State
    {
        std::wstring songTitle;
        DifficultyLevel level = DifficultyLevel::Normal;
        std::string scorePath;        // 選択された難易度の譜面パス
        std::wstring albumArtPath;    // 直前プレイ曲のジャケット
        std::wstring nameplatePath;   // ネームプレート用テクスチャ
		std::function<void()> retryFunc; // リトライコールバック
    };

    inline State& Access()
    {
        static State s;
        return s;
    }

    inline void Set(const std::wstring& songTitle,
        DifficultyLevel level,
        const std::string& scorePath,
        const std::wstring& albumArtPath = L"",
        const std::wstring& nameplatePath = L"",
        std::function<void()> retryFunc = nullptr
    )
    {
        auto& s = Access();
        s.songTitle     = songTitle;
        s.level         = level;
        s.scorePath     = scorePath;
        s.albumArtPath  = albumArtPath;
        s.nameplatePath = nameplatePath;
		s.retryFunc = retryFunc;
    }

    inline DifficultyLevel GetLevel()
    {
        return Access().level;
    }

    inline std::string ResolveScorePath(const std::wstring& songTitle, const std::string& defaultPath)
    {
        const auto& s = Access();
        if (s.songTitle == songTitle && !s.scorePath.empty())
        {
            return s.scorePath;
        }
        return defaultPath;
    }
}

namespace
{
    // UIレイアウト定数
    constexpr float kNameplateX   = SCREEN_WIDTH * 0.9f;
    constexpr float kNameplateY   = SCREEN_HEIGHT * 0.1f;
    constexpr float kNameplateW   = 400.0f;
    constexpr float kNameplateH   = 240.0f;
    constexpr float kBadgeX       = SCREEN_WIDTH * 0.9f;
    constexpr float kBadgeY       = SCREEN_HEIGHT * 0.2f;
    constexpr float kBadgeW       = 150.0f;
    constexpr float kBadgeH       = 150.0f;

    constexpr const wchar_t* kDiffBadgeTextures[3] = {
        L"assets\\texture\\D_Easy.png",
        L"assets\\texture\\D_Normal.png",
        L"assets\\texture\\D_Hard.png"
    };

    inline bool FileExists(const std::wstring& p)
    {
        return !p.empty() && _waccess(p.c_str(), 0) == 0;
    }
}

struct SongConfig
{
    std::wstring songTitle;     // GameScore用
    std::string  bgmKey;        // SoundManagerキー
    std::wstring bgmPath;       // wavパス
    int          bpm;
    std::string  scorePath;     // 譜面json
	std::wstring  albumArtPath; // アルバムアート画像パス
};

class SongScene : public Scene
{
protected:
    explicit SongScene(const SongConfig& cfg)
        : m_cfg(cfg)
        , m_tapQuantizer(cfg.bgmKey, "Tap", cfg.bpm, 4) // 16分
    {}

    bool m_paused = false;

    // 設定
    SongConfig m_cfg{};
    float m_masterVolume = 1.0f;

    // 遷移フラグ
    bool m_transitionRequested = false;

    // SE
    QuantizedSePlayer m_tapQuantizer;

    // リザルト演出
    bool  m_songFinished = false;
    float m_bannerTimer = 0.0f;
    const float m_bannerWaitSec = 1.5f;
    Polygon2D* m_resultBanner = nullptr;
    float m_bannerAlpha = 0.0f;
    const float m_bannerFadeSpeed = 1.5f;

    // UI
    Polygon2D* m_nameplateUi = nullptr;
    Polygon2D* m_diffBadgeUi = nullptr;

    // ポーズUI（UIPanelで管理）
    UIPanel m_pausePanel;

    void Init() override
    {
        Scene::Init();
        InitAudio();
        InitEnvironment();
        InitUI();
        InitGameplayObjects();
    }

    void Update() override
    {
        // ポーズトグル（Esc）
        if (Input::GetKeyTrigger(VK_ESCAPE))
        {
            TogglePause();
        }

        UpdateFadeColor();

        // ポーズ中はUIの入力処理のみ
        if (m_paused)
        {
            m_pausePanel.Update();
            return;
        }

        // BPM/オフセット更新
        if (auto* beatMgr = GetGameObject<BeatManager>())
        {
            m_tapQuantizer.SetBpm(beatMgr->GetBPM());
            m_tapQuantizer.SetManualOffsetMs(beatMgr->GetManualOffsetMilliseconds());
        }
        m_tapQuantizer.Update();

        Scene::Update();

        UpdateBannerFade();
        HandleSongFinish();
        HandleTransition();
    }

    void TogglePause()
    {
        m_paused = !m_paused;
        m_pausePanel.SetVisible(m_paused);
        
        if (m_paused)
        {
            m_pausePanel.ResetSelection();
            SoundManager::Pause(m_cfg.bgmKey, true);
        }
        else
        {
            SoundManager::Resume(m_cfg.bgmKey, true);
        }
    }

    // --- 初期化ヘルパー ---
    void InitAudio()
    {
        // ゲームスコアの初期化
        GameScore::Instance().BeginSong(m_cfg.songTitle.c_str());

        m_masterVolume = SoundManager::GetMasterVolume();
        SoundManager::SetMasterVolume(m_masterVolume);

        SoundManager::LoadStreaming(m_cfg.bgmKey.c_str(), m_cfg.bgmPath.c_str());
        SoundManager::PlayStreaming(m_cfg.bgmKey.c_str(), false, 1.0f);

        SoundManager::Load("SE_Clear", L"assets\\sound\\SE_Clear.wav");
    }

    void InitEnvironment()
    {
        AddGameObject<Camera>(LAYER_SYSTEM);

        auto* sky = AddGameObject<Sky>(LAYER_WORLD);
        if (sky) sky->SetScale(Vector3(500.0f, 500.0f, 500.0f));

        AddGameObject<Lane>(LAYER_WORLD);

        // 曲のジャケット表示背景
        float fWidth = 12.0f;
        auto* f = AddGameObject<Field>(LAYER_WORLD, Vector3(0.0f, -5.0f, 20.0f), fWidth, fWidth * 0.6f, m_cfg.albumArtPath.c_str());
        if (f) f->SetRotation(Vector3(-1.0f, 0.0f, 0.0f));
    }

    void InitUI()
    {
        AddGameObject<Fade>(LAYER_UI);
        AddGameObject<GameScoreVisual>(LAYER_UI, 0, 0, 0, 0, 40, 40, (wchar_t*)L"assets\\texture\\number2.png");

        CreateNameplate();
        CreateDiffBadge();
        CreatePauseUI();
    }

    void InitGameplayObjects()
    {
        // BPM管理
        auto* beatMgr = AddGameObject<BeatManager>(LAYER_SYSTEM);
        if (beatMgr)
        {
            beatMgr->SetBPM(m_cfg.bpm);
            beatMgr->SetTrackedSoundKey(m_cfg.bgmKey, true);
            AddGameObject<Beat>(LAYER_SYSTEM, beatMgr->GetBPM());
        }

        AddGameObject<JudgeLine>(LAYER_SYSTEM);

        // 難易度に応じて譜面パスを差し替え
        const std::string scorePath = DifficultyState::ResolveScorePath(m_cfg.songTitle, m_cfg.scorePath);
        AddGameObject<NoteManager>(LAYER_SYSTEM, scorePath.c_str());

        AddGameObject<GameParticle>(LAYER_WORLD_EFFECT)->SetType(ParticleType::Default);
        AddGameObject<Player>(LAYER_WORLD);

        //===========================================================
        // エフェクトプールの事前確保
        //===========================================================
        PrewarmPool<TapEffect>(LAYER_WORLD_EFFECT, 16, Vector3(0, 0, 0));
        PrewarmPool<LaneEffect>(LAYER_WORLD_EFFECT, 16, Vector3(0, 0, 0), 1.0f, 1.0f, Vector3(1, 1, 1));
    }

    // --- UI生成 ---
    void CreateNameplate()
    {
        const auto& st = DifficultyState::Access();

        std::wstring nameplateTex =
            FileExists(st.nameplatePath) ? st.nameplatePath
            : FileExists(m_cfg.albumArtPath) ? m_cfg.albumArtPath
            : L"assets\\texture\\white.png";

        m_nameplateUi = AddGameObject<Polygon2D>(
            LAYER_UI,
            kNameplateX,
            kNameplateY,
            kNameplateW,
            kNameplateH,
            const_cast<wchar_t*>(nameplateTex.c_str()));

        if (m_nameplateUi)
        {
            m_nameplateUi->SetPriority(40);
            m_nameplateUi->SetGamma(0.8f);
        }
    }

    void CreateDiffBadge()
    {
        const auto& st = DifficultyState::Access();
        const int diffIdx = std::max(0, std::min(2, static_cast<int>(st.level)));
        const wchar_t* diffTex = kDiffBadgeTextures[diffIdx];

        m_diffBadgeUi = AddGameObject<Polygon2D>(
            LAYER_UI,
            kBadgeX,
            kBadgeY,
            kBadgeW,
            kBadgeH,
            const_cast<wchar_t*>(diffTex));

        if (m_diffBadgeUi)
        {
            m_diffBadgeUi->SetPriority(40);
            m_diffBadgeUi->SetGamma(0.8f);
        }
    }

    void CreatePauseUI()
    {
        // 背景（画面全体を暗くする）
        m_pausePanel.SetBackground(
            SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f,
            (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT,
            L"assets\\texture\\white.png");
        m_pausePanel.SetBackgroundAlpha(0.7f);

        // タイトル（PAUSE）
        m_pausePanel.SetTitle(
            SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT * 0.2f,
            400, 400,
            L"assets\\texture\\Pause.png");

        // メニュー項目
        float menuY = SCREEN_HEIGHT * 0.45f;
        float spacing = 150.0f;

        // 「つづける」
        m_pausePanel.AddItem(
            SCREEN_WIDTH / 2.0f, menuY,
            300, 300,
            L"assets\\texture\\Resume.png",
            [this]() { TogglePause(); });

        // 「やりなおす」
        m_pausePanel.AddItem(
            SCREEN_WIDTH / 2.0f, menuY + spacing,
            300, 300,
            L"assets\\texture\\Retry.png",
            [this]() { RetryGame(); });
		// 「オートプレイ」
        m_pausePanel.AddItem(
            SCREEN_WIDTH / 2.0f, menuY + spacing * 2,
            300, 300,
            L"assets\\texture\\Auto.png",
            [this]() { SetAutoPlay(); });

        // 「やめる」
        m_pausePanel.AddItem(
            SCREEN_WIDTH / 2.0f, menuY + spacing * 3,
            450, 300,
            L"assets\\texture\\BackMenu.png",
            [this]() { QuitToGame(); });


        m_pausePanel.SetUnselectedAlpha(0.5f);
        m_pausePanel.Create(this, 100);
    }

    // ポーズメニューのアクション
    void RetryGame()
    {
        SoundManager::StopAll();
        OnRetry(); // 派生クラスで実装
    }
    
    // 派生クラスでオーバーライドしてリトライを実装
    virtual void OnRetry()
    {
        SoundManager::SetMasterVolume(m_masterVolume);
        Manager::SetScene<Title>();
    }

    void SetAutoPlay()
    {
        if (auto* player = GetGameObject<Player>())
        {
            player->SetAutoPlay();
			TogglePause();
		}
    }

    // --- 更新ヘルパー ---
    void UpdateFadeColor()
    {
        if (auto* fade = GetGameObject<Fade>())
        {
            fade->SetColor({ 1.0f,1.0f,1.0f });
            fade->SetFadeSpeed(0.01f);
        }
    }

    void UpdateBannerFade()
    {
        if (m_resultBanner && m_bannerAlpha < 1.0f)
        {
            m_bannerAlpha = std::min(1.0f, m_bannerAlpha + m_bannerFadeSpeed * FRAME_TIME);
            m_resultBanner->SetGamma(m_bannerAlpha);
        }
    }

    void HandleSongFinish()
    {
        const bool songEnded = !SoundManager::IsSoundActive(m_cfg.bgmKey.c_str(), true);

        if (m_songFinished || !songEnded)
            return;

        m_songFinished = true;
        m_bannerTimer = 0.0f;

        SoundManager::StopAll();
        GameScore::Instance().CommitResult();

        if (GameScore::Instance().GetCurrent().grade != L'C')
        {
            SoundManager::Play("SE_Clear", false, 0.3f);
        }

        ShowResultBanner();
    }

    void HandleTransition()
    {
        auto* fade = GetGameObject<Fade>();

        // バナー表示後の待ち → フェード開始
        if (m_songFinished && !m_transitionRequested)
        {
            m_bannerTimer += FRAME_TIME;
            if (m_bannerTimer >= m_bannerWaitSec)
            {
                m_transitionRequested = true;
                if (fade) fade->StartFadeOut();
                else { SoundManager::SetMasterVolume(m_masterVolume); Manager::SetScene<Result>(); return; }
            }
        }

        // フェード完了後リザルトへ
        if (m_transitionRequested && (!fade || !fade->IsFading()))
        {
            SoundManager::SetMasterVolume(m_masterVolume);
            Manager::SetScene<Result>();
        }
    }

    // バナー生成ヘルパー
    void ShowResultBanner()
    {
        if (m_resultBanner) return;

        const wchar_t* tex =
            (GameScore::Instance().GetCurrent().grade != L'C')
            ? L"assets\\texture\\Clear.png"
            : L"assets\\texture\\Failed.png";

        m_resultBanner = AddGameObject<Polygon2D>(
            LAYER_UI,
            SCREEN_WIDTH / 2,
            SCREEN_HEIGHT / 2,
            750, 500,
            const_cast<wchar_t*>(tex));

        if (m_resultBanner)
        {
            m_resultBanner->SetPriority(90);
            m_bannerAlpha = 0.0f;
            m_resultBanner->SetGamma(m_bannerAlpha); // α=0で開始
        }
    }

    void QuitToGame();

public:
    void RequestTapSe(float volume = 1.0f)
    {
        m_tapQuantizer.RequestFire(volume);
    }
};