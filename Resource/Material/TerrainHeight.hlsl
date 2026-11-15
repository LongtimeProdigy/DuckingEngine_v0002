#ifndef TERRAIN_HEIGHT_HLSL
#define TERRAIN_HEIGHT_HLSL

#include "CommonTexture.hlsl"

cbuffer TerrainStreamingConstants : register(b3)
{
    uint _terrainStreamEnabled;
    uint _terrainHeightCache;
    uint _terrainNormalCache;
    uint _terrainPageTable;
    uint _terrainTailHeight;
    uint _terrainTailNormal;
    uint _terrainCacheColumns;
    uint _terrainCacheRows;
    uint _terrainCacheSide;
    float _terrainSampleSpacing;
    float _terrainHeightOffset;
    float _terrainHeightScale;
    uint _terrainCellsX;
    uint _terrainCellsZ;
    uint _terrainChunkCells;
    uint _terrainTilesX;
    uint _terrainTilesZ;
    uint _terrainTailLod;
    uint _terrainTailWidth;
    uint _terrainTailHeightPixels;
    uint _terrainPageTableSide;
    float _terrainCameraX;
    float _terrainCameraZ;
    int _terrainAnchorX;
    int _terrainAnchorZ;
    float _terrainCameraWorldX;
    float _terrainCameraWorldZ;
    float _terrainDetailRadius;
    uint _terrainHeightTexture;
    float _terrainWorldOriginX;
    float _terrainWorldOriginZ;
}

float2 TerrainAnchorWorldXZ()
{
    return float2(_terrainWorldOriginX, _terrainWorldOriginZ) + float2(_terrainAnchorX, _terrainAnchorZ) * _terrainSampleSpacing;
}

float4 SampleTerrainPage(uint2 tile, uint lod)
{
    const uint2 address = uint2(tile.x % _terrainPageTableSide, tile.y % _terrainPageTableSide + lod * _terrainPageTableSide);
    const float4 page = getTexture(_terrainPageTable).Load(int3(address, 0));
    if (any(uint2(page.zw) != tile))
    {
        return 0.0f;
    }
    return page;
}

float TerrainPageFade(int2 tile, uint lod)
{
    const uint cells = _terrainChunkCells << lod;
    const int2 count = int2((_terrainCellsX - 1u) / cells + 1u, (_terrainCellsZ - 1u) / cells + 1u);
    if (any(tile < 0) || any(tile >= count))
    {
        return 1.0f;
    }
    return SampleTerrainPage(uint2(tile), lod).y;
}

float3 DecodeTerrainNormal(float2 xz)
{
    return normalize(float3(xz.x, sqrt(saturate(1.0f - dot(xz, xz))), xz.y));
}

float2 TerrainGridPosition(int2 sample, float2 fraction, uint lod)
{
    const uint step = 1u << lod;
    const uint2 cells = uint2(_terrainCellsX, _terrainCellsZ);
    const uint2 lastRegular = cells / step;
    const uint2 remainder = cells % step;
    const float2 position = float2(sample / int(step)) + (float2(sample % int(step)) + fraction) / float(step);
    const float2 edge = float2(lastRegular) + (float2(sample - int2(lastRegular * step)) + fraction) / float2(max(remainder, 1u));
    return float2(sample.x >= int(lastRegular.x * step) && remainder.x > 0u ? edge.x : position.x, sample.y >= int(lastRegular.y * step) && remainder.y > 0u ? edge.y : position.y);
}

