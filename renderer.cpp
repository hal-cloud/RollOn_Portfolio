
#include "main.h"
#include "renderer.h"
#include <io.h>


D3D_FEATURE_LEVEL       Renderer::s_featureLevel = D3D_FEATURE_LEVEL_11_0;

ID3D11Device*           Renderer::s_device{};
ID3D11DeviceContext*    Renderer::s_deviceContext{};
IDXGISwapChain*         Renderer::s_swapChain{};
ID3D11RenderTargetView* Renderer::s_renderTargetView{};
ID3D11DepthStencilView* Renderer::s_depthStencilView{};

ID3D11Buffer*			Renderer::s_worldBuffer{};
ID3D11Buffer*			Renderer::s_viewBuffer{};
ID3D11Buffer*			Renderer::s_projectionBuffer{};
ID3D11Buffer*			Renderer::s_materialBuffer{};
ID3D11Buffer*			Renderer::s_lightBuffer{};


ID3D11DepthStencilState* Renderer::s_depthStateEnable{};
ID3D11DepthStencilState* Renderer::s_depthStateDisable{};


ID3D11BlendState*		Renderer::s_blendState{};
ID3D11BlendState*		Renderer::s_blendStateATC{};

ID3D11BlendState*		Renderer::s_additiveBlendState = nullptr;



void Renderer::Init()
{
	HRESULT hr = S_OK;

	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = SCREEN_WIDTH;
	swapChainDesc.BufferDesc.Height = SCREEN_HEIGHT;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = GetWindow();
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = TRUE;

	hr = D3D11CreateDeviceAndSwapChain( NULL,
										D3D_DRIVER_TYPE_HARDWARE,
										NULL,
										0,
										NULL,
										0,
										D3D11_SDK_VERSION,
										&swapChainDesc,
										&s_swapChain,
										&s_device,
										&s_featureLevel,
										&s_deviceContext );

	ID3D11Texture2D* renderTarget{};
	s_swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&renderTarget );
	s_device->CreateRenderTargetView( renderTarget, NULL, &s_renderTargetView );
	renderTarget->Release();

	ID3D11Texture2D* depthStencile{};
	D3D11_TEXTURE2D_DESC textureDesc{};
	textureDesc.Width = swapChainDesc.BufferDesc.Width;
	textureDesc.Height = swapChainDesc.BufferDesc.Height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_D16_UNORM;
	textureDesc.SampleDesc = swapChainDesc.SampleDesc;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	s_device->CreateTexture2D(&textureDesc, NULL, &depthStencile);

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
	depthStencilViewDesc.Format = textureDesc.Format;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Flags = 0;
	s_device->CreateDepthStencilView(depthStencile, &depthStencilViewDesc, &s_depthStencilView);
	depthStencile->Release();

	s_deviceContext->OMSetRenderTargets(1, &s_renderTargetView, s_depthStencilView);

	D3D11_VIEWPORT viewport;
	viewport.Width = (FLOAT)SCREEN_WIDTH;
	viewport.Height = (FLOAT)SCREEN_HEIGHT;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	s_deviceContext->RSSetViewports( 1, &viewport );

	D3D11_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID; 
	rasterizerDesc.CullMode = D3D11_CULL_BACK; 
	rasterizerDesc.DepthClipEnable = TRUE; 
	rasterizerDesc.MultisampleEnable = FALSE; 

	ID3D11RasterizerState *rs;
	s_device->CreateRasterizerState( &rasterizerDesc, &rs );

	s_deviceContext->RSSetState( rs );

	D3D11_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	s_device->CreateBlendState( &blendDesc, &s_blendState );

	blendDesc.AlphaToCoverageEnable = TRUE;
	s_device->CreateBlendState( &blendDesc, &s_blendStateATC );

	float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	s_deviceContext->OMSetBlendState(s_blendState, blendFactor, 0xffffffff );

	D3D11_BLEND_DESC additiveBlendDesc = {};
	additiveBlendDesc.AlphaToCoverageEnable = FALSE;
	additiveBlendDesc.IndependentBlendEnable = FALSE;
	additiveBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	additiveBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA; // �\�[�X�̃A���t�@�l���g�p
	additiveBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;      // ���Z����
	additiveBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	additiveBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	additiveBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	additiveBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	additiveBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	s_device->CreateBlendState(&additiveBlendDesc, &s_additiveBlendState);

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask	= D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	depthStencilDesc.StencilEnable = FALSE;

	s_device->CreateDepthStencilState( &depthStencilDesc, &s_depthStateEnable );//�[�x�L���X�e�[�g

	depthStencilDesc.DepthWriteMask	= D3D11_DEPTH_WRITE_MASK_ZERO;
	s_device->CreateDepthStencilState( &depthStencilDesc, &s_depthStateDisable );//�[�x�����X�e�[�g

	s_deviceContext->OMSetDepthStencilState( s_depthStateEnable, NULL );

	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxAnisotropy = 4;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	ID3D11SamplerState* samplerState{};
	s_device->CreateSamplerState( &samplerDesc, &samplerState );

	s_deviceContext->PSSetSamplers(0, 1, &samplerState);

	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.ByteWidth = sizeof(XMFLOAT4X4);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = sizeof(float);

	s_device->CreateBuffer( &bufferDesc, NULL, &s_worldBuffer );
	s_deviceContext->VSSetConstantBuffers( 0, 1, &s_worldBuffer);

	s_device->CreateBuffer( &bufferDesc, NULL, &s_viewBuffer );
	s_deviceContext->VSSetConstantBuffers( 1, 1, &s_viewBuffer );

	s_device->CreateBuffer( &bufferDesc, NULL, &s_projectionBuffer );
	s_deviceContext->VSSetConstantBuffers( 2, 1, &s_projectionBuffer );


	bufferDesc.ByteWidth = sizeof(MATERIAL);

	s_device->CreateBuffer( &bufferDesc, NULL, &s_materialBuffer );
	s_deviceContext->VSSetConstantBuffers( 3, 1, &s_materialBuffer );
	s_deviceContext->PSSetConstantBuffers( 3, 1, &s_materialBuffer );

	bufferDesc.ByteWidth = sizeof(LIGHT);

	s_device->CreateBuffer( &bufferDesc, NULL, &s_lightBuffer );
	s_deviceContext->VSSetConstantBuffers( 4, 1, &s_lightBuffer );
	s_deviceContext->PSSetConstantBuffers( 4, 1, &s_lightBuffer );

	LIGHT light{};
	light.Enable = true;
	light.Direction = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	light.Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	light.Diffuse = XMFLOAT4(1.5f, 1.5f, 1.5f, 1.0f);
	SetLight(light);

	MATERIAL material{};
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	SetMaterial(material);
}

