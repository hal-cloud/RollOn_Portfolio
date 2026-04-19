//=========================================================
// atlas.cpp
// スプライト描画基底クラス実装
//=========================================================
#include "main.h"
#include "renderer.h"
#include "atlas.h"
#include "texture.h"


//=========================================================
// 初期化
//=========================================================

//-----------------------------------------------------
// 頂点バッファ・テクスチャ・シェーダーを生成する
//
// 4頂点を center 基準で配置する（左上・右上・左下・右下の順）。
// 頂点バッファは D3D11_USAGE_DYNAMIC にすることで、
// 派生クラスが Map/Unmap で毎フレーム UV を書き換えられるようにする。
// テクスチャは Texture::Load() のキャッシュを利用するため
// 同一ファイルを複数インスタンスで共有してもロードは1回だけ行われる。
//-----------------------------------------------------
void Atlas::Init(int posx, int posy, int width, int height, const wchar_t* fileName)
{
	VERTEX_3D vertex[4];

	m_position = Vector3((float)posx, (float)posy, 0.0f);
	m_height   = height;
	m_width    = width;

	// center 基準で4頂点を配置（左上→右上→左下→右下）
	// TriangleStrip で描画するためこの順序が必要
	vertex[0].Position = XMFLOAT3(posx - (width / 2), posy - (height / 2), 0.0f);
	vertex[0].Normal   = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertex[0].Diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

	vertex[1].Position = XMFLOAT3(posx + (width / 2), posy - (height / 2), 0.0f);
	vertex[1].Normal   = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertex[1].Diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[1].TexCoord = XMFLOAT2(1.0f, 0.0f);

	vertex[2].Position = XMFLOAT3(posx - (width / 2), posy + (height / 2), 0.0f);
	vertex[2].Normal   = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertex[2].Diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[2].TexCoord = XMFLOAT2(0.0f, 1.0f);

	vertex[3].Position = XMFLOAT3(posx + (width / 2), posy + (height / 2), 0.0f);
	vertex[3].Normal   = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertex[3].Diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[3].TexCoord = XMFLOAT2(1.0f, 1.0f);

	// 頂点バッファ生成
	// DYNAMIC + CPU_ACCESS_WRITE：派生クラスが Map/Unmap で UV を毎フレーム書き換えるため
	D3D11_BUFFER_DESC bd{};
	bd.Usage          = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth      = sizeof(VERTEX_3D) * 4;
	bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = vertex;

	Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_vertexBuffer);

	// テクスチャ読み込み
	// Texture::Load() はキャッシュ済みの場合は既存ポインタを返すため二重ロードしない
	m_texture = Texture::Load(fileName);
	assert(m_texture); // ロード失敗時はファイルパス・フォーマットを確認すること

	// 非ライティング・テクスチャありシェーダーを使用する
	Renderer::CreateVertexShader(&m_vertexShader, &m_vertexLayout, "shader\\unlitTextureVS.cso");
	Renderer::CreatePixelShader(&m_pixelShader, "shader\\unlitTexturePS.cso");
}


//=========================================================
// 終了
//=========================================================

//-----------------------------------------------------
// 全 GPU リソースを解放する
// テクスチャは Texture クラスが所有するため Release しない
//-----------------------------------------------------
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
