#pragma once
#include <array>
#include "main.h"
#include "renderer.h"
#include "gameObject.h"

class Field : public GameObject
{
private:
    ID3D11Buffer* m_vertexBuffer = nullptr;

    ID3D11InputLayout* m_vertexLayout;
    ID3D11VertexShader* m_vertexShader;
    ID3D11PixelShader* m_pixelShader;
    ID3D11ShaderResourceView* m_texture;

	float m_width = 1.0f;
	float m_height = 1.0f;

    float m_gamma = 1.0f;
    Vector3 m_color = Vector3(1.0f, 1.0f, 1.0f);
    VERTEX_3D m_vertices[4]{};
    bool m_additiveBlend = false;

    void UpdateVertexBuffer();
public:
    void Init(Vector3 pos, float width, float height,const wchar_t* fileName);
    void Uninit(void) override;
    void Update() override;
    void Draw() override;
    void SetGamma(float gamma) { m_gamma = gamma; }
    void SetColor(Vector3 color) { m_color = color; }
    void SetVertexColor(int index, const XMFLOAT4& color);
    void SetVertexColors(const std::array<XMFLOAT4, 4>& colors);
    void SetPixelShader(const char* pixelShaderCsoPath);
	float GetWidth() const { return m_width; }
	float GetHeight() const { return m_height; }
    Vector3 GetColor() const { return m_color; }
    void SetAdditiveBlend(bool enable) { m_additiveBlend = enable; }
};