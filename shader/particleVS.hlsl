
#include "common.hlsl"

struct PARTICLEDATA
{
    float3 Position;
    float3 Velocity;
    float Alpha;
    float4 Color;
    int LifeTime;
    int MaxLifeTime;
};

StructuredBuffer<PARTICLEDATA> gParticles : register(t2);

void main(in VS_IN In, out PS_IN Out)
{
    Out.Position = mul(In.Position, World);

    PARTICLEDATA p = gParticles[In.InstanceID];

    Out.Position.xyz += p.Position + p.Velocity;

    Out.Position = mul(Out.Position, View);
    Out.Position = mul(Out.Position, Projection);

    Out.TexCoord = In.TexCoord;

    float4 base = In.Diffuse * Material.Diffuse;
    Out.Diffuse = base * float4(p.Color.rgb, p.Alpha);
}