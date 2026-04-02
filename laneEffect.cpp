#include "laneEffect.h"
#include "texture.h"
#include "manager.h"
#include "scene.h"

ID3D11InputLayout*      LaneEffect::s_vertexLayout = nullptr;
ID3D11VertexShader*       LaneEffect::s_vertexShader = nullptr;
ID3D11PixelShader*      LaneEffect::s_pixelShader  = nullptr;
ID3D11ShaderResourceView* LaneEffect::s_texture      = nullptr;
int LaneEffect::s_refCount = 0;

namespace
{
	constexpr int kMaxLifeTime = 30;
}

void LaneEffect::RebuildVertices(float width, float height)
{
	const float hw = width  * 0.5f;
	const float hh = height * 0.5f;

	m_vertices[0] = { XMFLOAT3(-hw, 0, hh), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(0,0) };
	m_vertices[1] = { XMFLOAT3( hw, 0, hh), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(1,0) };
	m_vertices[2] = { XMFLOAT3(-hw, 0,-hh), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(0,1) };
	m_vertices[3] = { XMFLOAT3( hw, 0,-hh), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(1,1) };

	if (m_vertexBuffer)
	{
		Renderer::GetDeviceContext()->UpdateSubresource(m_vertexBuffer, 0, nullptr, m_vertices, 0, 0);
	}
}

void LaneEffect::Init(Vector3 pos, float width, float height, Vector3 color)
{
	m_position = pos;
	m_color    = color * 10;
	m_gamma    = 1.0f;
	m_lifeTime = kMaxLifeTime;

	if (s_refCount == 0)
	{
		s_texture = Texture::Load(L"assets\\texture\\white.png");
		Renderer::CreateVertexShader(&s_vertexShader, &s_vertexLayout, "shader\\unlitTextureVS.cso");
		Renderer::CreatePixelShader(&s_pixelShader, "shader\\laneEffectPS.cso");
	}
	++s_refCount;

	if (!m_vertexBuffer)
	{
		RebuildVertices(width, height);

		D3D11_BUFFER_DESC bd{};
		bd.Usage     = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(VERTEX_3D) * 4;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA sd{};
		sd.pSysMem = m_vertices;
		Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_vertexBuffer);
	}
	else
	{
		RebuildVertices(width, height);
	}
}

void LaneEffect::Uninit()
{
	// インスタンス固有リソースの解放
	if (m_vertexBuffer) { m_vertexBuffer->Release(); m_vertexBuffer = nullptr; }

	// 共有リソースの解放
	--s_refCount;
	if (s_refCount <= 0)
	{
		if (s_vertexLayout) { s_vertexLayout->Release(); s_vertexLayout = nullptr; }
		if (s_vertexShader) { s_vertexShader->Release(); s_vertexShader = nullptr; }
		if (s_pixelShader)  { s_pixelShader->Release();  s_pixelShader  = nullptr; }
		s_texture = nullptr;
		s_refCount = 0;
	}
}

void LaneEffect::Reset()
{
	GameObject::Reset();
	m_lifeTime = kMaxLifeTime;
	m_gamma = 1.0f;
}

void LaneEffect::Reinit(Vector3 pos, float width, float height, Vector3 color)
{
	Activate();
	m_position = pos;
	m_color    = color * 10;
	m_gamma    = 1.0f;
	m_lifeTime = kMaxLifeTime;
	RebuildVertices(width, height);
}

void LaneEffect::ReturnToPool()
{
	Deactivate();
}

void LaneEffect::Update()
{
	if (!m_active) return;

	m_lifeTime--;
	m_gamma = static_cast<float>(m_lifeTime) / 10.0f;
	if (m_lifeTime <= 0)
	{
		ReturnToPool();
	}
}

void LaneEffect::Draw()
{
	if (!m_active) return;

	Renderer::SetDepthEnable(false);
	Renderer::SetAdditiveBlend(true);

	Renderer::GetDeviceContext()->IASetInputLayout(s_vertexLayout);

	Renderer::SetUpModelDraw(
		s_vertexLayout, s_vertexShader, s_pixelShader,
		m_scale, m_rotation, m_position);

	MATERIAL mat;
	mat.Diffuse = XMFLOAT4(m_color.x, m_color.y, m_color.z, m_gamma);
	mat.TextureEnable = true;
	Renderer::SetMaterial(mat);

	UINT stride = sizeof(VERTEX_3D), offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &s_texture);
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	Renderer::GetDeviceContext()->Draw(4, 0);

	Renderer::SetAdditiveBlend(false);
	Renderer::SetDepthEnable(true);
}