void Renderer::Uninit()
{
	s_worldBuffer->Release();
	s_viewBuffer->Release();
	s_projectionBuffer->Release();
	s_lightBuffer->Release();
	s_materialBuffer->Release();

	s_deviceContext->ClearState();
	s_renderTargetView->Release();
	s_swapChain->Release();
	s_deviceContext->Release();
	s_device->Release();

	s_additiveBlendState->Release();
}

void Renderer::Begin()
{
	float clearColor[4] = { 0.05f, 0.05f, 0.05f, 1.0f };
	s_deviceContext->ClearRenderTargetView( s_renderTargetView, clearColor );
	s_deviceContext->ClearDepthStencilView( s_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Renderer::End()
{
	s_swapChain->Present( 1, 0 );
}

void Renderer::SetDepthEnable( bool Enable )
{
	if( Enable )
		s_deviceContext->OMSetDepthStencilState( s_depthStateEnable, NULL );
	else
		s_deviceContext->OMSetDepthStencilState( s_depthStateDisable, NULL );

}

void Renderer::SetATCEnable( bool Enable )
{
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if (Enable)
		s_deviceContext->OMSetBlendState(s_blendStateATC, blendFactor, 0xffffffff);
	else
		s_deviceContext->OMSetBlendState(s_blendState, blendFactor, 0xffffffff);

}

void Renderer::SetWorldViewProjection2D()
{
	SetWorldMatrix(XMMatrixIdentity());
	SetViewMatrix(XMMatrixIdentity());

	XMMATRIX projection;
	projection = XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f);
	SetProjectionMatrix(projection);
}

void Renderer::SetWorldMatrix(XMMATRIX WorldMatrix)
{
	XMFLOAT4X4 worldf;
	XMStoreFloat4x4(&worldf, XMMatrixTranspose(WorldMatrix));
	s_deviceContext->UpdateSubresource(s_worldBuffer, 0, NULL, &worldf, 0, 0);
}

void Renderer::SetViewMatrix(XMMATRIX ViewMatrix)
{
	XMFLOAT4X4 viewf;
	XMStoreFloat4x4(&viewf, XMMatrixTranspose(ViewMatrix));
	s_deviceContext->UpdateSubresource(s_viewBuffer, 0, NULL, &viewf, 0, 0);
}

void Renderer::SetProjectionMatrix(XMMATRIX ProjectionMatrix)
{
	XMFLOAT4X4 projectionf;
	XMStoreFloat4x4(&projectionf, XMMatrixTranspose(ProjectionMatrix));
	s_deviceContext->UpdateSubresource(s_projectionBuffer, 0, NULL, &projectionf, 0, 0);

}

void Renderer::SetMaterial( MATERIAL Material )
{
	s_deviceContext->UpdateSubresource( s_materialBuffer, 0, NULL, &Material, 0, 0 );
}

void Renderer::SetLight( LIGHT Light )
{
	s_deviceContext->UpdateSubresource(s_lightBuffer, 0, NULL, &Light, 0, 0);
}

void Renderer::CreateVertexShader( ID3D11VertexShader** VertexShader, ID3D11InputLayout** VertexLayout, const char* FileName )
{

	FILE* file;
	long int fsize;

	file = fopen(FileName, "rb");
	assert(file);

	fsize = _filelength(_fileno(file));
	unsigned char* buffer = new unsigned char[fsize];
	fread(buffer, fsize, 1, file);
	fclose(file);

	s_device->CreateVertexShader(buffer, fsize, NULL, VertexShader);


	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 6, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 10, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = ARRAYSIZE(layout);

	s_device->CreateInputLayout(layout,
		numElements,
		buffer,
		fsize,
		VertexLayout);

	delete[] buffer;
}



void Renderer::CreatePixelShader( ID3D11PixelShader** PixelShader, const char* FileName )
{
	FILE* file;
	long int fsize;

	file = fopen(FileName, "rb");
	assert(file);

	fsize = _filelength(_fileno(file));
	unsigned char* buffer = new unsigned char[fsize];
	fread(buffer, fsize, 1, file);
	fclose(file);

	s_device->CreatePixelShader(buffer, fsize, NULL, PixelShader);

	delete[] buffer;
}

void Renderer::CreateComputeShader( ID3D11ComputeShader** ComputeShader, const char* FileName )
{
	FILE* file;
	long int fsize;

	file = fopen(FileName, "rb");
	assert(file);

	fsize = _filelength(_fileno(file));
	unsigned char* buffer = new unsigned char[fsize];
	fread(buffer, fsize, 1, file);
	fclose(file);

	s_device->CreateComputeShader(buffer, fsize, NULL, ComputeShader);

	delete[] buffer;
}

void Renderer::SetAdditiveBlend(bool enable)
{
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if (enable)
	{
		s_deviceContext->OMSetBlendState(s_additiveBlendState, blendFactor, 0xFFFFFFFF);
	}
	else
	{
		s_deviceContext->OMSetBlendState(s_blendState, blendFactor, 0xFFFFFFFF);
	}
}

void Renderer::SetUpModelDraw(
	ID3D11InputLayout* inputLayout,
	ID3D11VertexShader* vertexShader,
	ID3D11PixelShader* pixelShader,
	const Vector3& scale,
	const Vector3& rotation,
	const Vector3& position
) {
	GetDeviceContext()->IASetInputLayout(inputLayout);
	GetDeviceContext()->VSSetShader(vertexShader, NULL, 0);
	GetDeviceContext()->PSSetShader(pixelShader, NULL, 0);

	XMMATRIX world = XMMatrixIdentity();
	XMMATRIX s = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX r = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMMATRIX t = XMMatrixTranslation(position.x, position.y, position.z);
	world = s * r * t;

	SetWorldMatrix(world);
}
