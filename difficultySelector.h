#pragma once
#include <array>
#include "polygon.h"
#include "scene.h"

class DifficultySelector
{
public:
    void Init(Scene* scene);
    void Change(int delta);
    void UpdateVisuals();
    int  Current() const { return m_currentDifficulty; }

private:
    Scene* m_scene = nullptr;
    int   m_currentDifficulty = 1;
    float m_visualDifficulty  = 1.0f;
    std::array<Polygon2D*, 3> m_badges{};
};