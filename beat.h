#pragma once
#include "main.h"
#include "renderer.h"
#include "gameObject.h"
#include <vector>
#include <string>
#include <chrono>

class Beat : public GameObject
{
private:
	static constexpr float kMinScale = 0.1f;
	static constexpr float kMaxScale = 2.0f;
	static constexpr float kMaxDeltaTime = 0.1f;

	int m_bpm = 0;
	float m_timer = 0;
	float m_interval = 0;

	std::chrono::time_point<std::chrono::steady_clock> m_lastTime;

	class Polygon2D* m_beatVisual = nullptr;

	float m_currentScale = 1.0f;
	float m_targetScale = 1.0f;
	float m_expandScale = 1.5f;
	float m_shrinkSpeed = 8.0f;
public:

	void Init(int bpm);
	void Uninit() override;
	void Update() override;
	void Draw() override;
	float GetElapsedFromLastBeat() const;
	float GetTimeToNextBeat() const;
	std::chrono::steady_clock::time_point GetLastBeatTime() const { return m_lastTime; }
};
