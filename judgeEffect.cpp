#include "judgeEffect.h"
#include "texture.h"
#include "polygon.h"
#include "manager.h"
#include "scene.h"

void JudgeEffect::Init(float posx, float posy, float width, float height, wchar_t* fileName)
{
	m_laneEffect = Manager::GetScene()->AddGameObject<Polygon2D>(LAYER_UI, posx, posy, width, height, (wchar_t*)fileName);
}

void JudgeEffect::Uninit()
{
	m_laneEffect->SetDestroy();
}

void JudgeEffect::Update()
{
	m_lifeTime--;
	m_laneEffect->SetPosition(m_laneEffect->GetPosition() + Vector3(0.0f, 0.5f, 0.0f));
	m_laneEffect->SetGamma(static_cast<float>(m_lifeTime) / 10.0f);
	if (m_lifeTime <= 0)
	{
		SetDestroy();
	}
}