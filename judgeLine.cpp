#include "judgeLine.h"
#include "manager.h"
#include "scene.h"
#include "field.h"

void JudgeLine::Init()
{
	m_judgeLineVisual = Manager::GetScene()->AddGameObject<Field>(LAYER_WORLD, Vector3(0, 0, 0), 8.0f, 0.5f, (wchar_t*)L"assets\\texture\\white.png");
	m_judgeLineVisual->SetPosition(Vector3(0.0f, 0.01f, 0.0f));
	m_judgeLineVisual->SetColor(Vector3(1.0f, 0.8f, 0.8f));
	m_judgeLineVisual->SetGamma(0.6f);
}
void JudgeLine::Uninit(void)
{
	m_judgeLineVisual->SetDestroy();
}
