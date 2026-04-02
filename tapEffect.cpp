#include "tapEffect.h"
#include "manager.h"
#include "scene.h"
#include "texture.h"

ID3D11Buffer* TapEffect::s_VertexBuffer = nullptr;
ID3D11InputLayout* TapEffect::s_VertexLayout = nullptr;
ID3D11VertexShader* TapEffect::s_VertexShader = nullptr;
ID3D11PixelShader* TapEffect::s_PixelShader = nullptr;
ID3D11ShaderResourceView* TapEffect::s_Texture = nullptr;
int TapEffect::s_InstanceCount = 0;

namespace
{
	constexpr float kCircleYOffsets[2] = { 0.2f, 0.25f };

	inline Vector3 CalcCircleScale(float progress, int index)
	{
		if (index == 0)
		{
			return Vector3(
				progress * 0.8f + 0.2f,
				progress * 0.6f + 0.4f,
				progress * 0.6f + 0.4f) * 2.25f;
		}
		else
		{
			float s = progress * 0.9f + 0.1f;
			return Vector3(s, s, s) * 1.75f;
		}
	}
}

void TapEffect::Init(Vector3 pos)
{
	m_position = pos;
	m_currentLifeTime = kEffectMaxTime;

	for (int i = 0; i < 2; ++i)
	{
		m_circles[i].position = pos + Vector3(0.0f, kCircleYOffsets[i], 0.0f);
		m_circles[i].scale    = Vector3(1.0f, 1.0f, 1.0f);
		m_circles[i].gamma    = 1.0f;
	}

	if (s_InstanceCount == 0)
	{
		VERTEX_3D verts[4];
		const float hw = 0.5f, hh = 0.5f;
		verts[0] = { XMFLOAT3(-hw, 0, hh), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(0,0) };
		verts[1] = { XMFLOAT3( hw, 0, hh), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(1,0) };
		verts[2] = { XMFLOAT3(-hw, 0,-hh), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(0,1) };
		verts[3] = { XMFLOAT3( hw, 0,-hh), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(1,1) };

		D3D11_BUFFER_DESC bd{};
		bd.Usage     = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(VERTEX_3D) * 4;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA sd{};
		sd.pSysMem = verts;
		Renderer::GetDevice()->CreateBuffer(&bd, &sd, &s_VertexBuffer);

		s_Texture = Texture::Load(L"assets\\texture\\circle.png");
		Renderer::CreateVertexShader(&s_VertexShader, &s_VertexLayout, "shader\\unlitTextureVS.cso");
		Renderer::CreatePixelShader(&s_PixelShader, "shader\\unlitTexturePS.cso");
	}

	++s_InstanceCount;
}

void TapEffect::Uninit()
{
	--s_InstanceCount;
	if (s_InstanceCount <= 0)
	{
		if (s_VertexBuffer) { s_VertexBuffer->Release(); s_VertexBuffer = nullptr; }
		if (s_VertexLayout) { s_VertexLayout->Release(); s_VertexLayout = nullptr; }
		if (s_VertexShader) { s_VertexShader->Release(); s_VertexShader = nullptr; }
		if (s_PixelShader) { s_PixelShader->Release(); s_PixelShader = nullptr; }
		s_Texture = nullptr;
	}
}

void TapEffect::Reset()
{
	GameObject::Reset();
	m_currentLifeTime = kEffectMaxTime;
}

void TapEffect::Reinit(Vector3 pos)
{
	Activate();
	m_position = pos;
	m_currentLifeTime = kEffectMaxTime;

	for (int i = 0; i < 2; ++i)
	{
		m_circles[i].position = pos + Vector3(0.0f, kCircleYOffsets[i], 0.0f);
		m_circles[i].scale    = Vector3(1.0f, 1.0f, 1.0f);
		m_circles[i].gamma    = 1.0f;
	}
}

void TapEffect::ReturnToPool()
{
	Deactivate();
}

void TapEffect::Update()
{
	if (!m_active) return;

	const float progress = static_cast<float>(kEffectMaxTime - m_currentLifeTime) / kEffectMaxTime;
	const float alpha    = static_cast<float>(m_currentLifeTime) / kEffectMaxTime;

	for (int i = 0; i < 2; ++i)
	{
		m_circles[i].gamma = alpha;
		m_circles[i].scale = CalcCircleScale(progress, i);
	}

	m_currentLifeTime--;
	if (m_currentLifeTime <= 0)
	{
		ReturnToPool();
	}
}

void TapEffect::DrawCircle(const CircleVisual& c)
{
	MATERIAL mat;
	mat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, c.gamma);
	mat.TextureEnable = true;
	Renderer::SetMaterial(mat);

	Renderer::SetUpModelDraw(
		s_VertexLayout, s_VertexShader, s_PixelShader,
		c.scale, Vector3(0, 0, 0), c.position);

	UINT stride = sizeof(VERTEX_3D), offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &s_VertexBuffer, &stride, &offset);
	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &s_Texture);
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	Renderer::GetDeviceContext()->Draw(4, 0);
}

void TapEffect::Draw()
{
	if (!m_active) return;

	Renderer::SetDepthEnable(false);

	Renderer::GetDeviceContext()->IASetInputLayout(s_VertexLayout);
	for (int i = 0; i < 2; ++i)
	{
		DrawCircle(m_circles[i]);
	}

	Renderer::SetDepthEnable(true);
}