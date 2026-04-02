#include "main.h"
#include "manager.h"
#include "renderer.h"

#include "scene.h"
#include "input.h"
#include "xinputButton.h"
#include "camera.h"

#include "polygon.h"
#include "title.h"
#include "soundManager.h"
#include "loading.h"


void Title::Init()
{
	m_masterVolume = SoundManager::GetMasterVolume();
	SoundManager::SetMasterVolume(m_masterVolume);

	SoundManager::LoadStreaming("BGM_Title", L"assets\\sound\\BG_RollOn_Ver.Title.wav");
	SoundManager::PlayStreaming("BGM_Title", true, 1.0f);
	SoundManager::Load("SE_Start", L"assets\\sound\\SE_Start.wav");

	m_isStartRequested = false;

	m_backGround = AddGameObject<Polygon2D>(LAYER_UI, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT, (wchar_t*)L"assets\\texture\\C_RO.png");
	m_buttonStart = AddGameObject<Polygon2D>(LAYER_UI, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 100, 400, 400, (wchar_t*)L"assets\\texture\\T_Start.png");
	AddGameObject<Polygon2D>(LAYER_UI, SCREEN_WIDTH / 2, SCREEN_HEIGHT * 0.3, 1100, 800, (wchar_t*)L"assets\\texture\\N_RO.png");

	m_fade = AddGameObject<Fade>(LAYER_UI);
}
void Title::Update()
{
	Scene::Update();

	if (Input::GetKeyTrigger(VK_RETURN) && !m_isStartRequested)
	{
		SoundManager::SetMasterVolume(m_masterVolume);
		SoundManager::Play("SE_Start", false, 1.0f);

		m_fade->SetFadeSpeed(0.0075f);
		m_fade->StartFadeOut();

		m_isStartRequested = true;
	}
	if (m_isStartRequested && !m_fade->IsFading())
	{
		Manager::SetScene<Loading>();
		return;
	}
}