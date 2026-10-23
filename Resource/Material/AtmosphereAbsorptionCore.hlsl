#ifndef ATMOSPHERE_ABSORPTION_CORE_HLSL
#define ATMOSPHERE_ABSORPTION_CORE_HLSL
// Bounded shell quadrature below the density transition. Rayleigh/Mie keep their analytic core.
float2 atmosphereAbsorptionSphereHits(float height, float radialDot, float radius)
{
    if (radius <= 0.0f) return float2(1e30f, -1e30f);
    const float r = max(_densityReferenceRadius + height, 0.0f);
    const float impact = sqrt(max((r - abs(radialDot)) * (r + abs(radialDot)), 0.0f));
    if (impact > radius) return float2(1e30f, -1e30f);
    const float root = sqrt(max((radius - impact) * (radius + impact), 0.0f));
    const float h = height - (radius - _densityReferenceRadius);
    const float c = h * (2.0f * radius + h);
    const float q = -radialDot - (radialDot >= 0.0f ? root : -root);
    const float other = abs(q) > 1e-10f ? c / q : -radialDot;
    return float2(min(q, other), max(q, other));
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
float2 atmosphereInteriorAbsorptionDepth(float height, float radialDot, float length)
{
    float2 result = 0.0f;
    [unroll] for (uint layer = 0u; layer < 2u; ++layer)
    {
        if (!any(atmosphereAbsorptionExtinction(layer) > 0.0f)) continue;
        const float peak = max(_absorptionLayers[layer].peakRadius, 0.0f);
        result[layer] = atmosphereAbsorptionShell(height, radialDot, length,
            max(peak - _absorptionLayers[layer].lowerWidth, 0.0f), peak, layer)
            + atmosphereAbsorptionShell(height, radialDot, length, peak,
                peak + max(_absorptionLayers[layer].upperWidth, 0.0f), layer);
    }
    return result;
}
#endif
