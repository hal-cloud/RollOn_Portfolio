#include "main.h"
#include "manager.h"
#include "renderer.h"

#include "scene.h"
#include "title.h"
#include "game.h"
#include "camera.h"
#include "input.h"

#include "polygon.h"
#include "result.h"
#include "soundManager.h"
#include "gameScore.h"
#include "number.h"

#include "fade.h"
#include <io.h>
#include "songScene.h"
#include "highScoreManager.h"

void Result::Init()
{	
	m_masterVolume = SoundManager::GetMasterVolume();
	SoundManager::SetMasterVolume(m_masterVolume);

	SoundManager::LoadStreaming("BG_Result", L"assets\\sound\\BG_Result.wav");
	SoundManager::PlayStreaming("BG_Result", true, 0.6f);

	AddGameObject<Polygon2D>(
		LAYER_UI,
		SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		(wchar_t*)L"assets\\texture\\sky.png");

	const auto& state = DifficultyState::Access();
	const std::wstring& jacket = state.albumArtPath;
	const wchar_t* resolved =
		(!jacket.empty() && _waccess(jacket.c_str(), 0) == 0)
			? jacket.c_str()
			: L"assets\\texture\\white.png";

	AddGameObject<Polygon2D>(
		LAYER_UI,
		SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		const_cast<wchar_t*>(resolved))->SetGamma(0.6f);
	
	const ScoreResult& res = GameScore::Instance().GetLatest();
	const int values[] = { res.maxCombo, res.perfect, res.good, res.miss, res.score };

	{
		const std::string key = HighScoreManager::MakeKey(
			state.songTitle, static_cast<int>(state.level));
		char gradeChar = static_cast<char>(res.grade);
		HighScoreManager::TryUpdate(key, res.score, gradeChar);
	}

	const float startX = SCREEN_WIDTH * 0.8f;
	const float startY = SCREEN_HEIGHT * 0.25f;
	const float stepY  = 125.0f;
	const int   w = 80;
	const int   h = 80;
	for (int i = 0; i < 5; ++i)
	{
		auto* n = AddGameObject<Number>(LAYER_UI,
			(int)startX,
			(int)(startY + i * stepY),
			w, h,
			(wchar_t*)L"assets\\texture\\number2.png");
		if (n)
		{
			n->SetAlign(Number::Align::Right);
			n->TweenTo(values[i], 0.75);
			n->SetPriority(10);
		}
		switch (i)
		{
		case 0:
			AddGameObject<Polygon2D>(LAYER_UI,
				(int)SCREEN_WIDTH / 2,
				(int)(startY + i * stepY + 50),
				300, 300,
				(wchar_t*)L"assets\\texture\\combo.png");
			break;
		case 1:
			AddGameObject<Polygon2D>(LAYER_UI,
				(int)SCREEN_WIDTH / 2,
				(int)(startY + i * stepY + 50),
				400, 350,
				(wchar_t*)L"assets\\texture\\j_perfect.png");
			break;
		case 2:
			AddGameObject<Polygon2D>(LAYER_UI,
				(int)SCREEN_WIDTH / 2,
				(int)(startY + i * stepY + 50),
				400, 350,
				(wchar_t*)L"assets\\texture\\j_good.png");
			break;
		case 3:
			AddGameObject<Polygon2D>(LAYER_UI,
				(int)SCREEN_WIDTH / 2,
				(int)(startY + i * stepY + 50),
				400, 350,
				(wchar_t*)L"assets\\texture\\j_miss.png");
			break;
		case 4:
			AddGameObject<Polygon2D>(LAYER_UI,
				(int)SCREEN_WIDTH / 2,
				(int)(startY + i * stepY + 50),
				300, 250,
				(wchar_t*)L"assets\\texture\\score.png");
			break;
		default:
			break;
		}
	}

	auto selectGradeTex = [&](wchar_t grade) -> const wchar_t*
	{
		switch (grade)
		{
		case L'S': return L"assets\\texture\\rank_S.png";
		case L'A': return L"assets\\texture\\rank_A.png";
		case L'B': return L"assets\\texture\\rank_B.png";
		default:   return L"assets\\texture\\rank_C.png";
		}
	};
	const wchar_t* gradeTex = selectGradeTex(res.grade);
	if (_waccess(gradeTex, 0) != 0)
	{
		gradeTex = L"assets\\texture\\white.png";
	}
	AddGameObject<Polygon2D>(
		LAYER_UI,
		(int)SCREEN_WIDTH * 0.2,
		(int)SCREEN_HEIGHT / 2,
		600, 600,
		const_cast<wchar_t*>(gradeTex))->SetPriority(10);

	float menuY = SCREEN_HEIGHT * 0.7f;
	float spacing = 200.0f;

	m_resultPanel.AddItem(
		SCREEN_WIDTH * 0.2f, menuY,
		300, 300,
		L"assets\\texture\\Retry.png",
		[this]()
		{
			SoundManager::StopAll();
			SoundManager::SetMasterVolume(m_masterVolume);
			const auto& st = DifficultyState::Access();
			if (st.retryFunc)
			{
				st.retryFunc();
			}
			else
			{
				Manager::SetScene<Game>();
			}
		});

	m_resultPanel.AddItem(
		SCREEN_WIDTH * 0.2f, menuY + spacing,
		450, 300,
		L"assets\\texture\\BackMenu.png",
		[this]()
		{
			SoundManager::StopAll();
			SoundManager::SetMasterVolume(m_masterVolume);
			Manager::SetScene<Game>();
		});

	m_resultPanel.SetUnselectedAlpha(0.5f);
	m_resultPanel.Create(this, 50);
	m_resultPanel.SetVisible(true);

	AddGameObject<Fade>(LAYER_UI);
}

void Result::Update()
{   
    Fade* fade = GetGameObject<Fade>();
    if (fade)
    {
        fade->SetColor({ 1.0f,1.0f,1.0f });
        fade->SetFadeSpeed(0.01f);
    }
    Scene::Update();

    m_resultPanel.Update();
}
