#ifndef ATMOSPHERE_TRANSMITTANCE_HLSL
#define ATMOSPHERE_TRANSMITTANCE_HLSL
#include "CommonTexture.hlsl"
#include "Atmosphere/AtmosphereIntegration.hlsl"
#include "Atmosphere/AtmosphereGeometry.hlsl"

// Match SceneRenderer's allocation. Float columns avoid losing small transmittances.
static const uint ATMOSPHERE_LUT_WIDTH = 512u;
static const uint ATMOSPHERE_LUT_HEIGHT = 128u;

// heightAtReference부터 rayDir방향으로 쏠 때 atmosphereRadius구를 벗어나는 path length룰 반환함
// heightAtReference위치 점 p + t * rayDir라고 가정하면, (p + t*rayDir)² == atmosphereRadius²를 만족하는 t를 구하면 된다
// 이는 t를 매개로 하는 2차방정식 풀이이다. (t² + 2*p*t + p² - amotsphereRadius² == 0)
float atmosphereTopDistance(float heightAtReference, float radialDot)
{
    const float altitude = heightAtReference - (_atmosphereRadius - _densityReferenceRadius);   // atmosphereRadius가 고도 0이라고 했을때, heightAtRenference의 상대 높이
    const float c = altitude * (2.0f * _atmosphereRadius + altitude);   // 구하고자하는 건  (행성 중심부터 p까지)² - _atmosphereRadius²이지만 float 정밀도 개선을 위해 다음과 같은 식을 사용

    // -radialDot은 기하적으로 보면 heightAtReference(점 p)에서 dir방향의 선분에 행성중심에서 수선을 발을 내려 만든 직각삼각형의 밑변 길이가 된다
    // 이때 root의 기하적 의미는 수선의 발을 내린 점에서 선분이 구에 접하는 두 점 사이의 거리이다. (두 점과 수선의 발 사이의 거리는 같다) (피타고라스를 이용하면 나옴)
    // 따라서 t0 = -radialdot - root, t1 = -radialdot + root가된다
    const float root = sqrt(max(radialDot * radialDot - c, 0.0f));
    const float q = -radialDot - (radialDot >= 0.0f ? root : -root);    // radialDot이 양수라는 것은 방사방향과 rayDir이 90도 이하라는 것. 따라서 root를 더해야 대기표면 점이 나옴
    return max(max(q, abs(q) > 1e-10f ? c / q : -radialDot), 0.0f);     // 정밀도를 올리기 위해 q를 바로 return하지 않음. t1*p = c를 이용해서 t2를 구함, q가  0에 근사하면 접선이므로 -radialDot 사용
}

// 방사방향(heightAtReference - pranetCentre)와 heightAtReference에서 transtionRadius구의 접선이 이루는 각도 theta의 cos값을 반환
float atmosphereHorizonCosine(float heightAtReference)
{
    const float heightAtTransition = heightAtReference - atmosphereTransitionHeight();
    return -sqrt(max(heightAtTransition * (2.0f * _densityTransitionRadius + heightAtTransition), 0.0f)) / max(_densityReferenceRadius + heightAtReference, 1e-10f);
}

// uv를 실제 값으로 변경.
float2 atmosphereLutParameters(float2 uv)
{
    // uv.y의 0은 transitionradius 높이를 가리키기 때문에 referenceheight로 변환하기 위해서는 atmosphereTransitionHeight()를 더해야함
    const float heightAtReference = atmosphereTransitionHeight() + uv.y * uv.y * (_atmosphereRadius - _densityTransitionRadius); //uv.y²로 높이를 배치해 전환 경계 근처에 샘플이 집중됨

    const float horizon = atmosphereHorizonCosine(heightAtReference);   // heightAtReference에서 transitionRadius구에 접선하는 선분과 방사방향과의 mu
    const float s = 2.0f * uv.x - 1.0f;
    // 아래는 mu = dot(normalize(p - planetCentre), sunDir); 를 변형한 것 - 여기서 sunDir을 uv.x에 따라 0(방사방향)~180(pranetCentre방향)도로 확장한거임
    const float mu = s >= 0.0f ? lerp(horizon, 1.0f, s * s) : lerp(horizon, -1.0f, s * s);   // s²를 이용해서 접선부근(uv.x == 0.5)에서 샘플이 집중됨, s==0은 구에 접함

    return float2(heightAtReference, mu);
}

// atmosphereLutParameters의 역함수
float2 atmosphereLutCoordinates(float height, float mu)
{
    const float horizon = atmosphereHorizonCosine(height);
    const float s = mu >= horizon ? sqrt(saturate((mu - horizon) / max(1.0f - horizon, 1e-10f))) : -sqrt(saturate((horizon - mu) / max(1.0f + horizon, 1e-10f)));
    return float2(0.5f * (s + 1.0f), sqrt(saturate((height - atmosphereTransitionHeight()) / max(_atmosphereRadius - _densityTransitionRadius, 1e-10f))));
}

