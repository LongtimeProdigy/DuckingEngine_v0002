#ifndef ATMOSPHERE_INTEGRATION_HLSL
#define ATMOSPHERE_INTEGRATION_HLSL
#include "Atmosphere/AtmosphereParameters.hlsl"

static const float3 gStandardBetaR = float3(3.8e-6f, 13.5e-6f, 33.1e-6f);
static const float3 gStandardBetaM = float3(21e-6f, 21e-6f, 21e-6f);

// Rayleigh의 산란계수 또는 소멸계수 (레일리는 흡수계수가 0이므로 사실상 소멸계수 == 산란계수)
float3 atmosphereRayleighScattering() { return gStandardBetaR * max(_scatteringScale, 0.0f) * max(_rayleighScale, 0.0f); }
// Mie의 산란계수
float3 atmosphereMieScattering() { return gStandardBetaM * max(_scatteringScale, 0.0f) * max(_mieScale, 0.0f); }
// Mie의 소멸계수 (산란계수 + 소멸계수)
float3 atmosphereMieExtinction()
{
    return gStandardBetaM * max(_scatteringScale, 0.0f) * (max(_mieScale, 0.0f) + max(_mieAbsorptionScale, 0.0f));
}
// 흡수 레이어의 소멸계수, 순수 흡수 성분 (ex. 오존층)
float3 atmosphereAbsorptionExtinction(uint layer)
{
    return max(_absorptionLayers[layer].extinction, 0.0f) * max(_absorptionLayers[layer].strength, 0.0f) * max(_scatteringScale, 0.0f);
}
float atmosphereAbsorptionDensity(float height, uint layer)
{
    const float peakHeight = _absorptionLayers[layer].peakRadius - _densityReferenceRadius;
    const float delta = height - peakHeight;
    const float width = delta < 0.0f ? _absorptionLayers[layer].lowerWidth : _absorptionLayers[layer].upperWidth;
    return saturate(1.0f - abs(delta) / max(width, 1e-6f));
}
float3 atmosphereOpticalExtinction(float4 columns)
{
    return atmosphereRayleighScattering() * columns.x + atmosphereMieExtinction() * columns.y
        + atmosphereAbsorptionExtinction(0u) * columns.z + atmosphereAbsorptionExtinction(1u) * columns.w;
}

float atmosphereRayleighPhase(float mu) { return 3.0f / (16.0f * 3.14159265359f) * (1.0f + mu * mu); }
float atmosphereMiePhase(float mu)
{
    const float g = clamp(_mieAnisotropy, -0.95f, 0.95f);
    return (1.0f - g * g) / (4.0f * 3.14159265359f * pow(max(1.0f + g * g - 2.0f * g * mu, 1e-4f), 1.5f));
}

// Mean transmittance through a constant-extinction interval: integral_0^1 exp(-tau*u) du.
// The series avoids cancellation at small optical depths, including vacuum.
float3 atmosphereIntervalWeight(float3 tau)
{
    const float3 regular = (1.0f - exp(-tau)) / max(tau, 1e-30f);
    const float3 series = 1.0f - 0.5f * tau + tau * tau / 6.0f;
    return float3(tau.x < 1e-3f ? series.x : regular.x,
                  tau.y < 1e-3f ? series.y : regular.y,
                  tau.z < 1e-3f ? series.z : regular.z);
}

// _densityReferenceRadius을 기준으로 _densityTransitionRadius의 높이를 반환
float atmosphereTransitionHeight()
{
    return _densityTransitionRadius - _densityReferenceRadius;
}

float2 atmosphereExponentialDensity(float height)
{
    // A numerical ceiling protects extreme editor values from exponential overflow.
    return exp(min(-height / max(float2(_rayleighHeight, _mieHeight), 1e-6f), 50.0f));
}

