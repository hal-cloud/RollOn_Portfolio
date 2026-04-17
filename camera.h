#pragma once

#include "main.h"
#include "renderer.h"
#include <DirectXMath.h>
#include "gameObject.h"
#include <random>

class Camera : public GameObject
{
private:

    XMMATRIX m_viewMatrix;
    XMMATRIX m_projectionMatrix;

    Vector3 m_target;
    float m_distance = 0.0f;

    bool m_isShaking = false;
    float m_shakeDuration = 0.0f;
    float m_shakeElapsed = 0.0f;
    float m_shakeMagnitude = 0.0f;
    std::mt19937 m_randomEngine;
    std::uniform_real_distribution<float> m_shakeDist{ -1.0f, 1.0f };
    float m_shakeTime = 0.0f;

    float m_zoomFov = 1.0f;
    float m_defaultFov = 1.0f;

	bool m_isRocking = false;
public:

    void Init();
    void Uninit(void) override;
    void Update() override;
    void Draw() override;
    XMMATRIX GetViewMatrix() const { return m_viewMatrix; }
    XMMATRIX GetProjectionMatrix() const { return m_projectionMatrix; }

    void SetZoomFov(float fov) { m_zoomFov = fov; }
    float GetZoomFov() const { return m_zoomFov; }
    void ResetZoom() { m_zoomFov = m_defaultFov; }

    Vector3 GetTarget() { return m_target; }

    bool GetIsRocking() { return m_isRocking; }
};
