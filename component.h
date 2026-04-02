#pragma once

class Component
{

protected:

	class  GameObject* m_gameObject = nullptr;

public:
	Component() {};
	Component(GameObject* Object) { m_gameObject = Object; }

	virtual ~Component() {}

	virtual void Init() {};
	virtual void Uninit() {};
	virtual void Update() {};
	virtual void Draw() {};

};