float4 atmosphereDensity(float height)
{
    const float transitionHeight = atmosphereTransitionHeight();
    if (height >= transitionHeight)
        return float4(atmosphereExponentialDensity(height), atmosphereAbsorptionDensity(height, 0u), atmosphereAbsorptionDensity(height, 1u));

    const float radius = max(_densityTransitionRadius, 1e-6f);
    const float depth = clamp(transitionHeight - height, 0.0f, radius);
    const float2 compression = 1.0f + depth * (1.0f - 0.5f * depth / radius) / max(float2(_rayleighHeight, _mieHeight), 1e-6f);
    return float4(atmosphereExponentialDensity(transitionHeight) * compression, atmosphereAbsorptionDensity(height, 0u), atmosphereAbsorptionDensity(height, 1u));
}

// Exact integral of the interior profile, matched to the density at the boundary.
float2 atmosphereInteriorOpticalDepth(float height, float radialDot, float pathLength)
{
    const float radius = max(_densityTransitionRadius, 1e-6f);
    const float localHeight = height - atmosphereTransitionHeight();
    const float c = localHeight * (2.0f * radius + localHeight);
    const float meanCompression = max(-(c + radialDot * pathLength
        + pathLength * pathLength / 3.0f) / (2.0f * radius), 0.0f);
    return atmosphereExponentialDensity(atmosphereTransitionHeight()) * pathLength
        * (1.0f + meanCompression / max(float2(_rayleighHeight, _mieHeight), 1e-6f));
}

// radialDot = dot(origin - planetCentre, unitDirection).
// This rational form avoids losing small heights next to a large planet.
float atmosphereHeightOnRay(float startHeight, float radialDot, float t)
{
    const float numerator = startHeight * (2.0f * _densityReferenceRadius + startHeight)
        + t * (2.0f * radialDot + t);
    const float radius = sqrt(max(_densityReferenceRadius * _densityReferenceRadius + numerator, 0.0f));
    return numerator / max(radius + _densityReferenceRadius, 1e-10f);
}

#include "Atmosphere/AtmosphereAbsorptionCore.hlsl"

// 적분 구간 안에서 밀도 변화의 중요한 지점을 찾아, 분할할 위치 t를 반환
float atmosphereSplitPoint(float heightAtReference, float radialDot, float startPos, float endPos)
{
    // radialDot은 방사방향을 rayDir로 투영했을 때의 거리 == 행성 중심에서 rayDir에 수선의 발을 내렸을때 만나는 지점까지의 거리
    // 즉, rayDir 방향에서 행성 중심까지 가장 가까운 위치. 즉 밀도가 가장 높은 부분

    const float margin = max((endPos - startPos) * 1e-5f, 1e-10f);
    if (-radialDot > startPos + margin && -radialDot < endPos - margin) // 구간 내에 radialDot이 있으면 그게 밀도가 가장 높은 부분이므로 바로 반환
        return -radialDot;

    const float heightAtTransition = heightAtReference - atmosphereTransitionHeight();
    const float c = heightAtTransition * (2.0f * _densityTransitionRadius + heightAtTransition); // 정밀도 때문에 이렇게 계산
    const float det = radialDot * radialDot - c;
    if (det >= 0.0f)    // TransitionRadius 구와 rayDir이 겹치는 점이 1~2개가 있음
    {
        const float root = sqrt(det);
        const float q = -radialDot - (radialDot >= 0.0f ? root : -root);
        const float t0 = q;
        const float t1 = abs(q) > 1e-10f ? c / q : -radialDot;
        const float nearT = min(t0, t1);
        const float farT = max(t0, t1);
        if (nearT > startPos + margin && nearT < endPos - margin) 
            return nearT;
        if (farT > startPos + margin && farT < endPos - margin) 
            return farT;
    }

    [unroll] 
    for (uint layer = 0u; layer < 2u; ++layer)
    {
        if (!any(atmosphereAbsorptionExtinction(layer) > 0.0f)) 
            continue;

        const float peak = _absorptionLayers[layer].peakRadius;
        const float radii[3] = { max(0.0f, peak - _absorptionLayers[layer].lowerWidth), peak, peak + _absorptionLayers[layer].upperWidth };
        [unroll]
        for (uint boundary = 0u; boundary < 3u; ++boundary)
        {
            const float2 hits = atmosphereAbsorptionSphereHits(heightAtReference, radialDot, radii[boundary]);
            if (hits.x > startPos + margin && hits.x < endPos - margin) 
                return hits.x;
            if (hits.y > startPos + margin && hits.y < endPos - margin) 
                return hits.y;
        }
    }

    return -1.0f;
}

