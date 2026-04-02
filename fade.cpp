#include "fade.h"

void Fade::Init()
{
	m_fadeState = 0;
	m_fadeAlpha = 1.0f;

	m_polygon = new Polygon2D();
	m_polygon->Init(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT, (wchar_t*)L"assets\\texture\\white.png");
	m_polygon->SetColor({ 0.0f, 0.0f, 0.0f });
	m_polygon->SetGamma(m_fadeAlpha);
}

void Fade::Uninit()
{
	if (m_polygon)
	{
		m_polygon->Uninit();
		delete m_polygon;
		m_polygon = nullptr;
	}
}

void Fade::Update()
{
	if (m_fadeState == 0)
	{
		m_fadeAlpha -= m_fadeSpeed;
		if (m_fadeAlpha <= 0.0f)
		{
			m_fadeAlpha = 0.0f;
			m_fadeState = 1;
		}
		m_polygon->SetGamma(m_fadeAlpha);
	}

	else if (m_fadeState == 2)
	{
		m_fadeAlpha += m_fadeSpeed;
		if (m_fadeAlpha >= 1.0f)
		{
			m_fadeAlpha = 1.0f;
			m_fadeState = 1;
		}
		m_polygon->SetGamma(m_fadeAlpha);
	}
}

void Fade::Draw()
{
	if (m_polygon)
	{
		m_polygon->Draw();
	}
}