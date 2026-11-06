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

// index에 해당하는 MultipleScattering Texture를 샘플링. 
// 결과는 heightAtReference에서 태양이 특정방향(mu에 내포)에 해당하는 단위 태양 irradiance를 기준으로 계산한 전 방향 평균 입사 radiance
// 단위 태양(sunIntensity가 1)란 뜻으로 이 함수 밖에서 실제 _sunIntensity를 곱해서 사용
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
// 실제 태양 irradiance와 사용자 강도
float3 atmosphereMultipleIncident(float heightAtReference, float radialDotSun)
{
    if (_multipleScatteringStrength <= 0.0f || _multipleScatteringMaxOrder <= 1u) 
        return 0.0f;

    GPUASSERT(_densityReferenceRadius + heightAtReference > 1e-3f);
    const float radius = _densityReferenceRadius + heightAtReference;
    GPUASSERT(abs(radialDotSun) <= radius, float4(radialDotSun, radius, radialDotSun / radius, 0));
    const float mu = clamp(radialDotSun / radius, -1, 1); // 정밀도 문제로 clamp
    return sampleAtmosphereMultipleScattering(_multipleScatteringLutIndex, heightAtReference, mu) * atmosphereSunIrradiance() * _multipleScatteringStrength;
}
#endif
