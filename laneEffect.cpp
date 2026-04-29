//=========================================================
// laneEffect.cpp
// レーンエフェクトクラス 実装
//=========================================================
#include "laneEffect.h"
#include "texture.h"
#include "manager.h"
#include "scene.h"

ComPtr<ID3D11InputLayout>		 LaneEffect::s_vertexLayout = nullptr;
ComPtr<ID3D11VertexShader>       LaneEffect::s_vertexShader = nullptr;
ComPtr<ID3D11PixelShader>        LaneEffect::s_pixelShader  = nullptr;
ComPtr<ID3D11ShaderResourceView> LaneEffect::s_texture      = nullptr;
int								 LaneEffect::s_refCount = 0;

namespace
{
	// エフェクトの生存フレーム数
	// この値を 10 で割った値が初期透明度になるため、kMaxLifeTime / 10 = 3.0f が最大輝度
	constexpr int kMaxLifeTime = 30;
}

//=========================================================
// RebuildVertices
// 頂点座標を再計算して GPU バッファへ反映する
//
// Init 時の初回構築と、Reinit でサイズが変化した際の再構築の両方で呼ばれる。
/// m_vertexBuffer が nullptr の場合は座標の計算のみ行い、
// バッファ生成は Init 側に委ねる。
///=========================================================
void LaneEffect::RebuildVertices(float width, float height)
{
	const float hw = width  * 0.5f;
	const float hh = height * 0.5f;

	m_vertices[0] = { XMFLOAT3(-hw, 0, hh), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(0,0) };
	m_vertices[1] = { XMFLOAT3( hw, 0, hh), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(1,0) };
	m_vertices[2] = { XMFLOAT3(-hw, 0,-hh), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(0,1) };
	m_vertices[3] = { XMFLOAT3( hw, 0,-hh), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(1,1) };

	// バッファが既に存在する場合のみ GPU へ転送する
	if (m_vertexBuffer)
	{
		Renderer::GetDeviceContext()->UpdateSubresource(m_vertexBuffer.Get(), 0, nullptr, m_vertices, 0, 0);
	}
}

//=========================================================
// Init
// インスタンスを初期化し、必要に応じて共有リソースを生成する
// 
// m_vertexBuffer が nullptr の場合は新規に D3D11 バッファを生成し、
// 既に存在する場合は頂点座標だけ更新して再利用する。
///=========================================================
void LaneEffect::Init(Vector3 pos, float width, float height, Vector3 color)
{
	++s_refCount;
	m_position = pos;
	m_color    = color * 10;
	m_alpha    = 1.0f;
	m_lifeTime = kMaxLifeTime;

	if (!s_texture)
	{
		s_texture = Texture::Load(L"assets\\texture\\white.png");
		Renderer::CreateVertexShader(s_vertexShader.GetAddressOf(), s_vertexLayout.GetAddressOf(), "shader\\unlitTextureVS.cso");
		Renderer::CreatePixelShader(s_pixelShader.GetAddressOf(), "shader\\laneEffectPS.cso");
	}

	if (!m_vertexBuffer)
	{
		// 頂点データを計算してから D3D11_USAGE_DEFAULT バッファを生成する
		RebuildVertices(width, height);

		D3D11_BUFFER_DESC bd{};
		bd.Usage     = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(VERTEX_3D) * 4;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA sd{};
		sd.pSysMem = m_vertices;
		Renderer::GetDevice()->CreateBuffer(&bd, &sd, m_vertexBuffer.GetAddressOf());
	}
	else
	{
		// バッファは再利用し、内容だけ更新する
		RebuildVertices(width, height);
	}
}

//=========================================================
// Uninit
///=========================================================
void LaneEffect::Uninit()
{
	m_vertexBuffer.Reset();  // インスタンス固有リソースを解放

	if (--s_refCount <= 0)   // 最後のインスタンスだけ共有リソースを解放
	{
		s_vertexLayout.Reset();
		s_vertexShader.Reset();
		s_pixelShader.Reset();
		s_texture.Reset();
		s_refCount = 0;
	}
}

//=========================================================
// Reset
// プールへ返却する前に寿命と透明度を初期値に戻す
//=========================================================
void LaneEffect::Reset()
{
	GameObject::Reset();
	m_lifeTime = kMaxLifeTime;
	m_alpha = 1.0f;
}

//=========================================================
// Reinit
// プールから取り出した際に位置・サイズ・色を再設定してアクティブ化する
//
// Init と異なり共有リソースの生成は行わない。
// RebuildVertices で頂点バッファの内容だけ上書きする。
///=========================================================
void LaneEffect::Reinit(Vector3 pos, float width, float height, Vector3 color)
{
	Activate();
	m_position = pos;
	m_color    = color * 10; // シェーダー側で発光感を出すために 10 倍して渡す
	m_alpha    = 1.0f;
	m_lifeTime = kMaxLifeTime;
	RebuildVertices(width, height);
}

//=========================================================
// ReturnToPool
// 自身を非アクティブ化してプールに返却する
//=========================================================
void LaneEffect::ReturnToPool()
{
	Deactivate();
}

//=========================================================
// Update
// 毎フレーム寿命を 1 減算し、透明度を線形に減衰させる
//
// m_gamma = lifeTime / 10.0f のため、残り 10 フレームで透明度が 1.0f になる。
// 寿命が 0 以下になったらプールへ返却する。
///=========================================================
void LaneEffect::Update()
{
	if (!m_active) return;

	m_lifeTime--;
	m_alpha = static_cast<float>(m_lifeTime) / 10.0f;
	if (m_lifeTime <= 0)
	{
		ReturnToPool();
	}
}

//=========================================================
// Draw
// 加算合成で板ポリゴンを描画する
//
// 深度テストを無効化してほかのオブジェクトに重なって見えるようにし、
// 加算合成で発光エフェクトを表現する。
// マテリアルの alpha に m_gamma を渡して laneEffectPS.cso 側でフェードアウトする。
///=========================================================
void LaneEffect::Draw()
{
	if (!m_active) return;

	Renderer::SetDepthEnable(false);
	Renderer::SetAdditiveBlend(true);

	Renderer::GetDeviceContext()->IASetInputLayout(s_vertexLayout.Get());

	Renderer::SetUpModelDraw(
		s_vertexLayout.Get(), s_vertexShader.Get(), s_pixelShader.Get(),
		m_scale, m_rotation, m_position);

	// m_gamma をアルファとして渡し、シェーダー側でフェードアウトを制御する
	MATERIAL mat;
	mat.Diffuse = XMFLOAT4(m_color.x, m_color.y, m_color.z, m_alpha);
	mat.TextureEnable = true;
	Renderer::SetMaterial(mat);

	UINT stride = sizeof(VERTEX_3D), offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, s_texture.GetAddressOf());
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	Renderer::GetDeviceContext()->Draw(4, 0);

	Renderer::SetAdditiveBlend(false);
	Renderer::SetDepthEnable(true);
}