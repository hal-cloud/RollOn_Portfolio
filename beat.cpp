#include "beat.h"
#include "soundManager.h"
#include "manager.h"
#include "scene.h"
#include "polygon.h"
#include <chrono>
#include <cmath>

using Clock = std::chrono::steady_clock;

void Beat::Init(int bpm)
{
    m_bpm = bpm;
    m_interval = 60.0f / m_bpm;
    m_lastTime = Clock::now();
	m_beatVisual = Manager::GetScene()->AddGameObject<Polygon2D>(LAYER_UI, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 150, 200, 200, (wchar_t*)L"assets\\texture\\star.png");
}

void Beat::Uninit()
{

}

void Beat::Update()
{
    auto now = Clock::now();
    std::chrono::duration<float> elapsed = now - m_lastTime;

    while (elapsed.count() >= m_interval)
    {
        m_lastTime += std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<float>(m_interval));
        elapsed = now - m_lastTime;
        m_currentScale = m_expandScale;
    }

    float dt = std::min(elapsed.count(), kMaxDeltaTime);
    float decay = 1.0f - std::exp(-m_shrinkSpeed * dt);

    m_currentScale = m_currentScale + (m_targetScale - m_currentScale) * decay;
    m_currentScale = std::max(kMinScale, std::min(m_currentScale, kMaxScale));

    if (m_beatVisual) m_beatVisual->SetScale(Vector3(m_currentScale, m_currentScale, 1.0f));
 
}

void Beat::Draw()
{
}

float Beat::GetElapsedFromLastBeat() const
{
    auto now = Clock::now();
    std::chrono::duration<float> elapsed = now - m_lastTime;
    return elapsed.count();
}

float Beat::GetTimeToNextBeat() const
{
    return m_interval - GetElapsedFromLastBeat();
}