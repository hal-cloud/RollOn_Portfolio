#pragma once

#include "main.h"
#include "renderer.h"
#include "gameObject.h"

class LaneEffect : public GameObject
{
private:
	static ID3D11InputLayout*        s_vertexLayout;
	static ID3D11VertexShader*       s_vertexShader;
	static ID3D11PixelShader*   s_pixelShader;
	static ID3D11ShaderResourceView* s_texture;
	static int s_refCount;

	ID3D11Buffer* m_vertexBuffer = nullptr;
	VERTEX_3D m_vertices[4]{};

	Vector3 m_color{ 1, 1, 1 };
	float   m_gamma = 1.0f;
	int     m_lifeTime = 30;

	void RebuildVertices(float width, float height);

public:
	void Init(Vector3 pos, float width, float height, Vector3 color);
	void Uninit() override;
	void Update() override;
	void Draw() override;

	void Reset() override;
	void Reinit(Vector3 pos, float width, float height, Vector3 color);
	void ReturnToPool();
};
