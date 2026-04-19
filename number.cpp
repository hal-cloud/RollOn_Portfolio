//=========================================================
// number.cpp
// 数値スプライト描画クラス実装
//=========================================================
#include "main.h"
#include "renderer.h"
#include "manager.h"
#include "number.h"
#include <algorithm>
#include <cmath>


//=========================================================
// 値の設定
//=========================================================

//-----------------------------------------------------
// 値を即時反映する（トゥイーンなし）
// トゥイーン中に呼ばれた場合もアニメーションをキャンセルして即時確定する
//-----------------------------------------------------
void Number::SetValue(int value)
{
    m_value        = value;
    m_targetValue  = value;
    m_startValue   = value;
    m_displayValue = static_cast<double>(value);
    m_isTweening   = false;
    m_tweenElapsed  = 0.0f;
    m_tweenDuration = 0.0f;
}

//-----------------------------------------------------
// 現在値から target へ durationSec 秒かけてイージングする
// durationSec <= 0 の場合は即時 SetValue を呼んで完了とする
//-----------------------------------------------------
void Number::TweenTo(int target, float durationSec)
{
    if (durationSec <= 0.0f)
    {
        SetValue(target);
        return;
    }

    m_startValue    = m_value;
    m_targetValue   = target;
    m_tweenDuration = durationSec;
    m_tweenElapsed  = 0.0f;
    m_isTweening    = true;
}

//-----------------------------------------------------
// 現在のターゲット値に delta を加算してトゥイーン開始する
// トゥイーン中に連続して呼んでも目標値が積み上がるため連打に対応できる
//-----------------------------------------------------
void Number::AddSmooth(int delta, float durationSec)
{
    // トゥイーン中は目標値を基点にして加算する（現在の表示値ではない）
    const int base = m_isTweening ? m_targetValue : m_value;
    TweenTo(base + delta, durationSec);
}


//=========================================================
// 更新
//=========================================================

//-----------------------------------------------------
// トゥイーンの進行（毎フレーム呼び出し）
// SmoothStep（3次エルミート補間）でイーズイン・アウトを実現する。
// t >= 1.0f になった時点でトゥイーンを完了し、値を targetValue に確定する。
//-----------------------------------------------------
void Number::Update()
{
    Atlas::Update();

    if (!m_isTweening) return;

    m_tweenElapsed += FRAME_TIME;
    const float t = (m_tweenDuration > 0.0f)
        ? std::min(m_tweenElapsed / m_tweenDuration, 1.0f)
        : 1.0f;

    // SmoothStep: t^2 * (3 - 2t) でイーズイン・アウト
    const float smooth = t * t * (3.0f - 2.0f * t);

    m_displayValue = static_cast<double>(m_startValue) +
                     (static_cast<double>(m_targetValue - m_startValue) * smooth);

    // 小数点以下を四捨五入して描画用の整数値に変換
    m_value = static_cast<int>(std::round(m_displayValue));

    if (t >= 1.0f)
    {
        // トゥイーン完了：目標値に確定してアニメーション終了
        m_isTweening   = false;
        m_value        = m_targetValue;
        m_displayValue = static_cast<double>(m_targetValue);
    }
}


//=========================================================
// 描画
//=========================================================

//-----------------------------------------------------
// 桁ごとにアトラスから UV を切り出して描画する
//
// テクスチャは 0〜9 を5列×2行に並べたアトラスを想定する。
// 各桁を左から順に1枚ずつ Map/Unmap で UV を書き換えながら Draw() する。
// アライメントに応じて先頭桁の X 座標を決定し、桁数 × 幅ぶんずつずらす。
//-----------------------------------------------------
void Number::Draw()
{
    Renderer::GetDeviceContext()->IASetInputLayout(m_vertexLayout);

    // シェーダー設定
    Renderer::GetDeviceContext()->VSSetShader(m_vertexShader, NULL, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_pixelShader, NULL, 0);

    // マトリックス設定（2D 射影行列を使用）
    Renderer::SetWorldViewProjection2D();

    XMMATRIX world, scale, rotation, translation;
    world       = XMMatrixIdentity();
    scale       = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
    rotation    = XMMatrixRotationZ(m_rotation.z);
    translation = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);

    world = scale * rotation * translation;
    Renderer::SetWorldMatrix(world);

    MATERIAL material;
    material.Diffuse        = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    material.TextureEnable  = true;
    Renderer::SetMaterial(material);

    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_texture);
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 表示文字列を生成（0 は "0" で 1 桁表示）
    const std::string s      = (m_value == 0) ? "0" : std::to_string(m_value);
    const int         digits = static_cast<int>(s.size());

    const float w = m_width;
    const float h = m_height;

    // アライメントに応じて先頭桁の X 座標を決定する
    float startX = 0.0f;
    switch (m_align)
    {
    case Align::Left:   startX = 0.0f;                      break;
    case Align::Right:  startX = -(digits - 1) * w;         break;
    case Align::Center:
    default:            startX = -(digits - 1) * 0.5f * w; break;
    }

    // 各桁をアトラスから UV を切り出して描画する
    for (int i = 0; i < digits; i++)
    {
        D3D11_MAPPED_SUBRESOURCE msr;
        Renderer::GetDeviceContext()->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
        VERTEX_3D* vertex = (VERTEX_3D*)msr.pData;

        const float x   = startX + i * w;
        const float y   = 0.0f;
        const int   num = s[i] - '0';

        // アトラスの UV 計算（5列×2行レイアウト）
        const float tw = 1.0f / 5;
        const float th = 1.0f / 5;
        const float tx = (num % 5) * tw; // 列
        const float ty = (num / 5) * th; // 行

        vertex[0].Position = XMFLOAT3(x,     y,     0.0f);
        vertex[0].Normal   = XMFLOAT3(0.0f,  1.0f,  0.0f);
        vertex[0].Diffuse  = XMFLOAT4(1.0f,  1.0f,  1.0f, 1.0f);
        vertex[0].TexCoord = XMFLOAT2(tx,        ty);

        vertex[1].Position = XMFLOAT3(x + w, y,     0.0f);
        vertex[1].Normal   = XMFLOAT3(0.0f,  1.0f,  0.0f);
        vertex[1].Diffuse  = XMFLOAT4(1.0f,  1.0f,  1.0f, 1.0f);
        vertex[1].TexCoord = XMFLOAT2(tx + tw,    ty);

        vertex[2].Position = XMFLOAT3(x,     y + h, 0.0f);
        vertex[2].Normal   = XMFLOAT3(0.0f,  1.0f,  0.0f);
        vertex[2].Diffuse  = XMFLOAT4(1.0f,  1.0f,  1.0f, 1.0f);
        vertex[2].TexCoord = XMFLOAT2(tx,        ty + th);

        vertex[3].Position = XMFLOAT3(x + w, y + h, 0.0f);
        vertex[3].Normal   = XMFLOAT3(0.0f,  1.0f,  0.0f);
        vertex[3].Diffuse  = XMFLOAT4(1.0f,  1.0f,  1.0f, 1.0f);
        vertex[3].TexCoord = XMFLOAT2(tx + tw,    ty + th);

        Renderer::GetDeviceContext()->Unmap(m_vertexBuffer, 0);
        Renderer::GetDeviceContext()->Draw(4, 0);
    }
}