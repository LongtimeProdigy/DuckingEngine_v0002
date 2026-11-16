#ifndef TERRAIN_HEIGHT_HLSL
#define TERRAIN_HEIGHT_HLSL

#include "CommonTexture.hlsl"

cbuffer TerrainStreamingConstants : register(b3)
{
    uint _terrainStreamingEnabled;
    uint _terrainHeightCacheSRV;
    uint _terrainNormalCacheSRV;
    uint _terrainPageTableSRV;
    uint _terrainOverviewHeightSRV;
    uint _terrainOverviewNormalSRV;
    uint _terrainCacheSlotColumnCount;
    uint _terrainCacheSlotRowCount;
    uint _terrainCacheSlotTexelCountPerSide;
    float _terrainSampleWorldSpacing;
    float _terrainBaseWorldHeight;
    float _terrainWorldHeightRange;
    uint _terrainCellCountX;
    uint _terrainCellCountZ;
    uint _terrainChunkCellCountPerSide;
    uint _terrainLod0TileCountX;
    uint _terrainLod0TileCountZ;
    uint _terrainOverviewLod;
    uint _terrainOverviewSampleCountX;
    uint _terrainOverviewSampleCountZ;
    uint _terrainPageTableEntryCountPerSide;
    float _terrainCameraAnchorRelativeWorldX;
    float _terrainCameraAnchorRelativeWorldZ;
    int _terrainAnchorSampleX;
    int _terrainAnchorSampleZ;
    float _terrainCameraWorldX;
    float _terrainCameraWorldZ;
    float _terrainLod0WorldRadius;
    uint _terrainFullHeightTextureSRV;
    float _terrainOriginWorldX;
    float _terrainOriginWorldZ;
}

float2 TerrainAnchorWorldXZ()
{
    return float2(_terrainOriginWorldX, _terrainOriginWorldZ) + float2(_terrainAnchorSampleX, _terrainAnchorSampleZ) * _terrainSampleWorldSpacing;
}

float4 SampleTerrainPage(uint2 tile, uint lod)
{
    const uint2 address = uint2(tile.x % _terrainPageTableEntryCountPerSide, tile.y % _terrainPageTableEntryCountPerSide + lod * _terrainPageTableEntryCountPerSide);
    const float4 page = getTexture(_terrainPageTableSRV).Load(int3(address, 0));
    if (any(uint2(page.zw) != tile))
    {
        return 0.0f;
    }
    return page;
}

float TerrainPageFade(int2 tile, uint lod)
{
    const uint cells = _terrainChunkCellCountPerSide << lod;
    const int2 count = int2((_terrainCellCountX - 1u) / cells + 1u, (_terrainCellCountZ - 1u) / cells + 1u);
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
    const uint2 cells = uint2(_terrainCellCountX, _terrainCellCountZ);
    const uint2 lastRegular = cells / step;
    const uint2 remainder = cells % step;
    const float2 position = float2(sample / int(step)) + (float2(sample % int(step)) + fraction) / float(step);
    const float2 edge = float2(lastRegular) + (float2(sample - int2(lastRegular * step)) + fraction) / float2(max(remainder, 1u));
    return float2(sample.x >= int(lastRegular.x * step) && remainder.x > 0u ? edge.x : position.x, sample.y >= int(lastRegular.y * step) && remainder.y > 0u ? edge.y : position.y);
}

