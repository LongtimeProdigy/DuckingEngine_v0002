#ifndef ATMOSPHERE_GEOMETRY_HLSL
#define ATMOSPHERE_GEOMETRY_HLSL
#include "CommonMathConstant.hlsl"
#include "Atmosphere/AtmosphereParameters.hlsl"

float atmosphereHeight(float3 cameraRelativePosition)
{
    // 이 함수는 아래식을 구하는 게 전부이지만, float 정밀도를 개선하기 식을 변형함
    // length(cameraRelativePosition - _planetCentre) - _densityReferenceRadius

    const float3 cameraRadial = -_planetCentre;
    const float3 radial = cameraRadial + cameraRelativePosition;
    const float nearCameraRange = 0.1f * _densityReferenceRadius;
    if (dot(cameraRelativePosition, cameraRelativePosition) > nearCameraRange * nearCameraRange)    // 카메라로부터 멀리(표준 반지름의 10%)진 경우 정밀도가 떨어져도 비슷하니 그냥 계산
        return length(radial) - _densityReferenceRadius;

    // 카메라 근처인 경우 정밀도가 중요해지니 정밀도를 고려한 계산한다.
    // _cameraAltitude가 매우큰 경우 cameraRelativePosition에서 이미 부동소수점 오차가 엄청나게 크게 나온다. 이에 따라 아래 계산식이 꽤 큰 오차율을 뱉을 수 있으므로 clamp를 한다.
    const float numerator = dot(cameraRelativePosition, cameraRelativePosition) + 2.0f * dot(cameraRelativePosition, cameraRadial) + _cameraAltitude * (2.0f * _densityReferenceRadius + _cameraAltitude);
    GPUASSERT(length(radial) + _densityReferenceRadius >= 1);
    return clamp(numerator / length(radial) + _densityReferenceRadius, 0, _atmosphereRadius - _densityReferenceRadius);
}

float2 raySphere(const float3 sphereCentre, const float sphereRadius, const float3 rayOrigin, const float3 rayDir)
{
    const float3 radial = rayOrigin - sphereCentre;
    const float altitude = atmosphereHeight(rayOrigin) + (_densityReferenceRadius - sphereRadius);
    const float c = altitude * (2.0f * sphereRadius + altitude);
    const float b = dot(radial, rayDir);
    // A tangent ray makes b*b and c almost equal for a distant camera.
    // Cross product gives the distance from the centre to the ray directly.
    const float impact = length(cross(radial, rayDir));
    const float determinant = (sphereRadius - impact) * (sphereRadius + impact);
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
