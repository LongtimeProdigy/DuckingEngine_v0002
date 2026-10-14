#ifndef FINAL_OUTPUT_HLSL
#define FINAL_OUTPUT_HLSL

#include "PostProcessCommon.hlsl"
#include "AtmosphereParameters.hlsl"

cbuffer FinalOutputConstants : register(b2)
{
    uint _sourceTextureIndex;
};

float3 toneMapACES(float3 x)
{
    // Equivalent ACES fit, rearranged to remain finite at high solar radiance.
    x = max(x, 0.0f);
    return saturate((2.51f * x + 0.03f) / (2.43f * x + 0.59f + 0.14f / max(x, 1e-20f)));
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    const Texture2D<float4> renderTexture = getTexture(_sourceTextureIndex);
    const int2 pixel = int2(input.position.xy);
    const float3 originalCol = loadClamped(renderTexture, pixel);
    const float3 exposedHDR = max(originalCol, 0.0f) * exp2(_exposureEV);
    return float4(toneMapACES(exposedHDR), 1);
}

#endif
