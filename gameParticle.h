#pragma once
#include "particleBase.h"

class GameParticle : public ParticleBase
{
public:
    void Init() override
    {
        Config cfg;
        cfg.maxParticles    = 75;
        cfg.maxParticleLife = 240;
        cfg.emitterLifeTime = 1;
        cfg.drawOffsetY     = 5.0f;
        cfg.computeShader   = "shader\\gameParticleCS.cso";
        cfg.texture         = L"assets\\texture\\star.png";
        InitBase(cfg);
    }
    void Uninit() override { UninitBase(); }
    void Update() override { UpdateBase(); }
    void Draw()   override { DrawBase();   }

protected:
    int     RandomLife()     override;
    Vector3 RandomVelocity() override;
};