// x는 0~1로 정규화된 높이를 반환합니다. yz는 각각 normal의 x,z를 반환합니다.
float3 SampleTerrainStreamRelative(float2 relativeXZ)
{
    const float2 relativeSample = relativeXZ / _terrainSampleWorldSpacing;
    const int2 whole = int2(floor(relativeSample));
    const int2 unclamped = int2(_terrainAnchorSampleX, _terrainAnchorSampleZ) + whole;
    const int2 sample = clamp(unclamped, 0, int2(_terrainCellCountX, _terrainCellCountZ));
    const float2 fraction = float2(unclamped.x >= 0 && unclamped.x < int(_terrainCellCountX) ? frac(relativeSample.x) : 0.0f, unclamped.y >= 0 && unclamped.y < int(_terrainCellCountZ) ? frac(relativeSample.y) : 0.0f);
    const float2 tailPosition = TerrainGridPosition(sample, fraction, _terrainOverviewLod);
    const float2 tailUV = (tailPosition + 0.5f) / float2(_terrainOverviewSampleCountX, _terrainOverviewSampleCountZ);
    float3 result = float3(getTexture(_terrainOverviewHeightSRV).SampleLevel(bilinearRepeatSampler, tailUV, 0.0f).x, getTexture(_terrainOverviewNormalSRV).SampleLevel(bilinearRepeatSampler, tailUV, 0.0f).xy);
    const float distanceToCamera = length(relativeXZ - float2(_terrainCameraAnchorRelativeWorldX, _terrainCameraAnchorRelativeWorldZ));
    const float desiredLod = clamp(log2(max(distanceToCamera / _terrainLod0WorldRadius, 1.0f)), 0.0f, float(_terrainOverviewLod));
    const uint lower = uint(desiredLod);
    for (int level = int(_terrainOverviewLod) - 1; level >= int(lower); --level)
    {
        const uint lod = uint(level);
        const uint tileCells = _terrainChunkCellCountPerSide << lod;
        const uint2 tileCount = uint2((_terrainCellCountX - 1u) / tileCells + 1u, (_terrainCellCountZ - 1u) / tileCells + 1u);
        const uint2 tile = min(uint2(sample) / tileCells, tileCount - 1u);
        const float4 page = SampleTerrainPage(tile, lod);
        if (page.x == 0.0f)
            continue;

        const float2 local = TerrainGridPosition(sample, fraction, lod) - float2(tile * _terrainChunkCellCountPerSide);
        float fade = page.y;
        for (int z = -1; z <= 1; ++z)
        {
            for (int x = -1; x <= 1; ++x)
            {
                const float dx = x < 0 ? local.x : (x > 0 ? float(_terrainChunkCellCountPerSide) - local.x : 0.0f);
                const float dz = z < 0 ? local.y : (z > 0 ? float(_terrainChunkCellCountPerSide) - local.y : 0.0f);
                if ((x == 0 && z == 0) == false && max(dx, dz) < 2.0f)
                    fade = min(fade, lerp(TerrainPageFade(int2(tile) + int2(x, z), lod), 1.0f, smoothstep(0.0f, 2.0f, max(dx, dz))));
            }
        }

        const uint slot = uint(page.x) - 1u;
        const uint2 offset = uint2(slot % _terrainCacheSlotColumnCount, slot / _terrainCacheSlotColumnCount) * _terrainCacheSlotTexelCountPerSide;
        const float2 uv = (float2(offset) + local + 0.5f) / float2(_terrainCacheSlotColumnCount * _terrainCacheSlotTexelCountPerSide, _terrainCacheSlotRowCount * _terrainCacheSlotTexelCountPerSide);
        const float3 fine = float3(getTexture(_terrainHeightCacheSRV).SampleLevel(bilinearRepeatSampler, uv, 0.0f).x, getTexture(_terrainNormalCacheSRV).SampleLevel(bilinearRepeatSampler, uv, 0.0f).xy);
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
    if (_terrainStreamingEnabled)
        return _terrainBaseWorldHeight + _terrainWorldHeightRange * SampleTerrainStreamRelative(relativeXZ).x;

    Texture2D<float4> heightTexture = getTexture(_terrainFullHeightTextureSRV);
    uint2 textureSize;
    heightTexture.GetDimensions(textureSize.x, textureSize.y);
    const float2 texel = clamp(relativeXZ / _terrainSampleWorldSpacing + float2(_terrainAnchorSampleX, _terrainAnchorSampleZ), 0.0f, float2(textureSize) - 1.0f);
    const float2 uv = (texel + 0.5f) / float2(textureSize);
    return _terrainBaseWorldHeight + _terrainWorldHeightRange * heightTexture.SampleLevel(bilinearRepeatSampler, uv, 0.0f).x;
}

float SampleTerrainHeightAndSlopeRelative(float2 relativeXZ, out float2 slope)
{
    if (_terrainStreamingEnabled)
    {
        const float3 surface = SampleTerrainStreamRelative(relativeXZ);
        const float normalY = sqrt(max(1.0f - dot(surface.yz, surface.yz), 1e-6f));
        slope = -surface.yz / normalY;
        return _terrainBaseWorldHeight + _terrainWorldHeightRange * surface.x;
    }

    const float height = SampleTerrainHeightRelative(relativeXZ);
    slope = (float2(SampleTerrainHeightRelative(relativeXZ + float2(_terrainSampleWorldSpacing, 0.0f)),
                    SampleTerrainHeightRelative(relativeXZ + float2(0.0f, _terrainSampleWorldSpacing))) - height) / _terrainSampleWorldSpacing;
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
