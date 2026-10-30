#ifndef ATMOSPHERE_TRANSMITTANCE_HLSL
#define ATMOSPHERE_TRANSMITTANCE_HLSL
#include "CommonTexture.hlsl"
#include "Atmosphere/AtmosphereIntegration.hlsl"
#include "Atmosphere/AtmosphereGeometry.hlsl"

static const uint ATMOSPHERE_LUT_WIDTH = 512u;
static const uint ATMOSPHERE_LUT_HEIGHT = 128u;

// heightAtReference부터 rayDir방향으로 쏠 때 atmosphereRadius구를 벗어나는 path length룰 반환함
float atmosphereTopDistance(float heightAtReference, float radialDot)
{
    GPUASSERT(heightAtReference <= (_atmosphereRadius - _densityReferenceRadius));

    const float heightAtAtmosphere = heightAtReference - (_atmosphereRadius - _densityReferenceRadius); // (heightAtReference + _densityReferenceRadius) - _atmosphereRadius와 동치. 정밀도 때문에 -먼저 수행

    // 구하고자하는 건  방사방향² - _atmosphereRadius²이지만 float 정밀도 개선을 위해 다음과 같은 식을 사용
    const float c = heightAtAtmosphere * (2.0f * _atmosphereRadius + heightAtAtmosphere);
    // abs(radialDot)은 기하적으로 보면 heightAtReference(점 p)에서 dir방향의 선분에 행성중심에서 수선을 발을 내려 만든 직각삼각형의 밑변 길이가 된다
    // 이때 root의 기하적 의미는 수선의 발을 내린 점에서 선분이 구에 접하는 두 점부터 수선의 발 점까지 거리이다.
    GPUASSERT(radialDot * radialDot - c >= 0);
    const float root = sqrt(radialDot * radialDot - c);
    // 근의 공식에 따르면 t1, t2는 각각 -radialDot ± √root 이지만 정밀도 문제로 q를 사용함
    const float t1 = -radialDot - (radialDot >= 0.0f ? root : -root);   // 같은 부호끼리 더해 근을 더해 정밀도를 최소화 하기 위해 root선택
    const float t2 = abs(t1) > 1e-10f ? c / t1 : -radialDot;            // t1값이 0에 가깝다는 건 height가 atmosphereRadius근처이므로 -radialDot이 길이가 된다.
    const float t = max(t1, t2);    // rayDir방향으로 나가는 출구인 점을 찾기 때문에 무조건 양수를 찾아야합니다.
    GPUASSERT(t >= 0);
    return t;
}

// heightAtReference에서 방사방향과 transtionRadius구의 접선이 이루는 각도 theta의 cos값을 반환
float atmosphereHorizonCosine(float heightAtReference)
{
    const float heightAtTransition = heightAtReference - atmosphereTransitionHeight();
    const float heightAtCentre = _densityTransitionRadius + heightAtTransition;

    GPUASSERT(heightAtCentre > 1e-10f); // height는 항상 transitionRadius보다 위에 있음을 보장
    GPUASSERT(heightAtTransition >= 0);

    // 아래는 cos(theta) = root(heightAtCentre² - transitionRadius²) / heightAtCentre와 동치
    // 단, float 정밀도 때문에 heightAtCentre² - transitionRadius²를 heightAtTransition * (2 * transitionRadius + heightAtTransition)으로 계산한다
    // height가 항상 transitionRadius보다 위에 있기 때문에 맨 앞에 -를 곱해주어야함
    return -sqrt(heightAtTransition * (2.0f * _densityTransitionRadius + heightAtTransition)) / heightAtCentre;
}

