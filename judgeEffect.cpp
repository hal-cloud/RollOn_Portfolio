//=========================================================
// judgeEffect.cpp
// 判定エフェクトクラス 実装
//=========================================================
#include "judgeEffect.h"
#include "texture.h"
#include "polygon.h"
#include "manager.h"
#include "scene.h"

//-----------------------------------------------------
// Init
// 判定結果テクスチャを貼った Polygon2D をシーンに追加し、
// 以降の Update / Uninit でポインタを使い回す
//-----------------------------------------------------
void JudgeEffect::Init(float posx, float posy, float width, float height, wchar_t* fileName)
{
	m_laneEffect = Manager::GetScene()->AddGameObject<Polygon2D>(LAYER_UI, posx, posy, width, height, (wchar_t*)fileName);
}

//-----------------------------------------------------
// Uninit
// Polygon2D の所有権は Scene 側にあるため、
// SetDestroy() で削除を依頼するだけで delete はしない
//-----------------------------------------------------
void JudgeEffect::Uninit()
{
	m_laneEffect->SetDestroy();
}

//-----------------------------------------------------
// Update
//-----------------------------------------------------
void JudgeEffect::Update()
{
	m_lifeTime--;
	m_laneEffect->SetPosition(m_laneEffect->GetPosition() + Vector3(0.0f, 0.5f, 0.0f));
	m_laneEffect->SetAlpha(static_cast<float>(m_lifeTime) / 10.0f);
	if (m_lifeTime <= 0)
	{
		SetDestroy();
	}
}