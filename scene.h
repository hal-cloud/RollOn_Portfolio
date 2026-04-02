#pragma once
#include <list>
#include <string>
#include <vector>
#include "gameObject.h"

enum Layer : int
{
	LAYER_SYSTEM = 0,
	LAYER_WORLD = 1,
	LAYER_WORLD_EFFECT = 2,
	LAYER_UI = 3,
	LAYER_MAX
};

class Scene
{
private:
	std::vector<std::unique_ptr<GameObject>> m_gameObject[LAYER_MAX];
	std::vector<std::pair<int, std::unique_ptr<GameObject>>> m_pendingObjects;

public:

	virtual void Init();
	virtual void Uninit();
	virtual void Update();
	virtual void Draw();

	template<typename T, typename... Args>
	T* AddGameObject(int layer, Args&&... args) {
		auto gameObject = std::make_unique<T>();
		gameObject->Init(std::forward<Args>(args)...);
		T* ptr = gameObject.get();
		m_pendingObjects.emplace_back(layer, std::move(gameObject));
		return ptr;
	}

	template <typename T>
	T* GetGameObject()
	{
		for (int i = 0; i < LAYER_MAX; i++)
		{
			for (auto& gameObject : m_gameObject[i])
			{
				T* find = dynamic_cast<T*>(gameObject.get());
				if (find != nullptr)
					return find;
			}
		}
		return nullptr;
	}

	template <typename T>
	std::vector<T*> GetGameObjects()
	{
		std::vector<T*> finds;
		for (int i = 0; i < LAYER_MAX; i++)
		{
			for (auto& gameObject : m_gameObject[i])
			{
				T* find = dynamic_cast<T*>(gameObject.get());
				if (find != nullptr)
					finds.push_back(find);
			}
		}
		return finds;
	}

	template <typename T>
	T* FindInactive(int layer)
	{
		for (auto& gameObject : m_gameObject[layer])
		{
			if (!gameObject->IsActive())
			{
				T* found = dynamic_cast<T*>(gameObject.get());
				if (found != nullptr)
					return found;
			}
		}
		for (auto& pair : m_pendingObjects)
		{
			if (pair.first == layer && !pair.second->IsActive())
			{
				T* found = dynamic_cast<T*>(pair.second.get());
				if (found != nullptr)
					return found;
			}
		}
		return nullptr;
	}

	template<typename T, typename... Args>
	T* AcquireGameObject(int layer, Args&&... args)
	{
		T* recycled = FindInactive<T>(layer);
		if (recycled)
		{
			recycled->Reset();
			recycled->Init(std::forward<Args>(args)...);
			return recycled;
		}
		return AddGameObject<T>(layer, std::forward<Args>(args)...);
	}

	template<typename T, typename... Args>
	void PrewarmPool(int layer, int count, Args&&... args)
	{
		for (int i = 0; i < count; ++i)
		{
			T* obj = AddGameObject<T>(layer, std::forward<Args>(args)...);
			obj->Deactivate();
		}
	}
};