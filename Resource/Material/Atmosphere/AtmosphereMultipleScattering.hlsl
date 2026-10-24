#ifndef ATMOSPHERE_MULTIPLE_SCATTERING_HLSL
#define ATMOSPHERE_MULTIPLE_SCATTERING_HLSL
#include "Atmosphere/AtmosphereTransmittance.hlsl"
static const uint ATMOSPHERE_MS_WIDTH = 32u;
static const uint ATMOSPHERE_MS_HEIGHT = 32u;

// Cover the entire gas sphere. Allocate most rows to the exterior atmosphere,
// with extra resolution on both sides of the analytic-density transition.
float atmosphereMsHeight(float v)
{
    const float C = max(_densityTransitionRadius, 1e-6f);
    if (v < 0.25f)
    {
        const float u = 1.0f - v * 4.0f;
        return atmosphereTransitionHeight() - C * u * u;
    }
    const float u = (v - 0.25f) / 0.75f;
    return atmosphereTransitionHeight() + (_atmosphereRadius - C) * u * u;
}
float2 atmosphereMsCoordinates(float height, float mu)
{
    const float C = max(_densityTransitionRadius, 1e-6f);
    const float localHeight = height - atmosphereTransitionHeight();
    const float v = localHeight < 0.0f
        ? 0.25f * (1.0f - sqrt(saturate(-localHeight / C)))
        : 0.25f + 0.75f * sqrt(saturate(localHeight / max(_atmosphereRadius - C, 1e-6f)));
    return float2(0.5f * (clamp(mu, -1.0f, 1.0f) + 1.0f), v);
}
// RGB = mean incident radiance, per unit solar irradiance. No extra 4*pi
// divisor: the spherical average and normalized phase are already accounted for.
float3 sampleAtmosphereMs(uint index, float height, float mu)
{
    const float2 grid = atmosphereMsCoordinates(height, mu) * float2(ATMOSPHERE_MS_WIDTH - 1u, ATMOSPHERE_MS_HEIGHT - 1u);
    const uint2 a = uint2(grid), b = min(a + 1u, uint2(ATMOSPHERE_MS_WIDTH - 1u, ATMOSPHERE_MS_HEIGHT - 1u));
    const float2 f = frac(grid);
    const Texture2D<float4> lut = getTexture(index);
    return max(lerp(lerp(lut.Load(int3(a, 0)).rgb, lut.Load(int3(b.x, a.y, 0)).rgb, f.x),
        lerp(lut.Load(int3(a.x, b.y, 0)).rgb, lut.Load(int3(b, 0)).rgb, f.x), f.y), 0.0f);
}
float3 atmosphereMultipleIncident(float height, float radialDotSun)
{
    if (_multipleScatteringStrength <= 0.0f || _multipleScatteringMaxOrder <= 1u) return 0.0f;
    const float radius = max(_densityReferenceRadius + height, 0.0f);
    const float mu = radius > 1e-3f ? radialDotSun / radius : 0.0f;
    return sampleAtmosphereMs(_multipleScatteringLutIndex, height, mu)
        * atmosphereSunIrradiance() * saturate(_multipleScatteringStrength);
}
#endif
