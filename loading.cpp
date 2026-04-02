#include "main.h"

#include "manager.h"
#include "renderer.h"

#include "scene.h"
#include "game.h"
#include "input.h"
#include "xinputButton.h"
#include "camera.h"

#include "polygon.h"
#include "loading.h"
#include "soundManager.h"

#include <thread>
#include "sky.h"

void Loading::Init()
{
	m_masterVolume = SoundManager::GetMasterVolume();
	SoundManager::SetMasterVolume(m_masterVolume);

	m_backGround = AddGameObject<Polygon2D>(LAYER_UI, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT, (wchar_t*)L"assets\\texture\\C_RO.png");
	m_buttonStart = AddGameObject<Polygon2D>(LAYER_UI, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 100, 400, 400, (wchar_t*)L"assets\\texture\\squea.png");
	AddGameObject<Fade>(LAYER_UI);

	m_finish = false;

	std::thread th([=]
		{
			Sky::Load();
			m_finish = true;
		});
	th.detach();
}
void Loading::Update()
{
	Scene::Update();

	m_buttonRotation += 0.05f;
	m_buttonStart->SetRotation(Vector3(0.0f, 0.0f, m_buttonRotation));

	if (m_finish)
	{
		SoundManager::StopAll();
		SoundManager::SetMasterVolume(m_masterVolume);
		Manager::SetScene<Game>();
		return;
	}
}