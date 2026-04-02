#include "common.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    if (Material.TextureEnable)
    {
        float4 color = g_Texture.Sample(g_SamplerState, In.TexCoord);

        const float topStart = 0.0f;
        const float topEnd = 1.0f; 

        float t = saturate((In.TexCoord.y - topStart) / (topEnd - topStart));

        float alphaScale = lerp(0.0f, 0.125f, smoothstep(0.0f, 1.0f, t));

        color.a *= alphaScale;
        color.rgb *= color.a;
        color.rgb *= 1.5;

        outDiffuse = color * In.Diffuse;
    }
    else
    {
        outDiffuse = In.Diffuse;
    }
}