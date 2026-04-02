
#include "common.hlsl"


Texture2D		g_Texture : register(t0);
SamplerState	g_SamplerState : register(s0);

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    if (Material.TextureEnable)
    {
        float4 color = g_Texture.Sample(g_SamplerState, In.TexCoord);

        color.rgb *= color.a;
        color.rgb *= 1.5;
        outDiffuse = color * In.Diffuse;
    }
    else
    {
        outDiffuse = In.Diffuse;
    }
}

