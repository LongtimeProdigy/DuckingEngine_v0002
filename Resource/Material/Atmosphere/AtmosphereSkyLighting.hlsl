#ifndef ATMOSPHERE_SKY_LIGHTING_HLSL
#define ATMOSPHERE_SKY_LIGHTING_HLSL
#include "CommonTexture.hlsl"
#include "CommonMathConstant.hlsl"
#include "Atmosphere/AtmosphereParameters.hlsl"
static const uint SKY_WIDTH = 64u;
static const uint SKY_HEIGHT = 32u;
static const uint SKY_BANDS = 6u; // 0..4: GGX roughness, 5: diffuse irradiance.
float3 skyDirection(float2 uv)
{
    const float phi = uv.x * 2.0f * PI;
    const float theta = uv.y * PI;
    return float3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
}
float2 skyCoordinates(float3 direction)
{
    return float2(frac(atan2(direction.z, direction.x) / (2.0f * PI)), acos(clamp(direction.y, -1.0f, 1.0f)) / PI);
}
// Wrap longitude, clamp latitude, and never interpolate between atlas bands.
float3 sampleSkyBand(uint textureIndex, float3 direction, uint band)
{
    const float2 grid = skyCoordinates(normalize(direction)) * float2(SKY_WIDTH, SKY_HEIGHT) - 0.5f;
    const int2 p = int2(floor(grid));
    const float2 f = frac(grid);
    const int x0 = (p.x + int(SKY_WIDTH)) % int(SKY_WIDTH);
    const int x1 = (x0 + 1) % int(SKY_WIDTH);
    const int y0 = clamp(p.y, 0, int(SKY_HEIGHT) - 1) + int(band * SKY_HEIGHT);
    const int y1 = clamp(p.y + 1, 0, int(SKY_HEIGHT) - 1) + int(band * SKY_HEIGHT);
    const Texture2D<float4> source = getTexture(textureIndex);
    return lerp(lerp(source.Load(int3(x0, y0, 0)).rgb, source.Load(int3(x1, y0, 0)).rgb, f.x),
                lerp(source.Load(int3(x0, y1, 0)).rgb, source.Load(int3(x1, y1, 0)).rgb, f.x), f.y);
}
float3 atmosphereSkyAmbient(float3 N, float3 V, float3 baseColor, float metallic, float roughness, float ao)
{
    if (_skyLightingStrength <= 0.0f) return 0.0f;
    const float NoV = saturate(dot(N, V));
    const float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), baseColor, metallic);
    const float3 F = F0 + (max(1.0f - roughness, F0) - F0) * pow(1.0f - NoV, 5.0f);
    const float3 irradiance = sampleSkyBand(_skyFilteredIndex, N, 5u);
    const float level = saturate(roughness) * 4.0f;
    const uint band0 = uint(level), band1 = min(band0 + 1u, 4u);
    const float3 R = reflect(-V, N);
    const float3 reflection = lerp(sampleSkyBand(_skyFilteredIndex, R, band0),
                                  sampleSkyBand(_skyFilteredIndex, R, band1), frac(level));
    // Local camera probe; approximate specular IBL (no local visibility or multiscattering).
    return ((1.0f - F) * (1.0f - metallic) * baseColor * irradiance / PI + F * reflection)
        * saturate(ao) * max(_skyLightingStrength, 0.0f);
}
#endif
