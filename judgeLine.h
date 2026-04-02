#pragma once
#include <array>
#include "main.h"
#include "renderer.h"
#include "gameObject.h"

class JudgeLine : public GameObject
{
private:
	class Field* m_judgeLineVisual = nullptr;
public:
    void Init();
    void Uninit(void) override;
};