//=========================================================
// tapEffect.h
// タップエフェクトクラス
//
// ノートをタップしたときに発生する 2 重円アニメーション。
// GPU リソース（頂点バッファ・シェーダー）を static で共有し、
// インスタンスカウンタで最後のインスタンスが解放するときのみ
// GPU リソースを解放するフライウェイトパターンを採用する。
// オブジェクトプールと組み合わせて Reinit()/ReturnToPool() で再利用する。
//=========================================================
#pragma once
#include "main.h"
#include "renderer.h"
#include "gameObject.h"

class TapEffect : public GameObject
{
private:
	//-----------------------------------------------------
	// 1 本の円を表すデータ
	//-----------------------------------------------------
	struct CircleVisual
	{
		Vector3 position{ 0, 0, 0 };
		Vector3 scale{ 1, 1, 1 };
		float   alpha = 1.0f; // 透明度（0.0 = 完全透明、1.0 = 不透明）
	};

	//-----------------------------------------------------
	// 共有 GPU リソース（全インスタンスで 1 つだけ保持）
	// s_InstanceCount が 0 になったときのみ解放する
	//-----------------------------------------------------
	static ComPtr<ID3D11Buffer>             s_VertexBuffer;
	static ComPtr<ID3D11InputLayout>        s_VertexLayout;
	static ComPtr<ID3D11VertexShader>       s_VertexShader;
	static ComPtr<ID3D11PixelShader>        s_PixelShader;
	static ComPtr<ID3D11ShaderResourceView> s_Texture;
	static int                              s_InstanceCount;

	//-----------------------------------------------------
	// インスタンス固有データ
	//-----------------------------------------------------
	CircleVisual m_circles[2];

	int m_currentLifeTime = kEffectMaxTime;

	//-----------------------------------------------------
	// エフェクト持続フレーム数
	//-----------------------------------------------------
	static constexpr int kEffectMaxTime = 30;

	//-----------------------------------------------------
	// プライベートヘルパ
	// 1 本の円を SetUpModelDraw で描画する
	//-----------------------------------------------------
	void DrawCircle(const CircleVisual& c);

public:
	//-----------------------------------------------------
	// ライフサイクル
	//-----------------------------------------------------
	void Init(Vector3 pos);
	void Uninit() override;
	void Update() override;
	void Draw() override;

	//-----------------------------------------------------
	// プール用インタフェース
	// Reset    : 寿命をリセットし再利用できる状態にする
	// Reinit   : 位置を再設定してアクティブ化する（生成コストなし）
	// ReturnToPool : Deactivate してプールに返却する
	//-----------------------------------------------------
	void Reset() override;
	void Reinit(Vector3 pos);
	void ReturnToPool();
};