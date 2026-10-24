#include "PostProcessCommon.hlsl"
#include "Atmosphere/AtmosphereParameters.hlsl"

float bloomWeight(int offset, int radius)
{
    // A 3-sigma cutoff leaves 1.1% at the edge, visible as a box for solar HDR.
    // Fit 6 sigma into the support and taper exactly to zero without clipping radiance.
    const float sigma = max(float(radius) / 6.0f, 0.5f);
    const float inverseVariance = 1.0f / (sigma * sigma);
    const float edgeWeight = exp(-0.5f * radius * radius * inverseVariance);
    return max(exp(-0.5f * offset * offset * inverseVariance) - edgeWeight, 0.0f);
}

float3 blurBloom(Texture2D<float4> source, int2 pixel, int2 direction, bool extractBright)
{
    const int radius = int(clamp(_bloomRadius, 1.0f, 32.0f));
    float3 sum = float3(0, 0, 0);
    float weightSum = 0;
    const float exposure = exp2(_exposureEV);
    for (int i = -radius; i <= radius; ++i)
    {
        float3 color = max(loadClamped(source, pixel + direction * i), 0.0f);
        if (extractBright)
        {
            const float brightness = max(color.r, max(color.g, color.b));
            // Extract once from exposed HDR, retaining unexposed color for final composition.
            const float brightFraction = saturate(1.0f - _bloomThreshold / max(brightness * exposure, 1e-20f));
            color *= brightFraction;
        }
        const float weight = bloomWeight(i, radius);
        sum += color * weight;
        weightSum += weight;
    }
    return sum / weightSum;
}

float4 PSHorizontal(VS_OUTPUT input) : SV_TARGET
{
    const Texture2D<float4> source = getRenderTargetTexture(_frameIndex, 1);
    const int2 pixel = int2(input.position.xy);
    return float4(blurBloom(source, pixel, int2(1, 0), true), 1);
}

float4 PSVertical(VS_OUTPUT input) : SV_TARGET
{
    const Texture2D<float4> source = getRenderTargetTexture(_frameIndex, 0);
    const Texture2D<float4> originalHDR = getRenderTargetTexture(_frameIndex, 1);
    const int2 pixel = int2(input.position.xy);
    const float3 bloom = blurBloom(source, pixel, int2(0, 1), false);
    return float4(max(loadClamped(originalHDR, pixel) + bloom * _bloomStrength, 0.0f), 1);
}
