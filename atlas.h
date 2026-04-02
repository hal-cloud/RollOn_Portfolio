// SpriteAtlas.h
#pragma once
#include "gameObject.h"
#include "renderer.h"
#include "texture.h"
#include <memory>

class Atlas : public GameObject
{
public:
    void Init(int posx, int posy, int width, int height, const wchar_t* fileName);
    void Uninit();

protected:
    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11InputLayout* m_vertexLayout = nullptr;
    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11ShaderResourceView* m_texture = nullptr;
    int m_height = 0;
    int m_width = 0;
};
