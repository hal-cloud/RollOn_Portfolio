#include "main.h"
#include "renderer.h"
#include "manager.h"
#include "number.h"
#include <algorithm>
#include <cmath>


void Number::SetValue(int value)
{
    m_value = value;
    m_targetValue = value;
    m_startValue = value;
    m_displayValue = static_cast<double>(value);
    m_isTweening = false;
    m_tweenElapsed = 0.0f;
    m_tweenDuration = 0.0f;
}

void Number::TweenTo(int target, float durationSec)
{
    if (durationSec <= 0.0f)
    {
        SetValue(target);
        return;
    }

    m_startValue = m_value;
    m_targetValue = target;
    m_tweenDuration = durationSec;
    m_tweenElapsed = 0.0f;
    m_isTweening = true;
}


void Number::AddSmooth(int delta, float durationSec)
{
    const int base = m_isTweening ? m_targetValue : m_value;
    TweenTo(base + delta, durationSec);
}

void Number::Update()
{
    Atlas::Update();

    if (!m_isTweening)
        return;

    m_tweenElapsed += FRAME_TIME;
    float t = (m_tweenDuration > 0.0f) ? std::min(m_tweenElapsed / m_tweenDuration, 1.0f) : 1.0f;

    float smooth = t * t * (3.0f - 2.0f * t);

    m_displayValue = static_cast<double>(m_startValue) +
                     (static_cast<double>(m_targetValue - m_startValue) * smooth);

    m_value = static_cast<int>(std::round(m_displayValue));

    if (t >= 1.0f)
    {
        m_isTweening = false;
        m_value = m_targetValue;
        m_displayValue = static_cast<double>(m_targetValue);
    }
}

void Number::Draw() 
{
    Renderer::GetDeviceContext()->IASetInputLayout(m_vertexLayout);

    // シェーダー設定
    Renderer::GetDeviceContext()->VSSetShader(m_vertexShader, NULL, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_pixelShader, NULL, 0);

    // マトリックス設定
    Renderer::SetWorldViewProjection2D();

    XMMATRIX world, scale, rotation, translation;
    world = XMMatrixIdentity();
    scale = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
    rotation = XMMatrixRotationZ(m_rotation.z);
    translation = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);

    world = scale * rotation * translation;

    Renderer::SetWorldMatrix(world);

    MATERIAL material;
    material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    material.TextureEnable = true;
    Renderer::SetMaterial(material);

    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_texture);

    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    int value = m_value;

    // 表示文字列を生成（0 も 1 桁で表示）
    std::string s = (value == 0) ? "0" : std::to_string(value);
    int digits = static_cast<int>(s.size());

    float w = m_width;
    float h = m_height;

    float startX = 0.0f;
    switch (m_align)
    {
    case Align::Left:   startX = 0.0f; break;
    case Align::Right:  startX = -(digits - 1) * w; break;
    case Align::Center:
    default:            startX = -(digits - 1) * 0.5f * w; break;
    }

    // 各桁を（連番テクスチャ）描画
    for (int i = 0; i < digits; i++)
    {
        D3D11_MAPPED_SUBRESOURCE msr;
        Renderer::GetDeviceContext()->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
        VERTEX_3D* vertex = (VERTEX_3D*)msr.pData;

        float x = startX + i * w;
        float y = 0.0f;

        int num = s[i] - '0';

        float tw = 1.0f / 5;
        float th = 1.0f / 5;
        float tx = (num % 5) * tw;
        float ty = (num / 5) * th;

        vertex[0].Position = XMFLOAT3(x, y, 0.0f);
        vertex[0].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
        vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[0].TexCoord = XMFLOAT2(tx, ty);

        vertex[1].Position = XMFLOAT3(x + w, y, 0.0f);
        vertex[1].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
        vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[1].TexCoord = XMFLOAT2(tx + tw, ty);

        vertex[2].Position = XMFLOAT3(x, y + h, 0.0f);
        vertex[2].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
        vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[2].TexCoord = XMFLOAT2(tx, ty + th);

        vertex[3].Position = XMFLOAT3(x + w, y + h, 0.0f);
        vertex[3].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
        vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[3].TexCoord = XMFLOAT2(tx + tw, ty + th);

        Renderer::GetDeviceContext()->Unmap(m_vertexBuffer, 0);

        Renderer::GetDeviceContext()->Draw(4, 0);
    }
}
