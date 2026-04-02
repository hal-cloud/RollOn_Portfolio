#include "camera.h"
#include "player.h"
#include "manager.h"
#include "input.h"
#include "scene.h"
#include "soundManager.h"
#include <cmath>
#include <DirectXMath.h>
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")

using namespace DirectX;

void Camera::Init()
{
    m_position = { 0.0f,5.0f,-3.0f };
    m_target = { 0.0f,0.0f,3.5f };
}

void Camera::Uninit(void)
{

}

void Camera::Update()
{

}

void Camera::Draw()
{
    m_projectionMatrix = XMMatrixPerspectiveFovLH(
        m_zoomFov,
        (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
        1.0f, 1000.0f);

    XMFLOAT3 up = XMFLOAT3{ 0.0f, 1.0f, 0.0f };
    m_viewMatrix = XMMatrixLookAtLH(
        XMLoadFloat3((XMFLOAT3*)&m_position),
        XMLoadFloat3((XMFLOAT3*)&m_target),
        XMLoadFloat3(&up)
    );

    Renderer::SetViewMatrix(m_viewMatrix);
    Renderer::SetProjectionMatrix(m_projectionMatrix);
}

