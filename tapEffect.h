#pragma once
#include "main.h"
#include "renderer.h"
#include "gameObject.h"

class TapEffect : public GameObject
{
private:
	struct CircleVisual
	{
		Vector3 position{ 0, 0, 0 };
		Vector3 scale{ 1, 1, 1 };
		float   gamma = 1.0f;
	};

	static ID3D11Buffer* s_VertexBuffer;
	static ID3D11InputLayout* s_VertexLayout;
	static ID3D11VertexShader* s_VertexShader;
	static ID3D11PixelShader* s_PixelShader;
	static ID3D11ShaderResourceView* s_Texture;
	static int s_InstanceCount;

	CircleVisual m_circles[2];

	int m_currentLifeTime = kEffectMaxTime;
	static constexpr int kEffectMaxTime = 30;
	void DrawCircle(const CircleVisual& c);

public:
	void Init(Vector3 pos);
	void Uninit() override;
	void Update() override;
	void Draw() override;

	void Reset() override;
	void Reinit(Vector3 pos);
	void ReturnToPool();
};