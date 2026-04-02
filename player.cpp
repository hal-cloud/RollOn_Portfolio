#include "player.h"
#include "camera.h"
#include "manager.h"
#include "renderer.h"
#include "soundManager.h"


void Player::Init()
{
	SetComponents();
}

void Player::Uninit(void)
{
}

void Player::Update()
{
	GameObject::Update();
	for (auto& pair : m_components)
	{
		if (pair.second)
		{
			pair.second->Update();
		}
	}
}

void Player::Draw(void)
{
}

void Player::SetAutoPlay()
{
	for (auto& pair : m_components)
	{
		PlayerInput* inputComp = dynamic_cast<PlayerInput*>(pair.second.get());
		if (inputComp)
		{
			inputComp->ToggleAutoPlay();
		}
	}
}