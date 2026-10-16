#ifndef ATMOSPHERE_INTEGRATION_HLSL
#define ATMOSPHERE_INTEGRATION_HLSL
#include "AtmosphereParameters.hlsl"

static const float3 gStandardBetaR = float3(3.8e-6f, 13.5e-6f, 33.1e-6f);
static const float3 gStandardBetaM = float3(21e-6f, 21e-6f, 21e-6f);

// Heights remain relative to the density-1 radius; this boundary is independent.
float atmosphereTransitionHeight()
{
    return _densityTransitionRadius - _densityReferenceRadius;
}

float2 atmosphereExponentialDensity(float height)
{
    // A numerical ceiling protects extreme editor values from exponential overflow.
    return exp(min(-height / max(float2(_rayleighHeight, _mieHeight), 1e-6f), 50.0f));
}

float2 atmosphereDensity(float height)
{
    const float transitionHeight = atmosphereTransitionHeight();
    if (height >= transitionHeight)
        return atmosphereExponentialDensity(height);

    const float radius = max(_densityTransitionRadius, 1e-6f);
    const float depth = clamp(transitionHeight - height, 0.0f, radius);
    const float2 compression = 1.0f + depth * (1.0f - 0.5f * depth / radius)
        / max(float2(_rayleighHeight, _mieHeight), 1e-6f);
    return atmosphereExponentialDensity(transitionHeight) * compression;
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

// Split at the closest approach and density-profile boundary. Neither is a surface hit.
float atmosphereSplitPoint(float height, float radialDot, float a, float b)
{
    const float margin = max((b - a) * 1e-5f, 1e-10f);
    if (-radialDot > a + margin && -radialDot < b - margin)
        return -radialDot;

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
    return -1.0f;
}

struct AtmosphereOpticalDepth
{
    float2 value;
    uint limitReached;
};

// Bounded depth-first subdivision. At the limit, accept the two-half estimate:
// never discard the unprocessed tail of a ray. limitReached exposes failed tolerance.
AtmosphereOpticalDepth integrateAtmosphereOpticalDepth(float startHeight, float radialDot, float pathLength, uint seedCount)
{
    AtmosphereOpticalDepth result;
    result.value = float2(0, 0);
    result.limitReached = 0u;

    if (pathLength <= 0.0f) 
        return result;

    const uint seeds = clamp(seedCount, 1u, 64u);
    const uint maxDepth = min(_integrationMaxDepth, 10u);
    const float relTolerance = max(_integrationRelativeTolerance, 1e-5f);
    // Each component is judged in optical-depth units, including its extinction strength.
    const float2 betaMax = float2(33.1e-6f, 21e-6f) * max(_scatteringScale, 0.0f);
    const uint nodeBudget = 128u;
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
            const float2 coarse = atmosphereDensity(atmosphereHeightOnRay(startHeight, radialDot, m)) * width;
            const float2 fine = (atmosphereDensity(atmosphereHeightOnRay(startHeight, radialDot, 0.5f * (a + m)))
                              + atmosphereDensity(atmosphereHeightOnRay(startHeight, radialDot, 0.5f * (m + b)))) * (0.5f * width);
            const float2 error = abs(fine - coarse) * betaMax;
            const float2 tolerance = 1e-5f * (width / pathLength) + relTolerance * abs(fine * betaMax);
            const bool needsSplit = geometrySplit >= 0.0f || any(error > tolerance);
            const float split = geometrySplit >= 0.0f ? geometrySplit : m;
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
