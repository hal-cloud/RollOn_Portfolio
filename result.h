#pragma once
#include "scene.h"
#include "uiManager.h"

class Result : public Scene
{
private:
	float m_masterVolume = 1.0f;
	UIPanel m_resultPanel;
public:
	void Init() override;
	void Update() override;
};