// atmosphereLutParameters의 역함수
float2 atmosphereLutCoordinates(float heightAtReference, float mu)
{
    const float horizon = atmosphereHorizonCosine(heightAtReference);
    GPUASSERT(abs(horizon) < 1);    // horizon이 -1이면 0으로 나눗셈이 되므로 0도 예외해야함
    const float s = mu >= horizon ? sqrt(saturate((mu - horizon) / (1.0f - horizon))) : -sqrt(saturate((horizon - mu) / (1.0f + horizon)));
    GPUASSERT(_atmosphereRadius - _densityTransitionRadius > 0);    // 0이면 zero-divice이므로 예외해야함
    return float2(0.5f * (s + 1.0f), sqrt(saturate((heightAtReference - atmosphereTransitionHeight()) / (_atmosphereRadius - _densityTransitionRadius))));
}

// heightAtReference에서 태양 방향으로 _atmosphereRadius까지의 ‘성분별 밀도 적분값’을 반환
AtmosphereOpticalDepth atmosphereSunOpticalDepthOutsideTransitionRadius(float heightAtReference, float radialDot, float pathLength, bool allowReference = true)
{
    if (pathLength <= 0.0f)
    {
        AtmosphereOpticalDepth empty;
        empty.value = 0.0f;
        empty.limitReached = 0u;
        return empty;
    }

#if defined(ATMOSPHERE_FORCE_LUT)
    GPUASSERT(heightAtReference >= atmosphereTransitionHeight() && heightAtReference <= _atmosphereRadius - _densityReferenceRadius);
    heightAtReference = clamp(heightAtReference, atmosphereTransitionHeight(), _atmosphereRadius - _densityReferenceRadius);
#else
    if (atmosphereUsesLut(allowReference) == false || heightAtReference < atmosphereTransitionHeight() || heightAtReference > _atmosphereRadius - _densityReferenceRadius)
    {
        GPUASSERT(heightAtReference <= (_atmosphereRadius - _densityReferenceRadius), float4(heightAtReference, atmosphereTransitionHeight(), _atmosphereRadius - _densityReferenceRadius, allowReference ? 1 : 0));
        return integrateAtmosphereOpticalDepth(heightAtReference, radialDot, pathLength);
    }
#endif

    const float radius = _densityReferenceRadius + heightAtReference;
    GPUASSERT(isfinite(radius) && radius >= 1e-10f, float4(radius, heightAtReference, _densityReferenceRadius, radialDot));
    GPUASSERT(abs(radialDot) <= radius, float4(radialDot, _densityReferenceRadius, heightAtReference, radialDot / radius));
    const float mu = radialDot / radius; // radialDot은 (_densityReferenceRadius + heightAtReference) * cos이므로, 정밀도 문제로 clamp처리
    const float2 texel = atmosphereLutCoordinates(heightAtReference, clamp(mu, -1, 1)) * float2(ATMOSPHERE_LUT_WIDTH - 1u, ATMOSPHERE_LUT_HEIGHT - 1u);
    const uint2 p0 = uint2(texel);
    const uint2 p1 = min(p0 + 1u, uint2(ATMOSPHERE_LUT_WIDTH - 1u, ATMOSPHERE_LUT_HEIGHT - 1u));
    const float2 blend = frac(texel);

    const Texture2D<float4> lut = getTexture(_transmittanceLutIndex);
    // 직접 clamp sampler를 수행. 추후에 clamp sampler롤 바인딩하고 sampling하는 것 고려
    const float4 a = lut.Load(int3(p0, 0));
    const float4 b = lut.Load(int3(p1.x, p0.y, 0));
    const float4 c = lut.Load(int3(p0.x, p1.y, 0));
    const float4 d = lut.Load(int3(p1, 0));

    // Keep production lookup bounded even when the LUT generation exhausted its budget.
    AtmosphereOpticalDepth result;
    result.value = lerp(lerp(a, b, blend.x), lerp(c, d, blend.x), blend.y);
    GPUASSERT(all(result.value >= 0));

    const int2 flags = int2(0, ATMOSPHERE_LUT_HEIGHT);
    const float f0 = lut.Load(int3(int2(p0) + flags, 0)).x;
    const float f1 = lut.Load(int3(int2(p1.x, p0.y) + flags, 0)).x;
    const float f2 = lut.Load(int3(int2(p0.x, p1.y) + flags, 0)).x;
    const float f3 = lut.Load(int3(int2(p1) + flags, 0)).x;
    result.limitReached = max(max(f0, f1), max(f2, f3)) > 0.0f ? 1u : 0u;
    return result;
}

