#pragma once
#include "particleBase.h"   

class ParticleInstance : public ParticleBase
{
public:
    void Init() override
    {
        Config cfg;
        cfg.maxParticles    = 20;
        cfg.maxParticleLife = 60;
        cfg.emitterLifeTime = 20;
        cfg.drawOffsetY     = 0.3f;
        cfg.computeShader   = "shader\\particleCS.cso";
        cfg.texture         = L"assets\\texture\\star.png";
        InitBase(cfg);
    }
    void Uninit() override { UninitBase(); }
    void Update() override { UpdateBase(); }
    void Draw()   override { DrawBase();   }

    void RespawnParticle(PARTICLEDATA& p);

protected:
    int     RandomLife()     override;
    Vector3 RandomVelocity() override;
};