#pragma once
#include <array>
#include "polygon.h"
#include "scene.h"

class DifficultyStars
{
public:
    void Init(Scene* scene);
    void UpdateStars(int starCount);

private:
    Scene* m_scene = nullptr;
    std::array<Polygon2D*, 5> m_stars{};
};