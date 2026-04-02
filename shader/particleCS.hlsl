
struct PARTICLEDATA
{
    float3 Position;
    float3 Velocity;
    float Alpha;
    float4 Color;
    int LifeTime;
    int MaxLifeTime; 
};

RWStructuredBuffer<PARTICLEDATA> Particles : register(u0);

cbuffer ParticleUpdateBuffer : register(b0)
{
    int MaxParticles;
    int EmitterExpired;
    int ParticleType;
    int Padding;
}

static const uint SEED_MULTIPLIER = 1000;

float Random(uint seed, uint offset)
{
    uint n = seed * 747796405u + 2891336453u + offset;
    n = ((n >> ((n >> 28u) + 4u)) ^ n) * 277803737u;
    return float((n >> 22u) ^ n) / float(0xffffffff);
}

float3 RandomVelocity(uint seed)
{
    return float3(
        (Random(seed, 0) * 50.0 - 25.0) / 300.0,
        (Random(seed, 1) * 50) / 600.0,
        (Random(seed, 2) * 50.0 - 25.0) / 300.0
    );
}

int RandomLife(uint seed)
{
    return int(Random(seed, 3) * 30.0);
}

void RespawnParticle(inout PARTICLEDATA p, uint seed)
{
    p.Position = float3(0.0, 0.0, 0.0);
    p.Velocity = RandomVelocity(seed);
    p.MaxLifeTime = p.LifeTime = RandomLife(seed);
    p.Alpha = 1.0;
}

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.x;

    if (idx >= (uint)MaxParticles)
        return;
    
    PARTICLEDATA p = Particles[idx];

    if (p.LifeTime > 0)
    {
        p.Velocity.x *= 0.95;
        p.Velocity.z *= 0.95;
        p.Velocity.y *= 0.925;
        p.Velocity.y -= 0.001;
        p.Position += p.Velocity;
        
        p.LifeTime--;
    }

    if (p.LifeTime <= 0)
    {
        if (EmitterExpired == 0)
        {
            uint seed = idx * SEED_MULTIPLIER + (uint)(p.MaxLifeTime);
            RespawnParticle(p, seed);
        }
        else
        {
            p.LifeTime = 0;
            p.Alpha = 0.0;
            Particles[idx] = p;
            return;
        }
    }
    
    float lifeRatio = (float)p.LifeTime / (float)p.MaxLifeTime;
    
    switch (ParticleType)
    {
        case 1:
            p.Color = float4(0.0, 0.25 * lifeRatio, 1.0, 1.0);
            break;
        case 2:
            p.Color = float4(1.0, 0.3 * lifeRatio, 0.0, 1.0);
            break;
        case 3:
            p.Color = float4(
                Random(idx, p.LifeTime * 3),
                Random(idx, p.LifeTime * 5),
                Random(idx, p.LifeTime * 7),
                1.0
            );
            break;
        default:
            p.Color = float4(0.25, 0.5, 1.0, 1.0);
            break;
    }
    
    p.Alpha = lifeRatio;
    
    Particles[idx] = p;
}
