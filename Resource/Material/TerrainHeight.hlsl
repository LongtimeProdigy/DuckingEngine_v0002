#ifndef __DEFINE_TERRAIN_HEIGHT__HLSL__
#define __DEFINE_TERRAIN_HEIGHT__HLSL__

#include "CommonTexture.hlsl"
#include "TerrainStreamingLimits.h"

#define DK_SHADER_TERRAIN_STREAMING
#include "ShaderStructures.h"
#undef DK_SHADER_TERRAIN_STREAMING

float2 TerrainAnchorWorldXZ()
{
    return float2(_terrainOriginWorldX, _terrainOriginWorldZ) + float2(_terrainAnchorSampleX, _terrainAnchorSampleZ) * _terrainSampleWorldSpacing;
}

uint TerrainSectionSlot(uint2 section)
{
    return (section.y % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE) * TERRAIN_SECTION_SLOT_COUNT_PER_SIDE + section.x % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE;
}

bool TerrainSectionAvailable(uint2 section)
{
    const TerrainSectionConstants metadata = _terrainSections[TerrainSectionSlot(section)];
    return metadata._terrainSectionResident != 0u && all(uint2(metadata._terrainSectionX, metadata._terrainSectionZ) == section);
}

bool ResolveTerrainSection(uint2 sample, float2 fraction, out uint2 section)
{
    const uint2 sectionCount = uint2(_terrainSectionCountX, _terrainSectionCountZ);
    section = min(sample / _terrainSectionCellCountPerSide, sectionCount - 1u);
    if (TerrainSectionAvailable(section))
    {
        return true;
    }
    const bool left = section.x > 0u && sample.x == section.x * _terrainSectionCellCountPerSide && fraction.x == 0.0f;
    const bool top = section.y > 0u && sample.y == section.y * _terrainSectionCellCountPerSide && fraction.y == 0.0f;
    if (left && TerrainSectionAvailable(section - uint2(1u, 0u)))
    {
        section.x -= 1u;
        return true;
    }
    if (top && TerrainSectionAvailable(section - uint2(0u, 1u)))
    {
        section.y -= 1u;
        return true;
    }
    if (left && top && TerrainSectionAvailable(section - 1u))
    {
        section -= 1u;
        return true;
    }
    return false;
}

bool TerrainAvailableRelative(float2 relativeXZ)
{
    const float2 relativeSample = relativeXZ / _terrainSampleWorldSpacing;
    const int2 whole = int2(floor(relativeSample));
    const int2 sample = int2(_terrainAnchorSampleX, _terrainAnchorSampleZ) + whole;
    if (any(sample < 0) || any(sample > int2(_terrainCellCountX, _terrainCellCountZ)))
    {
        return false;
    }
    uint2 section;
    return ResolveTerrainSection(uint2(sample), frac(relativeSample), section);
}

float4 SampleTerrainPage(uint2 tile, uint lod, uint sectionSlot)
{
    const uint2 address = uint2(tile.x % _terrainPageTableEntryCountPerSide, tile.y % _terrainPageTableEntryCountPerSide + (sectionSlot * _terrainOverviewLod + lod) * _terrainPageTableEntryCountPerSide);
    const float4 page = getTexture(_terrainPageTableSRV).Load(int3(address, 0));
    if (any(uint2(page.zw) != tile))
    {
        return 0.0f;
    }
    return page;
}

float TerrainPageFade(int2 tile, uint lod, uint2 section)
{
    const int tileCells = int(_terrainChunkCellCountPerSide << lod);
    const int2 sample = int2(section * _terrainSectionCellCountPerSide) + tile * tileCells;
    if (any(sample < 0) || any(sample >= int2(_terrainCellCountX, _terrainCellCountZ)))
    {
        return 1.0f;
    }
    const uint2 neighborSection = uint2(sample) / _terrainSectionCellCountPerSide;
    if (TerrainSectionAvailable(neighborSection) == false)
    {
        return 0.0f;
    }
    const uint2 neighborTile = (uint2(sample) - neighborSection * _terrainSectionCellCountPerSide) / uint(tileCells);
    return SampleTerrainPage(neighborTile, lod, TerrainSectionSlot(neighborSection)).y;
}

float3 DecodeTerrainNormal(float2 xz)
{
    return normalize(float3(xz.x, sqrt(saturate(1.0f - dot(xz, xz))), xz.y));
}

