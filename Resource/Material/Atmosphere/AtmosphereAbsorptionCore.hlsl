#ifndef ATMOSPHERE_ABSORPTION_CORE_HLSL
#define ATMOSPHERE_ABSORPTION_CORE_HLSL

// sphere에 겹치는 t1, t2를 반환. 사실상 raySphere와 같지만 대기연산에 (정밀도 등) 최적화가 되어있음
float2 atmosphereAbsorptionSphereHits(float heightAtReference, float radialDot, float radius)
{
    if (radius <= 0.0f) 
        return float2(1e30f, -1e30f);

    GPUASSERT(_densityReferenceRadius >= 0);

    const float height = _densityReferenceRadius + heightAtReference;
    GPUASSERT(height - abs(radialDot) >= 0);
    const float triangleHeight = sqrt((height - abs(radialDot)) * (height + abs(radialDot)));    // 파타고라스 정리
    if (triangleHeight > radius)    // transitionRadius와 접하지 않음
        return float2(1e30f, -1e30f);

    GPUASSERT(radius - triangleHeight >= 0);
    const float root = sqrt((radius - triangleHeight) * (radius + triangleHeight));
    const float h = heightAtReference - (radius - _densityReferenceRadius); // height - radius지만 정밀도 문제로 이렇게 계산
    const float c = h * (2.0f * radius + h);
    const float t1 = -radialDot - (radialDot >= 0.0f ? root : -root);
    const float t2 = abs(t1) > 1e-10f ? c / t1 : -radialDot;
    return float2(min(t1, t2), max(t1, t2));
}
#endif
