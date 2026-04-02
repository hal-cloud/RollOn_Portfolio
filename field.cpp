#include "field.h"
#include "texture.h"

namespace
{
	constexpr int FIELD_VERTEX_COUNT = 4;
}

void Field::Init(Vector3 pos, float width, float height, const wchar_t* fileName)
{
	m_scale = Vector3(1.0f, 1.0f, 1.0f);
	m_position = pos;
	m_width = width;
	m_height = height;

	const float halfWidth = static_cast<float>(width) * 0.5f;
	const float halfHeight = static_cast<float>(height) * 0.5f;

	m_vertices[0].Position = XMFLOAT3(-halfWidth, 0.0f, halfHeight);
	m_vertices[0].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_vertices[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_vertices[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

	m_vertices[1].Position = XMFLOAT3(halfWidth, 0.0f, halfHeight);
	m_vertices[1].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_vertices[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_vertices[1].TexCoord = XMFLOAT2(1.0f, 0.0f);

	m_vertices[2].Position = XMFLOAT3(-halfWidth, 0.0f, -halfHeight);
	m_vertices[2].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_vertices[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_vertices[2].TexCoord = XMFLOAT2(0.0f, 1.0f);

	m_vertices[3].Position = XMFLOAT3(halfWidth, 0.0f, -halfHeight);
	m_vertices[3].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_vertices[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_vertices[3].TexCoord = XMFLOAT2(1.0f, 1.0f);

	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX_3D) * FIELD_VERTEX_COUNT;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = m_vertices;

	Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_vertexBuffer);

	m_texture = Texture::Load(fileName);
	assert(m_texture);

	Renderer::CreateVertexShader(&m_vertexShader, &m_vertexLayout, "shader\\unlitTextureVS.cso");
	Renderer::CreatePixelShader(&m_pixelShader, "shader\\unlitTexturePS.cso");
}

void Field::Uninit(void)
{
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = nullptr;
	}
	if (m_vertexLayout)
	{
		m_vertexLayout->Release();
		m_vertexLayout = nullptr;
	}
	if (m_vertexShader)
	{
		m_vertexShader->Release();
		m_vertexShader = nullptr;
	}
	if (m_pixelShader)
	{
		m_pixelShader->Release();
		m_pixelShader = nullptr;
	}
}

void Field::Update()
{
}

void Field::Draw(void)
{
	if (!m_active) return;

	if (m_additiveBlend)
	{
		Renderer::SetAdditiveBlend(true);
	}

	Renderer::GetDeviceContext()->IASetInputLayout(m_vertexLayout);

	Renderer::SetUpModelDraw(
		m_vertexLayout,
		m_vertexShader,
		m_pixelShader,
		m_scale,
		m_rotation,
		m_position
	);

	MATERIAL material;
	material.Diffuse = XMFLOAT4(m_color.x, m_color.y, m_color.z, m_gamma);
	material.TextureEnable = true;
	Renderer::SetMaterial(material);

	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_texture);

	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	Renderer::GetDeviceContext()->Draw(FIELD_VERTEX_COUNT, 0);

	if (m_additiveBlend)
	{
		Renderer::SetAdditiveBlend(false);
	}
}

void Field::SetVertexColor(int index, const XMFLOAT4& color)
{
	if (index < 0 || index >= FIELD_VERTEX_COUNT)
	{
		return;
	}

	m_vertices[index].Diffuse = color;
	UpdateVertexBuffer();
}

void Field::SetVertexColors(const std::array<XMFLOAT4, FIELD_VERTEX_COUNT>& colors)
{
	for (int i = 0; i < FIELD_VERTEX_COUNT; ++i)
	{
		m_vertices[i].Diffuse = colors[i];
	}
	UpdateVertexBuffer();
}

void Field::UpdateVertexBuffer()
{
	if (!m_vertexBuffer)
	{
		return;
	}

	Renderer::GetDeviceContext()->UpdateSubresource(m_vertexBuffer, 0, nullptr, m_vertices, 0, 0);
}

void Field::SetPixelShader(const char* pixelShaderCsoPath)
{
	if (!pixelShaderCsoPath)
	{
		return;
	}

	if (m_pixelShader)
	{
		m_pixelShader->Release();
		m_pixelShader = nullptr;
	}

	Renderer::CreatePixelShader(&m_pixelShader, pixelShaderCsoPath);
}