#ifndef TERRAIN_SURFACE_HLSL
#define TERRAIN_SURFACE_HLSL

#include "CommonTexture.hlsl"

cbuffer TerrainClipMap : register(b1)
{
    TextureParameter _diffuseTexture;
    TextureParameter _heightTexture;
    float _metallic;
    float _roughness;
    TextureParameter _soilDiffuseTexture;
}

float SampleTerrainHeight(float2 worldXZ)
{
    const float metersPerSample = 1.0f;
    const float minHeight = -16.0f;
    const float heightRange = 128.0f;
    Texture2D<float4> heightTexture = getTexture(_heightTexture);

    uint2 textureSize;
    heightTexture.GetDimensions(textureSize.x, textureSize.y);
    const float2 lastTexel = float2(textureSize) - 1.0f;
    const float2 texel = clamp(worldXZ / metersPerSample + 0.5f * lastTexel, 0.0f, lastTexel);
    const float2 uv = (texel + 0.5f) / float2(textureSize);
    return minHeight + heightRange * heightTexture.SampleLevel(bilinearRepeatSampler, uv, 0.0f).x;
}

uint HashTerrainCell(int2 cell)
{
    uint hash = asuint(cell.x) * 0x8da6b343u ^ asuint(cell.y) * 0xd8163841u;
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    hash ^= hash >> 15;
    return hash;
}

float TerrainNoise(float2 position)
{
    const int2 cell = int2(floor(position));
    const float2 fraction = frac(position);
    const float2 weight = fraction * fraction * (3.0f - 2.0f * fraction);
    const float4 cornerValues = float4(
        HashTerrainCell(cell) & 0xffffu, HashTerrainCell(cell + int2(1, 0)) & 0xffffu,
        HashTerrainCell(cell + int2(0, 1)) & 0xffffu, HashTerrainCell(cell + int2(1, 1)) & 0xffffu) / 65535.0f;
    return lerp(lerp(cornerValues.x, cornerValues.y, weight.x),
                lerp(cornerValues.z, cornerValues.w, weight.x), weight.y);
}

float TerrainMacro(float2 worldXZ)
{
    return 0.68f * TerrainNoise(worldXZ / 52.0f + float2(9.7f, -3.4f))
         + 0.32f * TerrainNoise(worldXZ / 13.0f);
}

float TerrainMossCoverage(float macro)
{
    return lerp(0.12f, 0.88f, smoothstep(0.31f, 0.69f, macro));
}

float TerrainGrassDensity(float2 worldXZ, float moss)
{
    const float densityNoise = TerrainNoise(worldXZ / 3.7f + float2(17.3f, -8.1f));
    const float pathCenter = 1.9f * sin(worldXZ.y * 0.24f);
    const float pathDistance = abs(worldXZ.x - pathCenter);
    const float entrancePath = smoothstep(-5.0f, 0.0f, worldXZ.y) * (1.0f - smoothstep(30.0f, 39.0f, worldXZ.y));
    const float pathMask = lerp(1.0f, smoothstep(0.6f, 2.25f, pathDistance), entrancePath);
    return lerp(0.18f, 0.96f, smoothstep(0.23f, 0.76f, 0.64f * densityNoise + 0.36f * moss)) * pathMask;
}

#endif
