#pragma once


struct VERTEX_3D
{
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT4 Diffuse;
	XMFLOAT2 TexCoord;
};

struct MATERIAL
{
	XMFLOAT4	Ambient;
	XMFLOAT4	Diffuse;
	XMFLOAT4	Specular;
	XMFLOAT4	Emission;
	float		Shininess;
	BOOL		TextureEnable;
	float		Dummy[2];
};

struct LIGHT
{
	BOOL		Enable;
	BOOL		Dummy[3];
	XMFLOAT4	Direction;
	XMFLOAT4	Diffuse;
	XMFLOAT4	Ambient;

	XMFLOAT4	Position;
	XMFLOAT4	PointLightParam;

	XMFLOAT4	SkyColor;
	XMFLOAT4	GroundColor;
	XMFLOAT4	GroundNormal;

	XMFLOAT4	Angle;
};


class Renderer
{
private:

	static D3D_FEATURE_LEVEL       s_featureLevel;

	static ID3D11Device*           s_device;
	static ID3D11DeviceContext*    s_deviceContext;
	static IDXGISwapChain*         s_swapChain;
	static ID3D11RenderTargetView* s_renderTargetView;
	static ID3D11DepthStencilView* s_depthStencilView;

	static ID3D11Buffer*			s_worldBuffer;
	static ID3D11Buffer*			s_viewBuffer;
	static ID3D11Buffer*			s_projectionBuffer;
	static ID3D11Buffer*			s_materialBuffer;
	static ID3D11Buffer*			s_lightBuffer;

	static ID3D11DepthStencilState* s_depthStateEnable;
	static ID3D11DepthStencilState* s_depthStateDisable;

	static ID3D11BlendState*		s_blendState;
	static ID3D11BlendState*		s_blendStateATC;
	static ID3D11BlendState*		s_additiveBlendState;

public:
	static void Init();
	static void Uninit();
	static void Begin();
	static void End();

	static void SetDepthEnable(bool Enable);
	static void SetATCEnable(bool Enable);
	static void SetWorldViewProjection2D();
	static void SetWorldMatrix(XMMATRIX WorldMatrix);
	static void SetViewMatrix(XMMATRIX ViewMatrix);
	static void SetProjectionMatrix(XMMATRIX ProjectionMatrix);
	static void SetMaterial(MATERIAL Material);
	static void SetLight(LIGHT Light);

	static ID3D11Device* GetDevice( void ){ return s_device; }
	static ID3D11DeviceContext* GetDeviceContext( void ){ return s_deviceContext; }

	static void SetUpModelDraw(
		ID3D11InputLayout* inputLayout,
		ID3D11VertexShader* vertexShader,
		ID3D11PixelShader* pixelShader,
		const Vector3& scale,
		const Vector3& rotation,
		const Vector3& position
	);

	static void CreateVertexShader(ID3D11VertexShader** VertexShader, ID3D11InputLayout** VertexLayout, const char* FileName);
	static void CreatePixelShader(ID3D11PixelShader** PixelShader, const char* FileName);
	static void CreateComputeShader(ID3D11ComputeShader** ComputeShader, const char* FileName);

	static void SetAdditiveBlend(bool enable);

};
