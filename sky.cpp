#include "sky.h"
#include "camera.h"
#include "manager.h"
#include "modelRenderer.h"
#include "player.h"
#include "scene.h"


ModelRenderer* Sky::s_modelRenderer = nullptr;

void Sky::Load()
{	
	s_modelRenderer = new ModelRenderer();
	s_modelRenderer->Load("assets\\model\\sky.obj");
}

void Sky::Init()
{
	Renderer::CreateVertexShader(&m_vertexShader, &m_vertexLayout, "shader\\unlitTextureVS.cso");
	Renderer::CreatePixelShader(&m_pixelShader, "shader\\unlitTexturePS.cso");
}

void Sky::Uninit(void)
{
	m_vertexLayout->Release();
	m_vertexShader->Release();
	m_pixelShader->Release();
}

void Sky::Update()
{
	m_position = Manager::GetScene()->GetGameObject<Camera>()->GetPosition(); // Sky‚ÌˆÊ’u‚ðPlayer‚ÌˆÊ’u‚É‡‚í‚¹‚é
}


void Sky::Draw(void)
{
	Renderer::SetUpModelDraw(
		m_vertexLayout,
		m_vertexShader,
		m_pixelShader,
		m_scale,
		m_rotation,
		m_position
	);

	s_modelRenderer->Draw();
}