// x는 0~1로 정규화된 높이를 반환합니다. yz는 각각 normal의 x,z를 반환합니다.
float3 SampleTerrainStreamRelative(float2 relativeXZ)
{
    const float2 relativeSample = relativeXZ / _terrainSampleSpacing;
    const int2 whole = int2(floor(relativeSample));
    const int2 unclamped = int2(_terrainAnchorX, _terrainAnchorZ) + whole;
    const int2 sample = clamp(unclamped, 0, int2(_terrainCellsX, _terrainCellsZ));
    const float2 fraction = float2(unclamped.x >= 0 && unclamped.x < int(_terrainCellsX) ? frac(relativeSample.x) : 0.0f, unclamped.y >= 0 && unclamped.y < int(_terrainCellsZ) ? frac(relativeSample.y) : 0.0f);
    const float2 tailPosition = TerrainGridPosition(sample, fraction, _terrainTailLod);
    const float2 tailUV = (tailPosition + 0.5f) / float2(_terrainTailWidth, _terrainTailHeightPixels);
    float3 result = float3(getTexture(_terrainTailHeight).SampleLevel(bilinearRepeatSampler, tailUV, 0.0f).x, getTexture(_terrainTailNormal).SampleLevel(bilinearRepeatSampler, tailUV, 0.0f).xy);
    const float distanceToCamera = length(relativeXZ - float2(_terrainCameraX, _terrainCameraZ));
    const float desiredLod = clamp(log2(max(distanceToCamera / _terrainDetailRadius, 1.0f)), 0.0f, float(_terrainTailLod));
    const uint lower = uint(desiredLod);
    for (int level = int(_terrainTailLod) - 1; level >= int(lower); --level)
    {
        const uint lod = uint(level);
        const uint tileCells = _terrainChunkCells << lod;
        const uint2 tileCount = uint2((_terrainCellsX - 1u) / tileCells + 1u, (_terrainCellsZ - 1u) / tileCells + 1u);
        const uint2 tile = min(uint2(sample) / tileCells, tileCount - 1u);
        const float4 page = SampleTerrainPage(tile, lod);
        if (page.x == 0.0f)
            continue;

        const float2 local = TerrainGridPosition(sample, fraction, lod) - float2(tile * _terrainChunkCells);
        float fade = page.y;
        for (int z = -1; z <= 1; ++z)
        {
            for (int x = -1; x <= 1; ++x)
            {
                const float dx = x < 0 ? local.x : (x > 0 ? float(_terrainChunkCells) - local.x : 0.0f);
                const float dz = z < 0 ? local.y : (z > 0 ? float(_terrainChunkCells) - local.y : 0.0f);
                if ((x == 0 && z == 0) == false && max(dx, dz) < 2.0f)
                    fade = min(fade, lerp(TerrainPageFade(int2(tile) + int2(x, z), lod), 1.0f, smoothstep(0.0f, 2.0f, max(dx, dz))));
            }
        }

        const uint slot = uint(page.x) - 1u;
        const uint2 offset = uint2(slot % _terrainCacheColumns, slot / _terrainCacheColumns) * _terrainCacheSide;
        const float2 uv = (float2(offset) + local + 0.5f) / float2(_terrainCacheColumns * _terrainCacheSide, _terrainCacheRows * _terrainCacheSide);
        const float3 fine = float3(getTexture(_terrainHeightCache).SampleLevel(bilinearRepeatSampler, uv, 0.0f).x, getTexture(_terrainNormalCache).SampleLevel(bilinearRepeatSampler, uv, 0.0f).xy);
        const float lodWeight = lod == lower ? 1.0f - frac(desiredLod) : 1.0f;
        result = lerp(result, fine, fade * lodWeight);
    }

    return result;
}

float3 SampleTerrainStream(float2 worldXZ)
{
    return SampleTerrainStreamRelative(worldXZ - TerrainAnchorWorldXZ());
}

// relativeXZ위치의 높이를 샘플링 하여 실제 World에서의 높이를 반환합니다.
float SampleTerrainHeightRelative(float2 relativeXZ)
{
    if (_terrainStreamEnabled)
        return _terrainHeightOffset + _terrainHeightScale * SampleTerrainStreamRelative(relativeXZ).x;

    Texture2D<float4> heightTexture = getTexture(_terrainHeightTexture);
    uint2 textureSize;
    heightTexture.GetDimensions(textureSize.x, textureSize.y);
    const float2 texel = clamp(relativeXZ / _terrainSampleSpacing + float2(_terrainAnchorX, _terrainAnchorZ), 0.0f, float2(textureSize) - 1.0f);
    const float2 uv = (texel + 0.5f) / float2(textureSize);
    return _terrainHeightOffset + _terrainHeightScale * heightTexture.SampleLevel(bilinearRepeatSampler, uv, 0.0f).x;
}

float SampleTerrainHeightAndSlopeRelative(float2 relativeXZ, out float2 slope)
{
    if (_terrainStreamEnabled)
    {
        const float3 surface = SampleTerrainStreamRelative(relativeXZ);
        const float normalY = sqrt(max(1.0f - dot(surface.yz, surface.yz), 1e-6f));
        slope = -surface.yz / normalY;
        return _terrainHeightOffset + _terrainHeightScale * surface.x;
    }

    const float height = SampleTerrainHeightRelative(relativeXZ);
    slope = (float2(SampleTerrainHeightRelative(relativeXZ + float2(_terrainSampleSpacing, 0.0f)),
                    SampleTerrainHeightRelative(relativeXZ + float2(0.0f, _terrainSampleSpacing))) - height) / _terrainSampleSpacing;
    return height;
}

float4 SampleTerrainMap(TextureParameter textureIndex, float2 mapUV)
{
    Texture2D<float4> terrainMap = getTexture(textureIndex);
    uint width, height;
    terrainMap.GetDimensions(width, height);
    const float2 halfTexel = 0.5f / float2(width, height);
    const float2 uv = clamp(mapUV, halfTexel, 1.0f - halfTexel);
    return terrainMap.SampleLevel(bilinearRepeatSampler, uv, 0.0f);
}

#endif
