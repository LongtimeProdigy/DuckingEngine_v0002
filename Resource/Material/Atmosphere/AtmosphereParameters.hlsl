#ifndef ATMOSPHERE_PARAMETERS_HLSL
#define ATMOSPHERE_PARAMETERS_HLSL

#include "Assert.hlsl"
#include "Atmosphere/AtmosphereLimits.h"

#define DK_SHADER_ATMOSPHERE
#include "ShaderStructures.h"
#undef DK_SHADER_ATMOSPHERE

bool atmosphereUsesLut(bool allowReference = true)
{
    return allowReference == false || (_atmosphereSamplingMode != ATMOSPHERE_REFERENCE && _atmosphereSamplingMode != ATMOSPHERE_LIMIT_DEBUG);
}

// 태양 방향에 수직인 단위 면적이 받는 태양 원반 전체의 빛
float3 atmosphereSunIrradiance()
{
    GPUASSERT(all(_sunColor >= 0));
    GPUASSERT(_sunIrradiance >= 0);
    const float3 color = max(_sunColor, 0.0f);
    // 선형 sRGB/Rec.709 기준의 휘도 가중치로 나누면 RGB 비율은 유지하면서, 가중 밝기가 1인 색이 됩니다.
    // 여기에 _sunIrradiance를 곱하므로 최종 결과의 가중 밝기는 _sunIrradiance가 됩니다
    return max(_sunIrradiance, 0.0f) * color / max(dot(color, float3(0.2126f, 0.7152f, 0.0722f)), 1e-6f);
}
#endif
