#ifndef ATMOSPHERE_GEOMETRY_HLSL
#define ATMOSPHERE_GEOMETRY_HLSL
#include "CommonMathConstant.hlsl"
#include "AtmosphereParameters.hlsl"

// All positions in this shader are relative to the camera. The CPU supplies
// camera altitude in double precision; this form preserves small surface heights.
float atmosphereHeight(float3 cameraRelativePosition)
{
    // 원래는 아래식을 구하는 게 전부이지만, float 정밀도를 개선하기 식을 변형함
    // length(cameraRadial + cameraRelativePosition) - _densityReferenceRadius
    const float3 cameraRadial = -_planetCentre;
    const float numerator = dot(cameraRelativePosition, cameraRelativePosition) + 2.0f * dot(cameraRelativePosition, cameraRadial) + _cameraAltitude * (2.0f * _densityReferenceRadius + _cameraAltitude);
    return numerator / max(length(cameraRadial + cameraRelativePosition) + _densityReferenceRadius, 1.0f);
}

float2 raySphere(const float3 sphereCentre, const float sphereRadius, const float3 rayOrigin, const float3 rayDir)
{
    const float3 radial = rayOrigin - sphereCentre;
    const float altitude = atmosphereHeight(rayOrigin) + (_densityReferenceRadius - sphereRadius);
    const float c = altitude * (2.0f * sphereRadius + altitude);
    const float b = dot(radial, rayDir);
    const float determinant = b * b - c;
    if (determinant < 0.0f)
        return float2(FLOAT_MAX, 0);
    const float root = sqrt(determinant);
    // Stable quadratic roots avoid subtracting two nearly equal values at the ground.
    const float q = -b - (b >= 0.0f ? root : -root);
    const float t0 = q;
    const float t1 = abs(q) > 1e-10f ? c / q : -b;
    const float nearHit = min(t0, t1);
    const float farHit = max(t0, t1);
    if (farHit <= 0.0f)
        return float2(FLOAT_MAX, 0);
    const float entry = max(nearHit, 0.0f);
    return float2(entry, max(farHit - entry, 0.0f));
}

#endif
