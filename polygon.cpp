#include "main.h"
#include "renderer.h"
#include "polygon.h"
#include "texture.h"

void Polygon2D::Init(int posx, int posy,int width,int height,const wchar_t* fileName)
{
	m_position.x = posx;
	m_position.y = posy;
	m_position.z = 0.0f;

	VERTEX_3D vertex[4];

	vertex[0].Position = XMFLOAT3(-width / 2, -height / 2, 0.0f);
	vertex[0].Normal = XMFLOAT3(0.0f,0.0f,0.0f);
	vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

	vertex[1].Position = XMFLOAT3(width / 2, -height / 2, 0.0f);
	vertex[1].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[1].TexCoord = XMFLOAT2(1.0f, 0.0f);

	vertex[2].Position = XMFLOAT3(-width / 2, height / 2, 0.0f);
	vertex[2].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[2].TexCoord = XMFLOAT2(0.0f, 1.0f);

	vertex[3].Position = XMFLOAT3(width / 2, height / 2, 0.0f);
	vertex[3].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[3].TexCoord = XMFLOAT2(1.0f, 1.0f);

	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX_3D) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = vertex;

	Renderer::GetDevice()->CreateBuffer(&bd, &sd, m_vertexBuffer.GetAddressOf());

	m_texture = Texture::Load(fileName);
	assert(m_texture);

	Renderer::CreateVertexShader(&m_vertexShader, &m_vertexLayout, "shader\\unlitTextureVS.cso");
	Renderer::CreatePixelShader(&m_pixelShader, "shader\\unlitTexturePS.cso");
}

void Polygon2D::Uninit(void)
{
	//m_vertexBuffer->Release();
	//m_vertexBuffer = nullptr;

	//m_vertexLayout->Release();
	//m_vertexLayout = nullptr;

	//m_vertexShader->Release();
	//m_vertexShader = nullptr;

	//m_pixelShader->Release();
	//m_pixelShader = nullptr;
}

void Polygon2D::Update()
{

}

void Polygon2D::Draw (void)
{
	Renderer::GetDeviceContext()->IASetInputLayout(m_vertexLayout.Get());

	Renderer::GetDeviceContext()->VSSetShader(m_vertexShader.Get(), NULL, 0);
	Renderer::GetDeviceContext()->PSSetShader(m_pixelShader.Get(), NULL, 0);

	Renderer::SetWorldViewProjection2D();

	XMMATRIX world, scale, rotation, translation;
	scale = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
	rotation = XMMatrixRotationZ(m_rotation.z);
	translation = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);

	world = scale * rotation * translation;

	Renderer::SetWorldMatrix(world);

	MATERIAL material;
	material.Diffuse = XMFLOAT4(m_color.x, m_color.y, m_color.z, m_alpha);
	material.TextureEnable = true;
	Renderer::SetMaterial(material);

	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, m_texture.GetAddressOf());

	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	Renderer::GetDeviceContext()->Draw(4, 0);
}

Polygon2D* Polygon2D::SetPixelShaderFromFile(const char* csoPath)
{
    if (!csoPath) return this;

	if (m_pixelShader)
	{
		m_pixelShader->Release();
		m_pixelShader = nullptr;
	}

	Renderer::CreatePixelShader(&m_pixelShader, csoPath);
   
    return this;
}

Polygon2D* Polygon2D::SetPixelShader(ID3D11PixelShader* shader, bool addRef)
{
    if (!shader) return this;

    if (m_pixelShader)
    {
        m_pixelShader->Release();
    }

    m_pixelShader = shader;
    if (addRef)
    {
        m_pixelShader->AddRef();
    }
    return this;
}

Polygon2D* Polygon2D::SetTexture(wchar_t* fileName)
{
    if (!fileName) return this;

    m_texture = Texture::Load(fileName);
    assert(m_texture);
    return this;
}
