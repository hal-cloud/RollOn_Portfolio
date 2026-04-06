#include "particleBase.h"
#include "camera.h"
#include "manager.h"
#include "scene.h"
#include "texture.h"

int ParticleBase::RandomLife()
{
    return (rand() % 30) + 1;
}

Vector3 ParticleBase::RandomVelocity()
{
    return Vector3(
        (rand() % 50 - 25) / 300.0f,
        (rand() % 50)      / 600.0f,
        (rand() % 50 - 25) / 300.0f
    );
}

void ParticleBase::InitBase(const Config& cfg)
{
    m_maxParticles    = cfg.maxParticles;
    m_maxParticleLife = cfg.maxParticleLife;
    m_emitterLifeTime = cfg.emitterLifeTime;
    m_drawOffsetY     = cfg.drawOffsetY;
    m_emitterType     = cfg.emitterType;

    VERTEX_3D vertex[4];
    vertex[0] = { XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(0,0) };
    vertex[1] = { XMFLOAT3( 1.0f,  1.0f, 0.0f), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(1,0) };
    vertex[2] = { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(0,1) };
    vertex[3] = { XMFLOAT3( 1.0f, -1.0f, 0.0f), XMFLOAT3(0,1,0), XMFLOAT4(1,1,1,1), XMFLOAT2(1,1) };

    m_scale = Vector3(0.175f, 0.175f, 0.175f);

    {
        D3D11_BUFFER_DESC bd{};
        bd.Usage     = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(VERTEX_3D) * 4;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA sd{ vertex };
        Renderer::GetDevice()->CreateBuffer(&bd, &sd, m_vertexBuffer.GetAddressOf());
    }

    m_texture = Texture::Load(cfg.texture);
    assert(m_texture);

    m_particles.resize(m_maxParticles);
    for (int i = 0; i < m_maxParticles; ++i)
    {
        auto& p       = m_particles[i];
        p.Position    = Vector3(0, 0, 0);
        p.Velocity    = RandomVelocity();
        p.MaxLifeTime = p.LifeTime = RandomLife();
        p.Alpha       = 1.0f;
        p.Color       = XMFLOAT4(1, 1, 1, 1);
    }

    {
        D3D11_BUFFER_DESC bd{};
        bd.Usage               = D3D11_USAGE_DEFAULT;
        bd.ByteWidth           = static_cast<UINT>(sizeof(PARTICLEDATA) * m_maxParticles);
        bd.StructureByteStride = sizeof(PARTICLEDATA);
        bd.BindFlags           = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        bd.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        D3D11_SUBRESOURCE_DATA sd{ m_particles.data() };
        Renderer::GetDevice()->CreateBuffer(&bd, &sd, m_positionBuffer.GetAddressOf());

        D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
        srvd.Format             = DXGI_FORMAT_UNKNOWN;
        srvd.ViewDimension      = D3D11_SRV_DIMENSION_BUFFER;
        srvd.Buffer.NumElements = m_maxParticles;
        Renderer::GetDevice()->CreateShaderResourceView(m_positionBuffer.Get(), &srvd, m_positionSRV.GetAddressOf());

        D3D11_UNORDERED_ACCESS_VIEW_DESC uavd{};
        uavd.Format             = DXGI_FORMAT_UNKNOWN;
        uavd.ViewDimension      = D3D11_UAV_DIMENSION_BUFFER;
        uavd.Buffer.NumElements = m_maxParticles;
        Renderer::GetDevice()->CreateUnorderedAccessView(m_positionBuffer.Get(), &uavd, m_positionUAV.GetAddressOf());
    }

    {
        struct Consts { int MaxParticles, EmitterExpired, ParticleType, Padding; };
        D3D11_BUFFER_DESC bd{};
        bd.Usage     = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(Consts);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        Renderer::GetDevice()->CreateBuffer(&bd, nullptr, m_computeConstantBuffer.GetAddressOf());
    }

    Renderer::CreateVertexShader(&m_vertexShader, &m_vertexLayout, "shader\\particleVS.cso");
    Renderer::CreatePixelShader (&m_pixelShader,                   "shader\\particlePS.cso");
    Renderer::CreateComputeShader(&m_computeShader,                cfg.computeShader);
}

