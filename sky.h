#pragma once

#include "main.h"
#include"renderer.h"
#include "gameObject.h"
#include "input.h"

class Sky : public GameObject
{
private:
	ID3D11InputLayout* m_vertexLayout;
	ID3D11VertexShader* m_vertexShader;
	ID3D11PixelShader* m_pixelShader;

	static class ModelRenderer* s_modelRenderer;
	
public:
	void Init();
	void Uninit(void) override;
	void Update() override;
	void Draw() override;

	static void Load();
};
