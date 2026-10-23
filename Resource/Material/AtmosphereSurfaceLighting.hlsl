#ifndef ATMOSPHERE_SURFACE_LIGHTING_HLSL
#define ATMOSPHERE_SURFACE_LIGHTING_HLSL
#include "CommonTexture.hlsl"
#include "AtmosphereParameters.hlsl"
static const uint SURFACE_BOUNCE_GRID = 16u;
float3 atmosphereSurfaceCoordinates(float3 p)
{
    const float3 local = p - _surfaceOrigin.xyz;
    return float3(0.5f + 0.5f * dot(local, _surfaceRight.xyz) / max(_surfaceRight.w, 1e-6f),
                  0.5f - 0.5f * dot(local, _surfaceUp.xyz) / max(_surfaceUp.w, 1e-6f),
                  dot(local, _surfaceForward.xyz) / max(_surfaceForward.w, 1e-6f));
}
float3 atmosphereSurfacePoint(float3 uvw)
{
    return _surfaceOrigin.xyz + _surfaceRight.xyz * ((2.0f * uvw.x - 1.0f) * _surfaceRight.w)
        + _surfaceUp.xyz * ((1.0f - 2.0f * uvw.y) * _surfaceUp.w) + _surfaceForward.xyz * (uvw.z * _surfaceForward.w);
}
bool atmosphereInsideSurfaceCapture(float3 uvw)
{
    return all(uvw >= 0.0f) && all(uvw <= 1.0f);
}
float atmosphereSurfaceShadowTap(float3 uvw)
{
    if (!atmosphereInsideSurfaceCapture(uvw)) return 1.0f;
    const uint2 pixel = min(uint2(uvw.xy * _surfaceMapSize), _surfaceMapSize - 1u);
    const float covered = getTexture(_surfaceAlbedoIndex).Load(int3(pixel, 0)).a;
    if (covered <= 0.0f) return 1.0f; // Empty texels are gas, never an implicit planet surface.
    const float depth = getTexture(_surfaceDepthIndex).Load(int3(pixel, 0)).x;
    return uvw.z <= depth + max(_surfaceBias, 0.0f) / max(_surfaceForward.w, 1e-6f) ? 1.0f : 0.0f;
}
float3 atmosphereCascadeCoordinates(float3 p, SurfaceShadowCascade c)
{
    const float3 local = p - c.origin.xyz;
    return float3(0.5f + 0.5f * dot(local, c.right.xyz) / max(c.right.w, 1e-6f),
                  0.5f - 0.5f * dot(local, c.up.xyz) / max(c.up.w, 1e-6f),
                  dot(local, c.forward.xyz) / max(c.forward.w, 1e-6f));
}
float atmosphereCascadeTap(float3 uvw, SurfaceShadowCascade c)
{
    if (!atmosphereInsideSurfaceCapture(uvw)) return 1.0f;
    const uint size = max(c.mapSize, 1u);
    const uint2 pixel = min(uint2(uvw.xy * size), size - 1u);
    // Cascade selection can differ between lanes in a pixel/compute wave.
    const float covered = getTexture(NonUniformResourceIndex(c.albedoIndex)).Load(int3(pixel, 0)).a;
    if (covered <= 0.0f) return 1.0f;
    const float depth = getTexture(NonUniformResourceIndex(c.depthIndex)).Load(int3(pixel, 0)).x;
    return uvw.z <= depth + max(_surfaceBias, 0.0f) / max(c.forward.w, 1e-6f) ? 1.0f : 0.0f;
}
float atmosphereCascadedVisibility(float3 p, bool filtered)
{
    float visibility = 0.0f, remaining = 1.0f;
    [loop] for (uint i = 0u; i < min(_shadowCascadeCount, 4u); ++i)
    {
        const SurfaceShadowCascade c = _shadowCascades[i];
        const float3 uvw = atmosphereCascadeCoordinates(p, c);
        if (!atmosphereInsideSurfaceCapture(uvw)) continue;
        // Blend into the next enclosing cascade; the last one fades to unshadowed.
        const float edge = min(min(uvw.x, 1.0f - uvw.x), min(uvw.y, 1.0f - uvw.y));
        const float band = max(clamp(_shadowCascadeBlend, 0.0f, 0.5f), 2.0f / max(float(c.mapSize), 1.0f));
        const float weight = smoothstep(0.0f, band, edge);
        if (weight <= 0.0f) continue;
        float sampleVisibility = 0.0f;
        if (filtered)
        {
            const float delta = 0.5f / max(float(c.mapSize), 1.0f);
            [unroll] for (uint tap = 0u; tap < 4u; ++tap)
            {
                const float2 offset = float2((tap & 1u) ? delta : -delta, (tap & 2u) ? delta : -delta);
                sampleVisibility += atmosphereCascadeTap(uvw + float3(offset, 0), c) * 0.25f;
            }
        }
        else sampleVisibility = atmosphereCascadeTap(uvw, c);
        // All cascades share depth coverage. Fade once per sample at the depth boundary.
        const float depthWeight = smoothstep(0.0f, 0.01f, min(uvw.z, 1.0f - uvw.z));
        sampleVisibility = lerp(1.0f, sampleVisibility, depthWeight);
        visibility += remaining * weight * sampleVisibility;
        remaining *= 1.0f - weight;
        if (remaining <= 0.0f) break;
    }
    return visibility + remaining;
}
float atmosphereSurfaceSunVisibility(float3 p)
{
    if (_surfaceMapEnabled == 0u || _surfaceShadowStrength <= 0.0f) return 1.0f;
    if (_shadowCascadeCount > 0u)
        return lerp(1.0f, atmosphereCascadedVisibility(p, false), saturate(_surfaceShadowStrength));
    return lerp(1.0f, atmosphereSurfaceShadowTap(atmosphereSurfaceCoordinates(p)), saturate(_surfaceShadowStrength));
}
float atmosphereSurfaceSunVisibilityFiltered(float3 p, float3 normal)
{
    if (_surfaceMapEnabled == 0u || _surfaceShadowStrength <= 0.0f) return 1.0f;
    if (_shadowCascadeCount > 0u)
        return lerp(1.0f, atmosphereCascadedVisibility(p + normal * max(_surfaceBias, 0.0f), true), saturate(_surfaceShadowStrength));
    const float3 uvw = atmosphereSurfaceCoordinates(p + normal * max(_surfaceBias, 0.0f));
    const float delta = 0.5f / max(float(_surfaceMapSize), 1.0f);
    float visibility = 0.0f;
    [unroll] for (uint i = 0u; i < 4u; ++i)
    {
        const float2 offset = float2((i & 1u) != 0u ? delta : -delta, (i & 2u) != 0u ? delta : -delta);
        visibility += atmosphereSurfaceShadowTap(uvw + float3(offset, 0));
    }
    return lerp(1.0f, visibility * 0.25f, saturate(_surfaceShadowStrength));
}
float3 atmosphereSurfaceBounce(float3 p)
{
    if (_surfaceMapEnabled == 0u || _surfaceBounceStrength <= 0.0f) return 0.0f;
    const float3 uvw = atmosphereSurfaceCoordinates(p);
    if (!atmosphereInsideSurfaceCapture(uvw)) return 0.0f;
    const float3 grid = uvw * SURFACE_BOUNCE_GRID - 0.5f;
    const int3 base = int3(floor(grid));
    const float3 f = frac(grid);
    float3 result = 0.0f;
    const Texture2D<float4> field = getTexture(_surfaceBounceIndex);
    [unroll] for (uint i = 0u; i < 8u; ++i)
    {
        const int3 corner = int3(i & 1u, (i >> 1u) & 1u, (i >> 2u) & 1u);
        const int3 cell = clamp(base + corner, 0, int(SURFACE_BOUNCE_GRID) - 1);
        const float3 weight = lerp(1.0f - f, f, float3(corner));
        result += field.Load(int3(cell.x + cell.z * int(SURFACE_BOUNCE_GRID), cell.y, 0)).rgb * weight.x * weight.y * weight.z;
    }
    return max(result, 0.0f) * max(_surfaceBounceStrength, 0.0f);
}
#endif
