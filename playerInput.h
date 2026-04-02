#pragma once
#include "component.h"
#include "input.h"
#include <array>
#include <deque>

class Scene;
class SongScene;

class PlayerInput : public Component
{
private:
	bool m_isAutoPlay = false;

	std::array<std::deque<double>, 4> m_inputBuffer{};
	double m_fallbackTimeSec = 0.0;

	Scene* m_cachedScene = nullptr;
	SongScene* m_cachedSongScene = nullptr;
public:
	void Init();
	void Update();
	void ToggleAutoPlay() { m_isAutoPlay = !m_isAutoPlay; }
};

