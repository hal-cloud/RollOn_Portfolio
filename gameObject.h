#pragma once
#include "main.h"
#include "component.h"
#include <unordered_map>
#include <typeindex>
#include <memory>

class GameObject
{
public:

	virtual ~GameObject() 
	{
		for (auto& pair : m_components)
		{
			pair.second->Uninit();
		}
		m_components.clear();
	}

	virtual void Init() {}
	virtual void Init(int posx, int posy, int width, int height, const wchar_t* fileName) {}
	virtual void Uninit() {}

	virtual void Update()
	{
		if (!m_active) return;

		for (auto &pair : m_components)
		{
			pair.second->Update();
		}
	}

	virtual void Draw() {
		if (!m_active) return;

		for (auto& pair : m_components)
		{
			pair.second->Draw();
		}
	}

	void SetDestroy() { m_destroy = true; }
	bool Destroy()
	{
		if(m_destroy)
		{
			Uninit();
			return true; 
		}
		else
		{
			return false;
		}
	}

	bool IsActive() const { return m_active; }

	void Deactivate()
	{
		m_active = false;
	}

	void Activate()
	{
		m_active = true;
		m_destroy = false;
	}

	virtual void Reset()
	{
		m_position = { 0.0f, 0.0f, 0.0f };
		m_prevPosition = { 0.0f, 0.0f, 0.0f };
		m_rotation = { 0.0f, 0.0f, 0.0f };
		m_scale = { 1.0f, 1.0f, 1.0f };
		m_destroy = false;
		m_active = true;
		m_drawPriority = 0;
	}

	void Reuse(Vector3 pos)
	{
		Reset();
		m_position = pos;
	}

	Vector3 GetPosition() { return m_position; }
	void SetPosition(Vector3 pos) { m_position = pos; }

	Vector3 GetRotation() { return m_rotation; }
	void SetRotation(Vector3 rot) { m_rotation = rot; }

	Vector3 GetScale() { return m_scale; }
	void SetScale(Vector3 scale) { m_scale = scale; }

	Vector3 GetRight()
	{
		XMMATRIX matrix;
		matrix = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);

		Vector3 right;
		XMStoreFloat3((XMFLOAT3*)&right, matrix.r[0]);

		return right;
	}
	Vector3 GetForward()
	{
		XMMATRIX matrix;
		matrix = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);

		Vector3 forward;
		XMStoreFloat3((XMFLOAT3*)&forward, matrix.r[2]);

		return forward;
	}
	Vector3 GetUp()
	{
		XMMATRIX matrix;
		matrix = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
		Vector3 up;
		XMStoreFloat3((XMFLOAT3*)&up, matrix.r[1]);
		return up;
	}
	float GetDistance(Vector3 position)
	{
		return (m_position - position).length();
	}
	Vector3 GetPrevPosition() { return m_prevPosition; }

	void SetPriority(int priority) { m_drawPriority = priority; }
	int GetPriority() { return m_drawPriority; }

	template<typename T, typename... TArgs>
	T* AddComponent(TArgs&&... args) {
		auto component = std::make_unique<T>(std::forward<TArgs>(args)...);
		component->Init();
		T* ptr = component.get();
		m_components[std::type_index(typeid(T))] = std::move(component);
		return ptr;
	}
	virtual void SetComponents() {};

protected:
	Vector3 m_position{ 0.0,0.0,0.0 };
	Vector3 m_prevPosition{ 0.0,0.0,0.0 };
	Vector3 m_rotation{ 0.0,0.0,0.0 };
	Vector3 m_scale{ 1.0,1.0,1.0 };

	bool m_destroy = false;
	bool m_active = true;

	int m_drawPriority = 0;

	std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components;
private:
};