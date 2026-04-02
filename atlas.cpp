#include "main.h"
#include "renderer.h"
#include "atlas.h"
#include "texture.h"


void Atlas::Init(int posx, int posy, int width, int height, const wchar_t* fileName)
{
	VERTEX_3D vertex[4];

	m_position = Vector3((float)posx, (float)posy, 0.0f);
	m_height = height;
	m_width = width;

	vertex[0].Position = XMFLOAT3(posx - (width / 2), posy - (height / 2), 0.0f);
	vertex[0].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

	vertex[1].Position = XMFLOAT3(posx + (width / 2), posy - (height / 2), 0.0f);
	vertex[1].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[1].TexCoord = XMFLOAT2(1.0f, 0.0f);

	vertex[2].Position = XMFLOAT3(posx - (width / 2), posy + (height / 2), 0.0f);
	vertex[2].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[2].TexCoord = XMFLOAT2(0.0f, 1.0f);

	vertex[3].Position = XMFLOAT3(posx + (width / 2), posy + (height / 2), 0.0f);
	vertex[3].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[3].TexCoord = XMFLOAT2(1.0f, 1.0f);

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX_3D) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = vertex;

	Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_vertexBuffer);

	// テクスチャ読み込み
	m_texture = Texture::Load(fileName);
	assert(m_texture);

	Renderer::CreateVertexShader(&m_vertexShader, &m_vertexLayout, "shader\\unlitTextureVS.cso");
	Renderer::CreatePixelShader(&m_pixelShader, "shader\\unlitTexturePS.cso");
}

void Atlas::Uninit(void)
{
	m_vertexBuffer->Release();
	m_vertexBuffer = nullptr;

	m_vertexLayout->Release();
	m_vertexLayout = nullptr;

	m_vertexShader->Release();
	m_vertexShader = nullptr;

	m_pixelShader->Release();
	m_pixelShader = nullptr;
}
