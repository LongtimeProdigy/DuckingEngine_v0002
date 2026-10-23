#define ATMOSPHERE_FORCE_LUT
#include "AtmosphereTransmittance.hlsl"
#include "AtmosphereSurfaceLighting.hlsl"
#include "CommonMathConstant.hlsl"
cbuffer SurfaceBounceOutputConstants : register(b2)
{
    uint _surfaceBounceOutputUav;
};
// Fixed three-point gas transport on local surface-to-probe segments: no nested adaptive integration.
float3 surfaceSegmentTransmission(float3 a, float3 b)
{
    const float3 delta = b - a;
    const float length = sqrt(dot(delta, delta));
    const float4 column = length * (
        atmosphereDensity(atmosphereHeight(a + delta * 0.1127016654f)) * (5.0f / 18.0f)
        + atmosphereDensity(atmosphereHeight(a + delta * 0.5f)) * (8.0f / 18.0f)
        + atmosphereDensity(atmosphereHeight(a + delta * 0.8872983346f)) * (5.0f / 18.0f));
    return exp(-atmosphereOpticalExtinction(column));
}
[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= SURFACE_BOUNCE_GRID * SURFACE_BOUNCE_GRID || id.y >= SURFACE_BOUNCE_GRID) return;
    const uint3 cell = uint3(id.x % SURFACE_BOUNCE_GRID, id.y, id.x / SURFACE_BOUNCE_GRID);
    const float3 uvw = (float3(cell) + 0.5f) / SURFACE_BOUNCE_GRID;
    const float3 p = atmosphereSurfacePoint(uvw);
    const float angle = radians(_sunDegree);
    const float3 sunDir = float3(0, sin(angle), cos(angle));
    const uint samples = clamp(_surfaceBounceSamples, 8u, 64u);
    const float scale = max(2.0f * max(_surfaceRight.w, _surfaceUp.w) / SURFACE_BOUNCE_GRID, 0.01f);
    const float texelArea = 4.0f * _surfaceRight.w * _surfaceUp.w / max(float(_surfaceMapSize * _surfaceMapSize), 1.0f);
    float3 bounce = 0.0f;
    if (_surfaceMapEnabled != 0u)
    {
        [loop] for (uint i = 0u; i < samples; ++i)
        {
            // Cauchy disk sampling concentrates work near the probe while retaining distant coverage.
            const float u = (float(i) + 0.5f) / samples;
            const float phi = 2.0f * PI * (float(reversebits(i)) * 2.3283064365386963e-10f);
            const float radius = scale * sqrt(u / max(1.0f - u, 1e-6f));
            const float2 sampleUV = uvw.xy + float2(cos(phi), sin(phi)) * radius / (2.0f * float2(_surfaceRight.w, _surfaceUp.w));
            if (any(sampleUV < 0.0f) || any(sampleUV >= 1.0f)) continue;
            const uint2 pixel = min(uint2(sampleUV * _surfaceMapSize), _surfaceMapSize - 1u);
            const float4 albedo = getTexture(_surfaceAlbedoIndex).Load(int3(pixel, 0));
            if (albedo.a <= 0.0f) continue;
            const float3 normalRaw = getTexture(_surfaceNormalIndex).Load(int3(pixel, 0)).xyz;
            const float3 normal = normalRaw * rsqrt(max(dot(normalRaw, normalRaw), 1e-12f));
            if (dot(normal, sunDir) <= 1e-4f) continue;
            const float depth = getTexture(_surfaceDepthIndex).Load(int3(pixel, 0)).x;
            const float3 q = atmosphereSurfacePoint(float3((float2(pixel) + 0.5f) / _surfaceMapSize, depth));
            const float3 delta = p - q;
            const float distance2 = max(dot(delta, delta), max(texelArea / PI, 1e-6f));
            const float cosine = saturate(dot(normal, delta * rsqrt(distance2)));
            const float metallic = saturate(getTexture(_surfaceMaterialIndex).Load(int3(pixel, 0)).x);
            // Projected texel area already contains the incoming cosine: do not multiply it twice.
            const float inversePdf = PI * scale * scale / max((1.0f - u) * (1.0f - u), 1e-6f);
            const float3 diffuseFlux = atmosphereSunIrradiance() * getSunTransmittance(q, sunDir)
                * saturate(albedo.rgb) * (1.0f - metallic) / PI;
            bounce += diffuseFlux * surfaceSegmentTransmission(q, p) * (cosine * inversePdf / (distance2 * samples));
        }
    }
    // Bound numerical/sampling spikes by the maximum diffuse radiance over a full sphere.
    bounce = min(max(bounce, 0.0f), 4.0f * atmosphereSunIrradiance());
    const RWTexture2D<float4> output = getTextureRW(_surfaceBounceOutputUav);
    output[id.xy] = float4(bounce, 1);
}
