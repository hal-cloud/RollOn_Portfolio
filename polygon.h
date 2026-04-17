#pragma once

#include "main.h"
#include"renderer.h"
#include "gameObject.h"

class Polygon2D : public GameObject
{
private:

protected:
	ID3D11Buffer* m_vertexBuffer;

	ID3D11InputLayout* m_vertexLayout;
	ID3D11VertexShader* m_vertexShader;
	ID3D11PixelShader* m_pixelShader;
	ID3D11ShaderResourceView* m_texture;

	float m_gamma = 1.0f;
	Vector3 m_color = Vector3(1.0f, 1.0f, 1.0f);

public:
	void Init(int posx, int posy,int width,int height,const wchar_t* fileName);
	void Uninit(void) override;
	void Update() override;
	void Draw() override;
	Polygon2D* SetGamma(float gamma) {
		m_gamma = gamma; 
		return this;
	}
	Polygon2D* SetColor(Vector3 color) {
		m_color = color;
		return this;
	};

	Polygon2D* SetPixelShaderFromFile(const char* csoPath);
	Polygon2D* SetPixelShader(ID3D11PixelShader* shader, bool addRef = true);
	Polygon2D* SetTexture(wchar_t* fileName);
};