float2 TerrainGridPosition(int2 sample, float2 fraction, uint lod, uint2 cells)
{
    const uint step = 1u << lod;
    const uint2 lastRegular = cells / step;
    const uint2 remainder = cells % step;
    const float2 position = float2(sample / int(step)) + (float2(sample % int(step)) + fraction) / float(step);
    const float2 edge = float2(lastRegular) + (float2(sample - int2(lastRegular * step)) + fraction) / float2(max(remainder, 1u));
    return float2(sample.x >= int(lastRegular.x * step) && remainder.x > 0u ? edge.x : position.x, sample.y >= int(lastRegular.y * step) && remainder.y > 0u ? edge.y : position.y);
}

float3 SampleTerrainStreamRelative(float2 relativeXZ)
{
    const float2 relativeSample = relativeXZ / _terrainSampleWorldSpacing;
    const int2 whole = int2(floor(relativeSample));
    const int2 unclamped = int2(_terrainAnchorSampleX, _terrainAnchorSampleZ) + whole;
    const int2 sample = clamp(unclamped, 0, int2(_terrainCellCountX, _terrainCellCountZ));
    const float2 fraction = float2(unclamped.x >= 0 && unclamped.x < int(_terrainCellCountX) ? frac(relativeSample.x) : 0.0f, unclamped.y >= 0 && unclamped.y < int(_terrainCellCountZ) ? frac(relativeSample.y) : 0.0f);
    uint2 section;
    if (ResolveTerrainSection(uint2(sample), fraction, section) == false)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }
    const uint sectionSlot = TerrainSectionSlot(section);
    const TerrainSectionConstants metadata = _terrainSections[sectionSlot];
    const int2 localSample = sample - int2(section * _terrainSectionCellCountPerSide);
    const uint2 cells = uint2(metadata._terrainSectionCellCountX, metadata._terrainSectionCellCountZ);
    const float2 overviewPosition = TerrainGridPosition(localSample, fraction, _terrainOverviewLod, cells);
    const float2 overviewOffset = float2(section.x % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE, section.y % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE) * _terrainOverviewSlotSampleCountPerSide;
    const float2 overviewUV = (overviewOffset + overviewPosition + 0.5f) / float2(_terrainOverviewSampleCountX, _terrainOverviewSampleCountZ);
    float3 result = float3(getTexture(_terrainOverviewHeightSRV).SampleLevel(bilinearRepeatSampler, overviewUV, 0.0f).x, getTexture(_terrainOverviewNormalSRV).SampleLevel(bilinearRepeatSampler, overviewUV, 0.0f).xy);
    const float distanceToCamera = length(relativeXZ - float2(_terrainCameraAnchorRelativeWorldX, _terrainCameraAnchorRelativeWorldZ));
    const float desiredLod = clamp(log2(max(distanceToCamera / _terrainLod0WorldRadius, 1.0f)), 0.0f, float(_terrainOverviewLod));
    const uint lower = uint(desiredLod);
    for (int level = int(_terrainOverviewLod) - 1; level >= int(lower); --level)
    {
        const uint lod = uint(level);
        const uint tileCells = _terrainChunkCellCountPerSide << lod;
        const uint2 tileCount = (cells - 1u) / tileCells + 1u;
        const uint2 tile = min(uint2(localSample) / tileCells, tileCount - 1u);
        const float4 page = SampleTerrainPage(tile, lod, sectionSlot);
        if (page.x == 0.0f)
        {
            continue;
        }
        const float2 local = TerrainGridPosition(localSample, fraction, lod, cells) - float2(tile * _terrainChunkCellCountPerSide);
        float fade = page.y;
        for (int z = -1; z <= 1; ++z)
        {
            for (int x = -1; x <= 1; ++x)
            {
                const float dx = x < 0 ? local.x : (x > 0 ? float(_terrainChunkCellCountPerSide) - local.x : 0.0f);
                const float dz = z < 0 ? local.y : (z > 0 ? float(_terrainChunkCellCountPerSide) - local.y : 0.0f);
                if ((x == 0 && z == 0) == false && max(dx, dz) < 2.0f)
                {
                    fade = min(fade, lerp(TerrainPageFade(int2(tile) + int2(x, z), lod, section), 1.0f, smoothstep(0.0f, 2.0f, max(dx, dz))));
                }
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

float SampleTerrainHeightRelative(float2 relativeXZ)
{
    return _terrainBaseWorldHeight + _terrainWorldHeightRange * SampleTerrainStreamRelative(relativeXZ).x;
}

float SampleTerrainHeightAndSlopeRelative(float2 relativeXZ, out float2 slope)
{
    const float3 surface = SampleTerrainStreamRelative(relativeXZ);
    const float normalY = sqrt(max(1.0f - dot(surface.yz, surface.yz), 1e-6f));
    slope = -surface.yz / normalY;
    return _terrainBaseWorldHeight + _terrainWorldHeightRange * surface.x;
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
