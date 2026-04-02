#pragma once

#include "main.h"
#include "renderer.h"
#include "gameObject.h"

class JudgeEffect : public GameObject
{
private:
	class Polygon2D* m_laneEffect = nullptr;
	int m_lifeTime = 60;
public:
	void Init(float posx, float posy, float width, float height, wchar_t* fileName);
	void Uninit() override;
	void Update() override;
};
