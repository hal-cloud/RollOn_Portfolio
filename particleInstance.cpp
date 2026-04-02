#include "particleInstance.h"

int ParticleInstance::RandomLife()
{
    return (rand() % 30) + 0;
}

Vector3 ParticleInstance::RandomVelocity()
{
    return Vector3(
        (rand() % 50 - 25) / 300.0f,
        (rand() % 50)      / 600.0f,
        (rand() % 50 - 25) / 300.0f
    );
}

void ParticleInstance::RespawnParticle(PARTICLEDATA& p)
{
    p.Position    = Vector3(0, 0, 0);
    p.Velocity    = RandomVelocity();
    p.MaxLifeTime = p.LifeTime = RandomLife();
    p.Alpha       = 1.0f;
}