float atmosphereAbsorptionGauss(float height, float radialDot, float a, float b, uint layer)
{
    if (b <= a) return 0.0f;
    const float nodes[4] = { -0.8611363116f, -0.3399810436f, 0.3399810436f, 0.8611363116f };
    const float weights[4] = { 0.3478548451f, 0.6521451549f, 0.6521451549f, 0.3478548451f };
    const float middle = 0.5f * (a + b), halfWidth = 0.5f * (b - a);
    float sum = 0.0f;
    [unroll] for (uint i = 0u; i < 4u; ++i)
        sum += weights[i] * atmosphereAbsorptionDensity(atmosphereHeightOnRay(height, radialDot, middle + halfWidth * nodes[i]), layer);
    return max(sum * halfWidth, 0.0f);
}
float atmosphereAbsorptionShell(float height, float radialDot, float length, float innerRadius, float outerRadius, uint layer)
{
    if (outerRadius <= innerRadius || outerRadius <= 0.0f) return 0.0f;
    const float2 outer = atmosphereAbsorptionSphereHits(height, radialDot, outerRadius);
    const float a = max(outer.x, 0.0f), b = min(outer.y, length);
    if (b <= a) return 0.0f;
    const float2 inner = atmosphereAbsorptionSphereHits(height, radialDot, innerRadius);
    const float leftEnd = min(b, max(a, inner.x));
    const float rightStart = max(a, min(b, inner.y));
    if (inner.y > a && inner.x < b)
        return atmosphereAbsorptionGauss(height, radialDot, a, leftEnd, layer)
             + atmosphereAbsorptionGauss(height, radialDot, rightStart, b, layer);
    // Split at closest approach so a central ray has no unresolved absolute-value corner.
    const float middle = clamp(-radialDot, a, b);
    return atmosphereAbsorptionGauss(height, radialDot, a, middle, layer)
         + atmosphereAbsorptionGauss(height, radialDot, middle, b, layer);
}

