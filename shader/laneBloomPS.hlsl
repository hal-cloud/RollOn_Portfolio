#include "common.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

float Luminance(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float3 SampleBloom(float2 uv, float2 texelSize, float3 tint, float threshold)
{
    static const float2 offsets[5] =
    {
        float2(0.0f, 0.0f),
        float2(1.5f, 0.0f),
        float2(-1.5f, 0.0f),
        float2(0.0f, 1.5f),
        float2(0.0f, -1.5f)
    };

    static const float weights[5] = { 0.4f, 0.15f, 0.15f, 0.15f, 0.15f };

    float3 accum = 0.0f;
    float weightSum = 0.0f;

    [unroll]
    for (int i = 0; i < 5; ++i)
    {
        float3 sampleColor = g_Texture.Sample(g_SamplerState, uv + offsets[i] * texelSize).rgb * tint;
        float sampleLum = Luminance(sampleColor);
        float bloomMask = saturate((sampleLum - threshold) / max(1e-4f, 1.0f - threshold));
        float w = weights[i] * bloomMask;
        accum += sampleColor * w;
        weightSum += w;
    }

    return weightSum > 0.0f ? accum / weightSum : 0.0f;
}

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    if (!Material.TextureEnable)
    {
        outDiffuse = In.Diffuse;
        return;
    }

    float3 tint = In.Diffuse.rgb;
    float tintAlpha = In.Diffuse.a;

    float4 baseSample = g_Texture.Sample(g_SamplerState, In.TexCoord);
    float4 albedo = float4(baseSample.rgb * tint, baseSample.a * tintAlpha);

    uint width, height, levels;
    g_Texture.GetDimensions(0, width, height, levels);
    float2 texelSize = float2(1.0f / max(1.0f, (float) width), 1.0f / max(1.0f, (float) height));

    const float bloomThreshold = 0.75f;
    float3 bloomSample = SampleBloom(In.TexCoord, texelSize, tint, bloomThreshold);
    float luminance = Luminance(albedo.rgb);
    float bloomMask = saturate((luminance - bloomThreshold) / max(1e-4f, 1.0f - bloomThreshold));

    const float bloomStrength = 1.6f;
    float3 color = albedo.rgb + bloomSample * bloomStrength * bloomMask;

    outDiffuse = float4(color, albedo.a);
}