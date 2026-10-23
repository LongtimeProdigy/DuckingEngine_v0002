#ifndef ATMOSPHERE_INTEGRATION_HLSL
#define ATMOSPHERE_INTEGRATION_HLSL
#include "AtmosphereParameters.hlsl"

static const float3 gStandardBetaR = float3(3.8e-6f, 13.5e-6f, 33.1e-6f);
static const float3 gStandardBetaM = float3(21e-6f, 21e-6f, 21e-6f);

float3 atmosphereRayleighScattering() { return gStandardBetaR * max(_scatteringScale, 0.0f) * max(_rayleighScale, 0.0f); }
float3 atmosphereMieScattering() { return gStandardBetaM * max(_scatteringScale, 0.0f) * max(_mieScale, 0.0f); }
float3 atmosphereMieExtinction()
{
    return gStandardBetaM * max(_scatteringScale, 0.0f)
        * (max(_mieScale, 0.0f) + max(_mieAbsorptionScale, 0.0f));
}
float3 atmosphereAbsorptionExtinction(uint layer)
{
    return max(_absorptionLayers[layer].extinction, 0.0f)
        * max(_absorptionLayers[layer].strength, 0.0f) * max(_scatteringScale, 0.0f);
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

// 밀도가 1인 _densityReferenceRadius을 기준으로 _densityTransitionRadius의 높이를 반환
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

#include "AtmosphereAbsorptionCore.hlsl"

// Split at the closest approach and density-profile boundary. Neither is a surface hit.
float atmosphereSplitPoint(float height, float radialDot, float a, float b)
{
    const float margin = max((b - a) * 1e-5f, 1e-10f);
    if (-radialDot > a + margin && -radialDot < b - margin)
    {
        // -radialDot은 사실상 a-b 구간의 ray에서 atmosphere중심까지의 최단거리를 나타낸다.
        // 그 최단거리가 a-b 구간에 존재하면 다로 최단거리를 내보냄
        return -radialDot;
    }

    const float localHeight = height - atmosphereTransitionHeight();
    const float c = localHeight * (2.0f * _densityTransitionRadius + localHeight);
    const float det = radialDot * radialDot - c;
    if (det >= 0.0f)
    {
        const float root = sqrt(det);
        const float q = -radialDot - (radialDot >= 0.0f ? root : -root);
        const float t0 = q;
        const float t1 = abs(q) > 1e-10f ? c / q : -radialDot;
        const float nearT = min(t0, t1);
        const float farT = max(t0, t1);
        if (nearT > a + margin && nearT < b - margin) 
            return nearT;
        if (farT > a + margin && farT < b - margin) 
            return farT;
    }
    // Thin absorption shells need explicit boundaries, independently of the density transition.
    [unroll] for (uint layer = 0u; layer < 2u; ++layer)
    {
        if (!any(atmosphereAbsorptionExtinction(layer) > 0.0f)) continue;
        const float peak = _absorptionLayers[layer].peakRadius;
        const float radii[3] = { max(0.0f, peak - _absorptionLayers[layer].lowerWidth), peak, peak + _absorptionLayers[layer].upperWidth };
        [unroll] for (uint boundary = 0u; boundary < 3u; ++boundary)
        {
            const float2 hits = atmosphereAbsorptionSphereHits(height, radialDot, radii[boundary]);
            if (hits.x > a + margin && hits.x < b - margin) return hits.x;
            if (hits.y > a + margin && hits.y < b - margin) return hits.y;
        }
    }
    return -1.0f;
}

struct AtmosphereOpticalDepth
{
    float4 value;
    uint limitReached;
};

// Bounded depth-first subdivision. At the limit, accept the two-half estimate:
// never discard the unprocessed tail of a ray. limitReached exposes failed tolerance.
AtmosphereOpticalDepth integrateAtmosphereOpticalDepth(float startHeight, float radialDot, float pathLength, uint seedCount)
{
    AtmosphereOpticalDepth result;
    result.value = 0.0f;
    result.limitReached = 0u;

    if (pathLength <= 0.0f) 
        return result;

    const uint seeds = clamp(seedCount, 1u, 64u);
    const uint maxDepth = min(_integrationMaxDepth, 10u);
    const float relTolerance = max(_integrationRelativeTolerance, 1e-5f);
    // Each component is judged in optical-depth units, including its extinction strength.
    const float3 betaR = atmosphereRayleighScattering();
    const float3 betaM = atmosphereMieExtinction();
    const float3 absorption0 = atmosphereAbsorptionExtinction(0u), absorption1 = atmosphereAbsorptionExtinction(1u);
    const float4 betaMax = float4(max(betaR.x, max(betaR.y, betaR.z)), max(betaM.x, max(betaM.y, betaM.z)),
        max(absorption0.x, max(absorption0.y, absorption0.z)), max(absorption1.x, max(absorption1.y, absorption1.z)));
    const uint nodeBudget = clamp(_opticalNodeBudget, 64u, 128u);
    uint visitedNodes = 0u;
    float3 stack[12];
    [loop]
    for (uint seed = 0u; seed < seeds; ++seed)
    {
        uint size = 1u;
        stack[0] = float3(pathLength * (float(seed) / seeds), pathLength * (float(seed + 1u) / seeds), 0.0f);
        [loop]
        while (size > 0u)
        {
            ++visitedNodes;
            const float3 node = stack[--size];
            const float a = node.x;
            const float b = node.y;
            const float m = 0.5f * (a + b);
            const float width = b - a;
            const uint depth = uint(node.z);
            const float geometrySplit = atmosphereSplitPoint(startHeight, radialDot, a, b);
            const float4 coarse = atmosphereDensity(atmosphereHeightOnRay(startHeight, radialDot, m)) * width;
            const float split = geometrySplit >= 0.0f ? geometrySplit : m;
            // Honor known boundaries even when the subdivision budget is exhausted.
            const float4 fine = atmosphereDensity(atmosphereHeightOnRay(startHeight, radialDot, 0.5f * (a + split))) * (split - a)
                             + atmosphereDensity(atmosphereHeightOnRay(startHeight, radialDot, 0.5f * (split + b))) * (b - split);
            const float4 error = abs(fine - coarse) * betaMax;
            const float4 tolerance = 1e-5f * (width / pathLength) + relTolerance * abs(fine * betaMax);
            const bool needsSplit = geometrySplit >= 0.0f || any(error > tolerance);
            if (needsSplit && depth < maxDepth && split > a && split < b
                && visitedNodes + size + (seeds - seed - 1u) + 2u <= nodeBudget)
            {
                stack[size++] = float3(split, b, depth + 1u);
                stack[size++] = float3(a, split, depth + 1u);
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
