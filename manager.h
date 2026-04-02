#pragma once
#include <list>
#include <string>
#include <vector>
#include <memory>
#include "gameObject.h"

class Scene;

class Manager
{
private:
	static std::unique_ptr<Scene> s_scene;
	static std::unique_ptr<Scene> s_sceneNext;

public:
	static void Init();
	static void Uninit();
	static void Update();
	static void Draw();
	static Scene* GetScene() { return s_scene.get(); }

	template <typename T>
	static void SetScene()
	{
		s_sceneNext = std::make_unique<T>();
	}
};