void ParticleBase::UninitBase()
{
    // ComPtr により自動解放されるため、明示的な Release() 呼び出しは不要
    // ただし即時解放が必要な場合は Reset() を呼ぶ
    m_vertexBuffer.Reset();
    m_positionBuffer.Reset();
    m_positionSRV.Reset();
    m_positionUAV.Reset();
    m_vertexLayout.Reset();
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_computeShader.Reset();
    m_computeConstantBuffer.Reset();
}

void ParticleBase::UpdateBase()
{
    if (!m_enable) { SetDestroy(); return; }

    if (!m_emitterExpired)
    {
        switch (m_emitterType)
        {
        case EmitterType::Timed:
            if (--m_emitterLifeTime <= 0)
            {
                m_emitterExpired     = true;
                m_framesSinceExpired = 0;
            }
            break;
        case EmitterType::Persistent:
            break;
        }
    }
    else
    {
        m_framesSinceExpired++;
    }

    struct Consts { int MaxParticles, EmitterExpired, ParticleType, Padding; }
    constants{ m_maxParticles, m_emitterExpired ? 1 : 0,
               static_cast<int>(m_type), 0 };

    auto* ctx = Renderer::GetDeviceContext();
    ctx->UpdateSubresource(m_computeConstantBuffer.Get(), 0, nullptr, &constants, 0, 0);
    ctx->CSSetShader(m_computeShader.Get(), nullptr, 0);
    ctx->CSSetConstantBuffers(0, 1, m_computeConstantBuffer.GetAddressOf());
    ctx->CSSetUnorderedAccessViews(0, 1, m_positionUAV.GetAddressOf(), nullptr);
    ctx->Dispatch((m_maxParticles + 63) / 64, 1, 1);

    ID3D11UnorderedAccessView* nullUAV = nullptr;
    ctx->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
    ctx->CSSetShader(nullptr, nullptr, 0);

    if (m_emitterExpired && m_framesSinceExpired > m_maxParticleLife)
        SetDestroy();
}

void ParticleBase::DrawBase()
{
    auto* ctx = Renderer::GetDeviceContext();
    ctx->IASetInputLayout(m_vertexLayout.Get());
    ctx->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    ctx->PSSetShader(m_pixelShader.Get(),  nullptr, 0);

    Camera& cam   = *Manager::GetScene()->GetGameObject<Camera>();
    XMMATRIX view = cam.GetViewMatrix();
    XMMATRIX inv  = XMMatrixInverse(nullptr, view);
    inv.r[3].m128_f32[0] = inv.r[3].m128_f32[1] = inv.r[3].m128_f32[2] = 0.0f;

    MATERIAL mat{};
    mat.Diffuse       = XMFLOAT4(1, 1, 1, 1);
    mat.TextureEnable = true;
    Renderer::SetMaterial(mat);

    UINT stride = sizeof(VERTEX_3D), offset = 0;
    ID3D11Buffer* vb = m_vertexBuffer.Get();
    ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    ID3D11ShaderResourceView* tex = m_texture.Get();
    ctx->PSSetShaderResources(0, 1, &tex);
    ID3D11ShaderResourceView* srv = m_positionSRV.Get();
    ctx->VSSetShaderResources(2, 1, &srv);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    Renderer::SetDepthEnable(false);
    Renderer::SetAdditiveBlend(true);

    XMMATRIX world = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z)
                   * inv
                   * XMMatrixTranslation(m_position.x, m_position.y + m_drawOffsetY, m_position.z);
    Renderer::SetWorldMatrix(world);

    ctx->DrawInstanced(4, m_maxParticles, 0, 0);

    Renderer::SetAdditiveBlend(false);
    Renderer::SetDepthEnable(true);
}