// Exact integral of the interior profile, matched to the density at the boundary.
// Core 내부 밀도 모델의 해석적 적분
float2 atmosphereInteriorOpticalDepth(float heightAtTransition, float radialDot, float pathLength)
{
    GPUASSERT(_densityTransitionRadius > 1e-6f);
    const float c = heightAtTransition * (2.0f * _densityTransitionRadius + heightAtTransition);
    const float meanCompression = max(-(c + radialDot * pathLength + pathLength * pathLength / 3.0f) / (2.0f * _densityTransitionRadius), 0.0f);
    return atmosphereExponentialDensity(atmosphereTransitionHeight()) * pathLength * (1.0f + meanCompression / max(float2(_rayleighHeight, _mieHeight), 1e-6f));
}
// 흡수층 구간을 나누고 Gauss 수치 적분
float2 atmosphereInteriorAbsorptionDepth(float heightAtReference, float radialDot, float length)
{
    float2 result = 0.0f;
    [unroll] 
    for (uint layer = 0u; layer < 2u; ++layer)
    {
        if (any(atmosphereAbsorptionExtinction(layer) > 0.0f) == false) 
            continue;

        const float peak = max(_absorptionLayers[layer].peakRadius, 0.0f);
        result[layer] = atmosphereAbsorptionShell(heightAtReference, radialDot, length, max(peak - _absorptionLayers[layer].lowerWidth, 0.0f), peak, layer) 
            + atmosphereAbsorptionShell(heightAtReference, radialDot, length, peak, peak + max(_absorptionLayers[layer].upperWidth, 0.0f), layer);
    }
    return result;
}
// heightAtReference에서 태양 방향으로 나아가는 경로의 성분별 밀도 적분값을 반환
AtmosphereOpticalDepth atmosphereSunOpticalDepth(float heightAtReference, float radialDot, float pathLength, bool allowReference = true)
{
    // TransitionRadius 밖일 때
    if (heightAtReference >= atmosphereTransitionHeight() || pathLength <= 0.0f)
        return atmosphereSunOpticalDepthOutsideTransitionRadius(heightAtReference, radialDot, pathLength, allowReference);

    // TransitionRadius 안일 때
    const float heightAtTransition = heightAtReference - atmosphereTransitionHeight();
    const float c = heightAtTransition * (2.0f * _densityTransitionRadius + heightAtTransition);
    GPUASSERT(radialDot * radialDot - c >= 0);
    const float root = clamp(sqrt(radialDot * radialDot - c), -(_densityTransitionRadius + heightAtTransition), _densityTransitionRadius + heightAtTransition); // root == sunRadialDot과 같아짐. radialDot은 무조건 height보다 작아야함
    float exitDistance;
    if (radialDot >= 0.0f)
    {
        const float denominator = root + radialDot;
        GPUASSERT(isfinite(denominator) && denominator > 0.0f, float4(root, radialDot, c, heightAtTransition));
        exitDistance = -c / denominator;
    }
    else
    {
        exitDistance = root - radialDot;
    }
    GPUASSERT(isfinite(exitDistance) && exitDistance >= 0.0f, float4(exitDistance, root, radialDot, c));
    const float coreLength = min(exitDistance, pathLength); // pathLength가 Core안에서 끝나는 경우 pathLength를 보내줘야하니 min연산
    AtmosphereOpticalDepth result;
    result.value = float4(atmosphereInteriorOpticalDepth(heightAtTransition, radialDot, coreLength), atmosphereInteriorAbsorptionDepth(heightAtReference, radialDot, coreLength));
    result.limitReached = 0u;

    if (coreLength < pathLength)    // Core 바깥으로 이어지는 구간이 있으면 더함
    {
        GPUASSERT(root >= 0);
        GPUASSERT(root <= _densityTransitionRadius, float4(root, _densityTransitionRadius, allowReference ? 1 : 0, 0));
        const AtmosphereOpticalDepth outer = atmosphereSunOpticalDepthOutsideTransitionRadius(atmosphereTransitionHeight(), root, pathLength - coreLength, allowReference);
        result.value += outer.value;
        result.limitReached = outer.limitReached;
    }
    return result;
}

// cameraRelativePosition에서 sunDir방향으로의 투과율을 구함 (LUT 사용)
float3 getSunTransmittance(float3 cameraRelativePosition, float3 sunDir)
{
    if (_scatteringScale <= 0.0f)
        return float3(1, 1, 1);

    const float2 hit = raySphere(_planetCentre, _atmosphereRadius, cameraRelativePosition, sunDir);
    if (hit.y <= 0.0f)
        return float3(1, 1, 1);

    const float3 origin = cameraRelativePosition + sunDir * hit.x;
    // hit.x > 0은 대기밖에 cameraRelativePosition이 있으므로 heightAtReference가 대기의 높이임
    const float heightAtReference = hit.x > 0.0f ? _atmosphereRadius - _densityReferenceRadius : atmosphereHeight(origin);
    GPUASSERT(heightAtReference <= (_atmosphereRadius - _densityReferenceRadius), float4(heightAtReference, atmosphereTransitionHeight(), _atmosphereRadius - _densityReferenceRadius, 0));
    GPUASSERT(abs(dot(origin - _planetCentre, sunDir)) <= _densityReferenceRadius + heightAtReference, float4(length(origin), length(_planetCentre), length(origin - _planetCentre), dot(origin - _planetCentre, sunDir)));
    const AtmosphereOpticalDepth optical = atmosphereSunOpticalDepth(heightAtReference, dot(origin - _planetCentre, sunDir), hit.y);
    return exp(-atmosphereOpticalExtinction(optical.value));
}
#endif