AtmosphereOpticalDepth atmosphereSunOpticalDepthOutsideCore(float height, float radialDot, float pathLength, bool allowReference = true)
{
    if (pathLength <= 0.0f)
    {
        AtmosphereOpticalDepth empty;
        empty.value = 0.0f;
        empty.limitReached = 0u;
        return empty;
    }

    // The LUT covers the exponential region, including negative reference altitudes.
#if defined(ATMOSPHERE_FORCE_LUT)
    // Probe generation never falls back to nested adaptive integration.
    height = clamp(height, atmosphereTransitionHeight(), _atmosphereRadius - _densityReferenceRadius);
#else
    if (!atmosphereUsesLut(allowReference) || height < atmosphereTransitionHeight() || height > _atmosphereRadius - _densityReferenceRadius)
        return integrateAtmosphereOpticalDepth(height, radialDot, pathLength);
#endif

    const float mu = clamp(radialDot / max(_densityReferenceRadius + height, 1e-10f), -1.0f, 1.0f);
    const float2 grid = atmosphereLutCoordinates(height, mu) * float2(ATMOSPHERE_LUT_WIDTH - 1u, ATMOSPHERE_LUT_HEIGHT - 1u);
    const uint2 p0 = uint2(grid);
    const uint2 p1 = min(p0 + 1u, uint2(ATMOSPHERE_LUT_WIDTH - 1u, ATMOSPHERE_LUT_HEIGHT - 1u));
    const float2 blend = frac(grid);
    const Texture2D<float4> lut = getTexture(_transmittanceLutIndex);

    // Explicit clamped bilinear loads avoid the engine's repeat-address sampler.
    const float4 a = lut.Load(int3(p0, 0));
    const float4 b = lut.Load(int3(p1.x, p0.y, 0));
    const float4 c = lut.Load(int3(p0.x, p1.y, 0));
    const float4 d = lut.Load(int3(p1, 0));
    // Keep production lookup bounded even when the LUT generation exhausted its budget.
    AtmosphereOpticalDepth result;
    result.value = max(lerp(lerp(a, b, blend.x), lerp(c, d, blend.x), blend.y), 0.0f);
    const int2 flags = int2(0, ATMOSPHERE_LUT_HEIGHT);
    const float f0 = lut.Load(int3(int2(p0) + flags, 0)).x;
    const float f1 = lut.Load(int3(int2(p1.x, p0.y) + flags, 0)).x;
    const float f2 = lut.Load(int3(int2(p0.x, p1.y) + flags, 0)).x;
    const float f3 = lut.Load(int3(int2(p1) + flags, 0)).x;
    result.limitReached = max(max(f0, f1), max(f2, f3)) > 0.0f ? 1u : 0u;
    return result;
}
// Integrate the quadratic interior density analytically, then reuse the
// outward surface LUT. This avoids nested per-pixel integration in production.
AtmosphereOpticalDepth atmosphereSunOpticalDepth(float height, float radialDot, float pathLength, bool allowReference = true)
{
    if (height >= atmosphereTransitionHeight() || pathLength <= 0.0f)
        return atmosphereSunOpticalDepthOutsideCore(height, radialDot, pathLength, allowReference);

    const float localHeight = height - atmosphereTransitionHeight();
    const float c = localHeight * (2.0f * _densityTransitionRadius + localHeight);
    const float root = sqrt(max(radialDot * radialDot - c, 0.0f));
    const float exitDistance = radialDot >= 0.0f ? -c / max(root + radialDot, 1e-10f) : root - radialDot;
    const float coreLength = min(max(exitDistance, 0.0f), pathLength);
    AtmosphereOpticalDepth result;
    result.value = float4(atmosphereInteriorOpticalDepth(height, radialDot, coreLength),
        atmosphereInteriorAbsorptionDepth(height, radialDot, coreLength));
    result.limitReached = 0u;
    if (coreLength < pathLength)
    {
        const AtmosphereOpticalDepth outer = atmosphereSunOpticalDepthOutsideCore(atmosphereTransitionHeight(), max(radialDot + coreLength, 0.0f), pathLength - coreLength, allowReference);
        result.value += outer.value;
        result.limitReached = outer.limitReached;
    }
    return result;
}
// Gas attenuation only. sunDir must be a unit vector toward the sun.
// Surface orientation, albedo and opaque occlusion belong to the caller.
float3 getSunTransmittance(float3 cameraRelativePosition, float3 sunDir)
{
    if (_scatteringScale <= 0.0f)
        return float3(1, 1, 1);

    const float2 hit = raySphere(_planetCentre, _atmosphereRadius, cameraRelativePosition, sunDir);
    if (hit.y <= 0.0f)
        return float3(1, 1, 1);

    // Outside surfaces start integration at the atmosphere entry, not in vacuum.
    const float3 origin = cameraRelativePosition + sunDir * hit.x;
    const float height = hit.x > 0.0f ? _atmosphereRadius - _densityReferenceRadius : atmosphereHeight(origin);
    const AtmosphereOpticalDepth optical = atmosphereSunOpticalDepth(height, dot(origin - _planetCentre, sunDir), hit.y);
    const float3 betaR = atmosphereRayleighScattering();
    const float3 betaM = atmosphereMieExtinction();
    return exp(-atmosphereOpticalExtinction(optical.value));
}
#endif
