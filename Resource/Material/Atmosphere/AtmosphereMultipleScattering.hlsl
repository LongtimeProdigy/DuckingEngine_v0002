#ifndef ATMOSPHERE_MULTIPLE_SCATTERING_HLSL
#define ATMOSPHERE_MULTIPLE_SCATTERING_HLSL

#include "Atmosphere/AtmosphereTransmittance.hlsl"

static const uint ATMOSPHERE_MULTIPLESCATTERING_WIDTH = 32u;
static const uint ATMOSPHERE_MULTIPLESCATTERING_HEIGHT = 32u;

// atmosphereMultipleScatteringHeightAtReference의 역함수
float2 atmosphereMultipleScatteringCoordinates(float heightAtReference, float mu)
{
    // runtime에 sqrt가 사용되는거면 굳이 atmosphereMultipleScatteringHeightAtReference에서 가까운 거리를 촘촘하게 하는 제곱처리를 제거 고려하는것도?
    GPUASSERT(_densityTransitionRadius > 1e-6f);
    GPUASSERT(_atmosphereRadius - _densityTransitionRadius > 1e-6f);
    GPUASSERT(abs(mu) <= 1, float4(mu, 0, 0, 0));
    const float heightAtTransition = heightAtReference - atmosphereTransitionHeight();
    const float v = heightAtTransition < 0.0f
        ? 0.25f * (1.0f - sqrt(saturate(-heightAtTransition / _densityTransitionRadius)))
        : 0.25f + 0.75f * sqrt(saturate(heightAtTransition / (_atmosphereRadius - _densityTransitionRadius)));
    return float2(0.5f * (mu + 1.0f), v);
}
float3 sampleAtmosphereMultipleScattering(uint index, float heightAtReference, float mu)
{
    const float2 texel = atmosphereMultipleScatteringCoordinates(heightAtReference, mu) * float2(ATMOSPHERE_MULTIPLESCATTERING_WIDTH - 1u, ATMOSPHERE_MULTIPLESCATTERING_HEIGHT - 1u);
    const uint2 p0 = uint2(texel);
    const uint2 p1 = min(p0 + 1u, uint2(ATMOSPHERE_MULTIPLESCATTERING_WIDTH - 1u, ATMOSPHERE_MULTIPLESCATTERING_HEIGHT - 1u));
    const float2 f = frac(texel);
    const Texture2D<float4> lut = getTexture(index);
    const float3 blend0 = lerp(lut.Load(int3(p0, 0)).rgb, lut.Load(int3(p1.x, p0.y, 0)).rgb, f.x);
    const float3 blend1 = lerp(lut.Load(int3(p0.x, p1.y, 0)).rgb, lut.Load(int3(p1, 0)).rgb, f.x);
    GPUASSERT(all(blend0 >= 0));
    GPUASSERT(all(blend1 >= 0));
    return lerp(blend0, blend1, f.y);
}
float3 atmosphereMultipleIncident(float height, float radialDotSun)
{
    if (_multipleScatteringStrength <= 0.0f || _multipleScatteringMaxOrder <= 1u) 
        return 0.0f;

    GPUAssert(_densityReferenceRadius + height > 1e-3f);
    const float radius = _densityReferenceRadius + height;
    GPUASSERT(abs(radialDotSun) <= radius, float4(radialDotSun, radius, radialDotSun / radius, 0));
    const float mu = radialDotSun / radius;
    return sampleAtmosphereMultipleScattering(_multipleScatteringLutIndex, height, clamp(mu, -1, 1)) * atmosphereSunIrradiance() * saturate(_multipleScatteringStrength);    // 정밀도 문제로 clamp
}
#endif
