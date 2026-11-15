#ifndef TERRAIN_SURFACE_HLSL
#define TERRAIN_SURFACE_HLSL

#include "TerrainHeight.hlsl"
#include "GridHash.hlsl"

cbuffer TerrainClipMap : register(b1)
{
    TextureParameter _diffuseTexture;
    float _metallic;
    float _roughness;
    TextureParameter _soilDiffuseTexture;
    TextureParameter _mossSurfaceTexture;
    TextureParameter _soilSurfaceTexture;
    TextureParameter _rockDiffuseTexture;
    TextureParameter _rockSurfaceTexture;
    TextureParameter _groundCoverageTexture;
    float _groundColorNearDistance;
    float _groundColorFarDistance;
    float _groundCoverageOriginX;
    float _groundCoverageOriginZ;
    float _groundCoverageWidth;
    float _groundCoverageDepth;
    float _groundCoverageFadeMeters;
    float _groundCoverageExteriorMeadow;
}

float TerrainNoise(float2 position)
{
    const int2 cell = int2(floor(position));
    const float2 fraction = frac(position);
    const float2 weight = fraction * fraction * (3.0f - 2.0f * fraction);
    const float4 cornerValues = float4(
        HashGridCell(cell) & 0xffffu,
        HashGridCell(cell + int2(1, 0)) & 0xffffu,
        HashGridCell(cell + int2(0, 1)) & 0xffffu,
        HashGridCell(cell + int2(1, 1)) & 0xffffu) / 65535.0f;
    return lerp(lerp(cornerValues.x, cornerValues.y, weight.x), lerp(cornerValues.z, cornerValues.w, weight.x), weight.y);
}

float TerrainMacro(float2 worldXZ)
{
    return 0.68f * TerrainNoise(worldXZ / 52.0f + float2(9.7f, -3.4f)) + 0.32f * TerrainNoise(worldXZ / 13.0f);
}

float TerrainMossCoverage(float macro)
{
    return lerp(0.12f, 0.88f, smoothstep(0.31f, 0.69f, macro));
}

#endif
