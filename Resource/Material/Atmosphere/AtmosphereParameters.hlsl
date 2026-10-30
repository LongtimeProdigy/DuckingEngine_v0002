#ifndef ATMOSPHERE_PARAMETERS_HLSL
#define ATMOSPHERE_PARAMETERS_HLSL

#include "Assert.hlsl"

#define ATEMOSPHERE_PRODUCTION 0
#define ATEMOSPHERE_REFERENCE 1
#define ATEMOSPHERE_LIMIT_DEBUG 2
#define ATMOSPHERE_WORK_DEBUG 3
#define ATMOSPHERE_TRANSMITTANCE_DEBUG 4

struct AtmosphereAbsorptionLayer
{
    float3 extinction; // RGB coefficient at unit layer density, in inverse metres.
    float strength;
    float peakRadius; // Absolute distance from the planet centre; independent of terrain.
    float lowerWidth;
    float upperWidth;
    float reserved;
};

struct SurfaceShadowCascade
{
    float4 right, up, forward, origin;
    uint albedoIndex, depthIndex, mapSize, padding;
};

cbuffer AtmosphereConstantBuffer : register(b1)
{
    uint _numInScatteringPoints;
    uint _opticalDepthPointCount;
    float _rayleighHeight;
    float _mieHeight;

    float _sunDegree;
    float _sunIrradiance;
    float _scatteringScale;
    float _cameraAltitude;

    float _densityReferenceRadius;
    float3 _planetCentre;

    float _atmosphereRadius;
    float _sunAngularRadius;
    float _exposureEV;
    float _bloomStrength;

    float _bloomRadius;
    float _bloomThreshold;
    float _sunRadianceScale;
    float _densityTransitionRadius; // Inner approximation boundary, in metres from centre.

    float _integrationRelativeTolerance;
    uint _integrationMaxDepth;
    uint _atmosphereSamplingMode; // 0: production, 1: direct reference, 2: reference limit debug
    uint _transmittanceLutIndex;

    float _rayleighScale;
    float _mieScale;
    float _mieAbsorptionScale;
    float _mieAnisotropy;

    float3 _sunColor; // Linear RGB chromaticity; normalized to unit luminance in the helper.
    float _skyLightingStrength;

    uint _skyRadianceIndex;
    uint _skyFilteredIndex;
    uint _viewNodeBudget;
    uint _opticalNodeBudget;

    AtmosphereAbsorptionLayer _absorptionLayers[2];
    float4 _surfaceRight; // xyz: world basis, w: half-width in metres.
    float4 _surfaceUp;
    float4 _surfaceForward; // w: total capture depth.
    float4 _surfaceOrigin; // Light-view origin relative to the main camera.
    uint _surfaceAlbedoIndex;
    uint _surfaceNormalIndex;
    uint _surfaceMaterialIndex;
    uint _surfaceDepthIndex;
    uint _surfaceBounceIndex;
    uint _surfaceMapEnabled;
    uint _surfaceMapSize;
    float _surfaceShadowStrength;
    float _surfaceBounceStrength;
    float _surfaceBias;
    uint _surfaceBounceSamples;
    float _surfacePadding;
    uint _multipleScatteringLutIndex;
    float _multipleScatteringStrength;
    uint _multipleScatteringMaxOrder;
    float _multipleScatteringPadding;
    SurfaceShadowCascade _shadowCascades[4];
    uint _shadowCascadeCount;
    float _shadowCascadeBlend;
    float2 _shadowCascadePadding;
};

bool atmosphereUsesLut(bool allowReference = true)
{
    return allowReference == false || _atmosphereSamplingMode == ATEMOSPHERE_PRODUCTION || _atmosphereSamplingMode >= 3u;
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
