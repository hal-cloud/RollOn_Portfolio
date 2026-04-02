#pragma once
#include "scene.h"
#include "polygon.h"
#include "fade.h"

class Title : public Scene
{
private:
	float m_masterVolume = 1.0f;

	Fade* m_fade = nullptr;
	Polygon2D* m_backGround = nullptr;
	Polygon2D* m_buttonStart = nullptr;
	bool m_isStartRequested = false;
public:
	void Init()override;
	void Update()override;
};