#pragma once
#include "main.h"
#include "manager.h"
#include "gameObject.h"
#include <unordered_map>
#include <memory>

class GameField : public GameObject
{
private:
	class Field* m_field[3] = {};
	float m_rotation = 0.0f;
public:
	void Init() override;
	void Uninit() override;
	void Update() override;
	void Draw() override;
};