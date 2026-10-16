#ifndef ATMOSPHERE_TRANSMITTANCE_HLSL
#define ATMOSPHERE_TRANSMITTANCE_HLSL
#include "CommonTexture.hlsl"
#include "AtmosphereIntegration.hlsl"
#include "AtmosphereGeometry.hlsl"

// Match SceneRenderer's allocation. Float columns avoid losing small transmittances.
static const uint ATMOSPHERE_LUT_WIDTH = 512u;
static const uint ATMOSPHERE_LUT_HEIGHT = 128u;

float atmosphereTopDistance(float height, float radialDot)
{
    const float altitude = height - (_atmosphereRadius - _densityReferenceRadius);
    const float c = altitude * (2.0f * _atmosphereRadius + altitude);
    const float root = sqrt(max(radialDot * radialDot - c, 0.0f));
    const float q = -radialDot - (radialDot >= 0.0f ? root : -root);
    return max(max(q, abs(q) > 1e-10f ? c / q : -radialDot), 0.0f);
}

float atmosphereHorizonCosine(float height)
{
    const float localHeight = height - atmosphereTransitionHeight();
    return -sqrt(max(localHeight * (2.0f * _densityTransitionRadius + localHeight), 0.0f))
        / max(_densityReferenceRadius + height, 1e-10f);
}

// Both inward and outward rays are represented. The LUT itself never assumes ground occlusion.
// Angular resolution is concentrated around the tangent to the density transition sphere.
float2 atmosphereLutParameters(float2 uv)
{
    const float height = atmosphereTransitionHeight()
        + uv.y * uv.y * (_atmosphereRadius - _densityTransitionRadius);
    const float horizon = atmosphereHorizonCosine(height);
    const float s = 2.0f * uv.x - 1.0f;
    const float mu = s >= 0.0f ? horizon + (1.0f - horizon) * s * s
                              : horizon - (1.0f + horizon) * s * s;
    return float2(height, mu);
}

float2 atmosphereLutCoordinates(float height, float mu)
{
    const float horizon = atmosphereHorizonCosine(height);
    const float s = mu >= horizon ? sqrt(saturate((mu - horizon) / max(1.0f - horizon, 1e-10f))) : -sqrt(saturate((horizon - mu) / max(1.0f + horizon, 1e-10f)));
    return float2(0.5f * (s + 1.0f), sqrt(saturate((height - atmosphereTransitionHeight()) / max(_atmosphereRadius - _densityTransitionRadius, 1e-10f))));
}

AtmosphereOpticalDepth atmosphereSunOpticalDepthOutsideCore(float height, float radialDot, float pathLength)
{
    if (pathLength <= 0.0f)
    {
        AtmosphereOpticalDepth empty;
        empty.value = float2(0, 0);
        empty.limitReached = 0u;
        return empty;
    }

    // The LUT covers the exponential region, including negative reference altitudes.
    if (_atmosphereSamplingMode != ATEMOSPHERE_PRODUCTION || height < atmosphereTransitionHeight() || height > _atmosphereRadius - _densityReferenceRadius)
        return integrateAtmosphereOpticalDepth(height, radialDot, pathLength, _opticalDepthPointCount);

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
    result.value = max(lerp(lerp(a.xy, b.xy, blend.x), lerp(c.xy, d.xy, blend.x), blend.y), 0.0f);
    result.limitReached = max(max(a.z, b.z), max(c.z, d.z)) > 0.0f ? 1u : 0u;
    return result;
}
// Integrate the quadratic interior density analytically, then reuse the
// outward surface LUT. This avoids nested per-pixel integration in production.
AtmosphereOpticalDepth atmosphereSunOpticalDepth(float height, float radialDot, float pathLength)
{
    if (height >= atmosphereTransitionHeight() || pathLength <= 0.0f)
        return atmosphereSunOpticalDepthOutsideCore(height, radialDot, pathLength);

    const float localHeight = height - atmosphereTransitionHeight();
    const float c = localHeight * (2.0f * _densityTransitionRadius + localHeight);
    const float root = sqrt(max(radialDot * radialDot - c, 0.0f));
    const float exitDistance = radialDot >= 0.0f ? -c / max(root + radialDot, 1e-10f) : root - radialDot;
    const float coreLength = min(max(exitDistance, 0.0f), pathLength);
    AtmosphereOpticalDepth result;
    result.value = atmosphereInteriorOpticalDepth(height, radialDot, coreLength);
    result.limitReached = 0u;
    if (coreLength < pathLength)
    {
        const AtmosphereOpticalDepth outer = atmosphereSunOpticalDepthOutsideCore(
            atmosphereTransitionHeight(), max(radialDot + coreLength, 0.0f), pathLength - coreLength);
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
    const AtmosphereOpticalDepth optical = atmosphereSunOpticalDepth(
        height, dot(origin - _planetCentre, sunDir), hit.y);
    const float3 betaR = gStandardBetaR * _scatteringScale;
    const float3 betaM = gStandardBetaM * _scatteringScale;
    return exp(-betaR * optical.value.x - betaM * optical.value.y);
}
#endif
