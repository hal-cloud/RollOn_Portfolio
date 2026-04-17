#pragma once
#include "main.h"
#include "renderer.h"
#include "gameObject.h"
#include "scene.h"
#include "polygon.h"


class Fade : public GameObject
{
private:
	int m_fadeState = 0;
	float m_fadeAlpha = 1.0f;
	float m_fadeSpeed = 0.02f;
	Polygon2D* m_polygon = nullptr;
public:
	void Init();
	void Uninit() override;
	void Update() override;
	void Draw() override;

	void StartFadeOut() { m_fadeState = 2; }
	void StartFadeIn() { m_fadeState = 0; m_fadeAlpha = 1.0f; }

	void SetFadeSpeed(float speed) { m_fadeSpeed = speed; }
	void SetColor(Vector3 color) { if (m_polygon) m_polygon->SetColor(color); }

	bool IsFading() { return m_fadeState != 1; }
};