struct AtmosphereOpticalDepth
{
    float4 value;
    uint limitReached;
};

AtmosphereOpticalDepth integrateAtmosphereOpticalDepth(float heightAtReference, float radialDot, float pathLength)
{
    AtmosphereOpticalDepth result;
    result.value = 0.0f;
    result.limitReached = 0u;

    if (pathLength <= 0.0f) 
        return result;

    const uint maxDepth = min(_integrationMaxDepth, 10u);
    const float relTolerance = max(_integrationRelativeTolerance, 1e-5f);
    const float3 betaR = atmosphereRayleighScattering();
    const float3 betaM = atmosphereMieExtinction();
    const float3 absorption0 = atmosphereAbsorptionExtinction(0u);
    const float3 absorption1 = atmosphereAbsorptionExtinction(1u);
    const float4 betaMax = float4(max(betaR.x, max(betaR.y, betaR.z)), max(betaM.x, max(betaM.y, betaM.z)), 
        max(absorption0.x, max(absorption0.y, absorption0.z)), max(absorption1.x, max(absorption1.y, absorption1.z)));
    const uint nodeBudget = clamp(_opticalNodeBudget, 64u, 128u);
    uint visitedNodes = 0u;
    float3 stack[12];   // x는 startpoint, y는 nextpoint, z는 depth
    [loop]
    for (uint seed = 0u; seed < _opticalDepthPointCount; ++seed)
    {
        uint size = 1u;
        stack[0] = float3(pathLength * (float(seed) / _opticalDepthPointCount), pathLength * (float(seed + 1u) / _opticalDepthPointCount), 0.0f);
        [loop]
        while (size > 0u)
        {
            ++visitedNodes;
            const float3 node = stack[--size];
            const float startPos = node.x;
            const float endPos = node.y;
            const float midPos = 0.5f * (startPos + endPos);
            const float width = endPos - startPos;
            const uint depth = uint(node.z);

            const float geometrySplit = atmosphereSplitPoint(heightAtReference, radialDot, startPos, endPos);
            const float4 coarse = atmosphereDensity(atmosphereHeightOnRay(heightAtReference, radialDot, midPos)) * width;
            const float split = geometrySplit >= 0.0f ? geometrySplit : midPos;
            const float4 fine = 
                atmosphereDensity(atmosphereHeightOnRay(heightAtReference, radialDot, 0.5f * (startPos + split))) 
                * (split - startPos) + atmosphereDensity(atmosphereHeightOnRay(heightAtReference, radialDot, 0.5f * (split + endPos))) * (endPos - split);
            const float4 error = abs(fine - coarse) * betaMax;
            const float4 tolerance = 1e-5f * (width / pathLength) + relTolerance * abs(fine * betaMax);
            const bool needsSplit = geometrySplit >= 0.0f || any(error > tolerance);
            if (needsSplit && depth < maxDepth && split > startPos && split < endPos && visitedNodes + size + (_opticalDepthPointCount - seed - 1u) + 2u <= nodeBudget)
            {
                stack[size++] = float3(split, endPos, depth + 1u);
                stack[size++] = float3(startPos, split, depth + 1u);
            }
            else
            {
                result.value += fine;
                result.limitReached |= needsSplit ? 1u : 0u;
            }
        }
    }

    return result;
}
#endif
