//=========================================================
// atlas.h
// スプライト描画基底クラス
//
// テクスチャ付き2Dポリゴン（4頂点）を描画する基底クラス。
// Number など、テクスチャアトラスを使うクラスはこのクラスを継承する。
// 頂点バッファ・シェーダーの生成と解放を責務とし、
// 実際の描画は派生クラスの Draw() が行う。
//=========================================================
#pragma once
#include "main.h"
#include "gameObject.h"
#include "renderer.h"
#include "texture.h"
#include <memory>

class Atlas : public GameObject
{
public:
    //-----------------------------------------------------
    // 初期化・終了
    // Init で頂点バッファ・テクスチャ・シェーダーを生成する。
    //-----------------------------------------------------
    void Init(int posx, int posy, int width, int height, const wchar_t* fileName);
    void Uninit();

protected:
    //-----------------------------------------------------
    // GPU リソース
    // 頂点バッファは D3D11_USAGE_DYNAMIC で作成し、
    // 派生クラスが Map/Unmap で毎フレーム書き換えられるようにする。
    //-----------------------------------------------------
    ComPtr<ID3D11Buffer>             m_vertexBuffer = nullptr;
    ComPtr<ID3D11InputLayout>        m_vertexLayout = nullptr;
    ComPtr<ID3D11VertexShader>       m_vertexShader = nullptr;
    ComPtr<ID3D11PixelShader>        m_pixelShader  = nullptr;
    ComPtr<ID3D11ShaderResourceView> m_texture      = nullptr; // Texture::Load() が所有（Release 不要）

    //-----------------------------------------------------
    // スプライトサイズ
    // 頂点座標の計算と UV オフセットの算出に使用する
    //-----------------------------------------------------
    int m_height = 0;
    int m_width  = 0;
};
