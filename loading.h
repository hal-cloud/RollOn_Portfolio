#pragma once
#include "scene.h"
#include "polygon.h"
#include "fade.h"

class Loading : public Scene
{
private:
	float m_masterVolume = 1.0f;

	Polygon2D* m_backGround = nullptr;
	Polygon2D* m_buttonStart = nullptr;

	float m_buttonRotation = 0.0f;
	bool m_finish = false;

public:
	void Init()override;
	void Update()override;
};