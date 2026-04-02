#include "manager.h"
#include "scene.h"
#include "gameField.h"
#include "field.h"

void GameField::Init()
{
	m_field[0] = Manager::GetScene()->AddGameObject<Field>(LAYER_WORLD, Vector3(0.0f, -1.0f, 0.0f), 17, 17, (wchar_t*)L"assets\\texture\\black.png");
	m_field[0]->SetColor(Vector3(0.1f, 0.2f, 1.0f));
	m_field[0]->SetGamma(0.6f);
	m_field[1] = Manager::GetScene()->AddGameObject<Field>(LAYER_WORLD_EFFECT, Vector3(0.0f, 0.75f, 3.0f), 8, 8, (wchar_t*)L"assets\\texture\\squea.png");
	m_field[1]->SetColor(Vector3(0.1f, 0.3f, 0.9f));
	m_field[2] = Manager::GetScene()->AddGameObject<Field>(LAYER_WORLD_EFFECT, Vector3(0.0f, 0.5f, 3.0f), 10, 10, (wchar_t*)L"assets\\texture\\squea.png");
	m_field[2]->SetColor(Vector3(0.1f, 0.5f, 1.0f));
}

void GameField::Uninit()
{
	for (int i = 0; i < 3; i++)
	{
		m_field[i]->SetDestroy();
	}

}

void GameField::Update()
{
	m_rotation += 0.01;
	if(m_rotation > 360.0f)
	{
		m_rotation -= 360.0f;
	}
	m_field[1]->SetRotation(Vector3(0, m_rotation, 0));
	m_field[2]->SetRotation(Vector3(0, -m_rotation, 0));
}

void GameField::Draw()
{
}
