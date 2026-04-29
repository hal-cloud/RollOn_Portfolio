//=========================================================
// laneEffect.h
// レーンエフェクトクラス
//
// 入力時にレーン上に発生するエフェクト。
// シェーダー・テクスチャを static で共有し、参照カウンタで
// 共有リソースのライフタイムを管理するフライウェイトパターンを採用。
// オブジェクトプールと組み合わせて Reinit()/ReturnToPool() で再利用する。
//=========================================================
#pragma once

#include "main.h"
#include "renderer.h"
#include "gameObject.h"

class LaneEffect : public GameObject
{
private:
    //-----------------------------------------------------
    // 共有 GPU リソース（全インスタンスで 1 つだけ保持）
    // s_refCount が 0 になったときのみ解放する
    //-----------------------------------------------------
    static ComPtr<ID3D11InputLayout>        s_vertexLayout;
    static ComPtr<ID3D11VertexShader>       s_vertexShader;
    static ComPtr<ID3D11PixelShader>        s_pixelShader;
    static ComPtr<ID3D11ShaderResourceView> s_texture;
    static int                              s_refCount;

    //-----------------------------------------------------
    // インスタンス固有リソース
    // 各インスタンスはレーン幅に合わせた頂点バッファを個別に持つ
    //-----------------------------------------------------
    ComPtr<ID3D11Buffer> m_vertexBuffer = nullptr;
    VERTEX_3D     m_vertices[4]{};

    //-----------------------------------------------------
    // 描画パラメータ
    // m_color は乗算係数として laneEffectPS.cso に渡す（10 倍して発光感を出す）
    // m_gamma は透明度。Update() で線形に減衰させてフェードアウトを表現する
    //-----------------------------------------------------
    Vector3 m_color{ 1, 1, 1 };
    float   m_alpha    = 1.0f;
    int     m_lifeTime = 30;

    //-----------------------------------------------------
    // プライベートヘルパ
    // 頂点座標を再計算して GPU バッファへ反映する
    //-----------------------------------------------------
    void RebuildVertices(float width, float height);

public:
    //-----------------------------------------------------
    // ライフサイクル
    //-----------------------------------------------------
    void Init(Vector3 pos, float width, float height, Vector3 color);
    void Uninit() override;
    void Update() override;
    void Draw() override;

    //-----------------------------------------------------
    // プール用インタフェース
    // Reset       : 寿命と透明度をリセットする
    // Reinit      : 位置・サイズ・色を再設定してアクティブ化する
    // ReturnToPool: Deactivate してプールに返却する
    //-----------------------------------------------------
    void Reset() override;
    void Reinit(Vector3 pos, float width, float height, Vector3 color);
    void ReturnToPool();
};
