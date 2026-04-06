#pragma once
#include "renderer.h"
#include "gameObject.h"

struct PARTICLEDATA
{
    Vector3  Position;
    Vector3  Velocity;
    float    Alpha;
    XMFLOAT4 Color;
    int      LifeTime;
    int      MaxLifeTime;
};

enum class ParticleType
{
    Default,
    Miss,
    Good,
    Perfect
};

enum class EmitterType
{
    Persistent,
    Timed,
    OneShot
};

class ParticleBase : public GameObject
{
public:
    struct Config
    {
        int            maxParticles    = 20;
        int            maxParticleLife = 60;
        int            emitterLifeTime = 20;
        float          drawOffsetY     = 0.3f;
        const char*    computeShader   = "shader\\particleCS.cso";
        const wchar_t* texture         = L"assets\\texture\\star.png";
        EmitterType    emitterType     = EmitterType::Timed;
    };

    void InitBase(const Config& cfg);
    void UninitBase();
    void UpdateBase();
    void DrawBase();

    void         SetType(ParticleType type) { m_type = type; }
    ParticleType GetType() const            { return m_type; }

protected:
    virtual int     RandomLife();
    virtual Vector3 RandomVelocity();

    bool m_enable = true;

private:
    ComPtr<ID3D11Buffer>              m_vertexBuffer;
    ComPtr<ID3D11Buffer>              m_positionBuffer;
    ComPtr<ID3D11ShaderResourceView>  m_positionSRV;
    ComPtr<ID3D11UnorderedAccessView> m_positionUAV;
    ComPtr<ID3D11InputLayout>         m_vertexLayout;
    ComPtr<ID3D11VertexShader>        m_vertexShader;
    ComPtr<ID3D11PixelShader>         m_pixelShader;
    ComPtr<ID3D11ComputeShader>       m_computeShader;
    ComPtr<ID3D11ShaderResourceView>  m_texture;
    ComPtr<ID3D11Buffer>              m_computeConstantBuffer;

    std::vector<PARTICLEDATA> m_particles;
    int          m_maxParticles       = 20;
    int          m_maxParticleLife    = 60;
    int          m_emitterLifeTime    = 20;
    float        m_drawOffsetY        = 0.3f;
    bool         m_emitterExpired     = false;
    int          m_framesSinceExpired = 0;
    ParticleType m_type               = ParticleType::Default;
    EmitterType  m_emitterType        = EmitterType::Timed;
};