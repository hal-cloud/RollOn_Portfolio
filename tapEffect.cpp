//=========================================================
// tapEffect.cpp
// タップエフェクトクラス実装
//=========================================================
#include "tapEffect.h"
#include "manager.h"
#include "scene.h"
#include "texture.h"

//=========================================================
// 共有リソースの定義
//=========================================================
ComPtr<ID3D11Buffer>             TapEffect::s_VertexBuffer  = nullptr;
ComPtr<ID3D11InputLayout>        TapEffect::s_VertexLayout  = nullptr;
ComPtr<ID3D11VertexShader>       TapEffect::s_VertexShader  = nullptr;
ComPtr<ID3D11PixelShader>        TapEffect::s_PixelShader   = nullptr;
ComPtr<ID3D11ShaderResourceView> TapEffect::s_Texture       = nullptr;
int                              TapEffect::s_InstanceCount = 0;

//=========================================================
// 内部ヘルパ・定数
//=========================================================
namespace
{
    // 各円の Y オフセット（地面から浮かせる高さ）
    constexpr float kCircleYOffsets[2] = { 0.2f, 0.25f };

    //-----------------------------------------------------
    // 進行度（0.0〜1.0）に対するスケールを計算する
    // index == 0 : 外側の楕円（X 方向に大きく広がる）
    // index == 1 : 内側の均一円
    //-----------------------------------------------------
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

//=========================================================
// ライフサイクル
//=========================================================

//-----------------------------------------------------
// GPU リソースは最初のインスタンスが生成されたときだけ作成する（フライウェイト）
// 頂点は XZ 平面上の 1×1 の矩形。スケールで大きさを制御する。
//-----------------------------------------------------
void TapEffect::Init(Vector3 pos)
{
	++s_InstanceCount;
	m_position = pos;
	m_currentLifeTime = kEffectMaxTime;

	for (int i = 0; i < 2; ++i)
	{
		m_circles[i].position = pos + Vector3(0.0f, kCircleYOffsets[i], 0.0f);
		m_circles[i].scale    = Vector3(1.0f, 1.0f, 1.0f);
		m_circles[i].alpha    = 1.0f;
	}

	if (!s_Texture)
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
		Renderer::GetDevice()->CreateBuffer(&bd, &sd, s_VertexBuffer.GetAddressOf());

		s_Texture = Texture::Load(L"assets\\texture\\circle.png");
		Renderer::CreateVertexShader(s_VertexShader.GetAddressOf(), s_VertexLayout.GetAddressOf(), "shader\\unlitTextureVS.cso");
		Renderer::CreatePixelShader(s_PixelShader.GetAddressOf(), "shader\\unlitTexturePS.cso");
	}
}

//-----------------------------------------------------
// Uninit
//-----------------------------------------------------
void TapEffect::Uninit()
{
	if (--s_InstanceCount <= 0)  // 最後のインスタンスだけ共有リソースを解放
	{
		s_VertexBuffer.Reset();
		s_VertexLayout.Reset();
		s_VertexShader.Reset();
		s_PixelShader.Reset();
		s_Texture.Reset();
		s_InstanceCount = 0;
	}
}

//=========================================================
// プール用インタフェース
//=========================================================

void TapEffect::Reset()
{
    GameObject::Reset();
    m_currentLifeTime = kEffectMaxTime;
}

//-----------------------------------------------------
// プールから取り出して再利用する際に呼ぶ
// GPU リソースの再生成は行わない（Init 済みのリソースを流用する）
//-----------------------------------------------------
void TapEffect::Reinit(Vector3 pos)
{
    Activate();
    m_position = pos;
    m_currentLifeTime = kEffectMaxTime;

    for (int i = 0; i < 2; ++i)
    {
        m_circles[i].position = pos + Vector3(0.0f, kCircleYOffsets[i], 0.0f);
        m_circles[i].scale    = Vector3(1.0f, 1.0f, 1.0f);
        m_circles[i].alpha    = 1.0f;
    }
}

void TapEffect::ReturnToPool()
{
    Deactivate();
}

//=========================================================
// 毎フレーム更新
//=========================================================

//-----------------------------------------------------
// 進行度（0.0〜1.0）に応じてスケールと透明度を更新する
// 寿命が尽きたら ReturnToPool() でプールに返す
//-----------------------------------------------------
void TapEffect::Update()
{
    if (!m_active) return;

    const float progress = static_cast<float>(kEffectMaxTime - m_currentLifeTime) / kEffectMaxTime;
    const float alpha    = static_cast<float>(m_currentLifeTime) / kEffectMaxTime;

    for (int i = 0; i < 2; ++i)
    {
        m_circles[i].alpha = alpha;
        m_circles[i].scale = CalcCircleScale(progress, i);
    }

    m_currentLifeTime--;
    if (m_currentLifeTime <= 0)
    {
        ReturnToPool();
    }
}

//=========================================================
// 描画
//=========================================================

//-----------------------------------------------------
// 1 本の円を描画するヘルパ
// マテリアルの Diffuse.w に gamma を渡してフェードを表現する
//-----------------------------------------------------
void TapEffect::DrawCircle(const CircleVisual& c)
{
    MATERIAL mat;
    mat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, c.alpha);
    mat.TextureEnable = true;
    Renderer::SetMaterial(mat);

    Renderer::SetUpModelDraw(
        s_VertexLayout.Get(), s_VertexShader.Get(), s_PixelShader.Get(),
        c.scale, Vector3(0, 0, 0), c.position);

    UINT stride = sizeof(VERTEX_3D), offset = 0;
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, s_VertexBuffer.GetAddressOf(), &stride, &offset);
    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, s_Texture.GetAddressOf());
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    Renderer::GetDeviceContext()->Draw(4, 0);
}

//-----------------------------------------------------
// 深度テストを無効にして 2 本の円を描画する
// 深度テストを無効にしないとノートの裏に隠れることがある
//-----------------------------------------------------
void TapEffect::Draw()
{
    if (!m_active) return;

    Renderer::SetDepthEnable(false);

    Renderer::GetDeviceContext()->IASetInputLayout(s_VertexLayout.Get());
    for (int i = 0; i < 2; ++i)
    {
        DrawCircle(m_circles[i]);
    }

    Renderer::SetDepthEnable(true);
}