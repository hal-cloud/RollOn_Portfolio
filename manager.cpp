#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "xinputButton.h"
#include "soundManager.h"
#include "input.h"
#include "scene.h"
#include "game.h"
#include "title.h"

std::unique_ptr<Scene> Manager::s_scene = nullptr;
std::unique_ptr<Scene> Manager::s_sceneNext = nullptr;

void Manager::Init()
{
	Renderer::Init();
	Input::Init();
	XinputButton::Init();
	SoundManager::Init();
	s_scene = std::make_unique<Title>();
	s_scene->Init();
}

void Manager::Uninit()
{
	s_scene->Uninit();
	Renderer::Uninit();
	Input::Uninit();
	XinputButton::Uninit();
	SoundManager::Uninit();
}

void Manager::Update()
{	
	XinputButton::Update();
	Input::Update();
	s_scene->Update();
	SoundManager::Update();

	if (s_sceneNext)
	{
		s_scene->Uninit();
		s_scene = std::move(s_sceneNext);
		s_sceneNext = nullptr;
		s_scene->Init();
	}
}

void Manager::Draw()
{
	Renderer::Begin();
	s_scene->Draw();
	Renderer::End();
}
