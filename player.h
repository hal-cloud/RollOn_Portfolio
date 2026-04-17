#pragma once

#include "main.h"
#include"renderer.h"
#include "gameObject.h"
#include "input.h"
#include "component.h"
#include "playerInput.h"

#include <string>

class Player : public GameObject
{
private:

public:
	void Init();
	void Uninit(void) override;
	void Update() override;
	void Draw() override;

	void SetAutoPlay();

	void SetComponents() override
	{
		AddComponent<PlayerInput>();
	}
};