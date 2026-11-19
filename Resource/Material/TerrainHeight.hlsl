#ifndef __DEFINE_TERRAIN_HEIGHT__HLSL__
#define __DEFINE_TERRAIN_HEIGHT__HLSL__

#include "CommonTexture.hlsl"
#include "Assert.hlsl"
#include "TerrainStreamingLimits.h"
#include "TerrainPageTable.h"

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
    const int2 minimumSample = int2(_terrainMinimumSampleX, _terrainMinimumSampleZ);
    if (any(sample < minimumSample) || any(sample > minimumSample + int2(_terrainCellCountX, _terrainCellCountZ)))
    {
        return false;
    }
    uint2 section;
    return ResolveTerrainSection(uint2(sample - minimumSample), frac(relativeSample), section);
}

float4 LoadTerrainPageEntry(uint2 sectionLocalTileIndex, uint lod, uint sectionSlot)
{
    GPUASSERT(lod < _terrainOverviewLod && sectionSlot < TERRAIN_SECTION_SLOT_COUNT && _terrainPageTableWidth > 0u);
    const uint sectionLodTileCountPerSide = _terrainPageTableWidth >> lod;
    GPUASSERT(all(sectionLocalTileIndex < sectionLodTileCountPerSide));
    const uint sectionPageTableRowCount = terrainPageTableLodRowOffset(_terrainPageTableWidth, _terrainOverviewLod);
    const uint lodPageTableRowOffset = terrainPageTableLodRowOffset(_terrainPageTableWidth, lod);
    const uint2 address = uint2(sectionLocalTileIndex.x, sectionLocalTileIndex.y + sectionSlot * sectionPageTableRowCount + lodPageTableRowOffset);
    const float4 pageEntry = getTexture(_terrainPageTableSRV).Load(int3(address, 0));
    GPUASSERT(pageEntry.x == 0.0f || all(uint2(pageEntry.zw) == sectionLocalTileIndex));
    return pageEntry;
}

float TerrainTileFade(int2 tile, uint lod, uint2 section)
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
    return LoadTerrainPageEntry(neighborTile, lod, TerrainSectionSlot(neighborSection)).y;
}

float3 DecodeTerrainNormal(float2 xz)
{
    return normalize(float3(xz.x, sqrt(saturate(1.0f - dot(xz, xz))), xz.y));
}

float2 TerrainGridPosition(int2 sample, float2 fraction, uint lod, uint2 cells)
{
    const uint step = 1u << lod;
    GPUASSERT(all(cells % step == 0u) && all(sample >= 0) && all(sample <= int2(cells)));
    return float2(sample / int(step)) + (float2(sample % int(step)) + fraction) / float(step);
}

float3 SampleTerrainStreamRelative(float2 relativeXZ)
{
    const float2 relativeSample = relativeXZ / _terrainSampleWorldSpacing;
    const int2 whole = int2(floor(relativeSample));
    const int2 unclamped = int2(_terrainAnchorSampleX, _terrainAnchorSampleZ) + whole;
    const int2 minimumSample = int2(_terrainMinimumSampleX, _terrainMinimumSampleZ);
    const int2 maximumSample = minimumSample + int2(_terrainCellCountX, _terrainCellCountZ);
    const int2 sample = clamp(unclamped, minimumSample, maximumSample) - minimumSample;
    const float2 fraction = float2(unclamped.x >= minimumSample.x && unclamped.x < maximumSample.x ? frac(relativeSample.x) : 0.0f, unclamped.y >= minimumSample.y && unclamped.y < maximumSample.y ? frac(relativeSample.y) : 0.0f);
    uint2 section;
    if (ResolveTerrainSection(uint2(sample), fraction, section) == false)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }
    const uint sectionSlot = TerrainSectionSlot(section);
    const TerrainSectionConstants metadata = _terrainSections[sectionSlot];
    const int2 localSample = sample - int2(section * _terrainSectionCellCountPerSide);
    const uint2 cells = uint2(metadata._terrainSectionCellCountX, metadata._terrainSectionCellCountZ);
    GPUASSERT(all(cells == _terrainSectionCellCountPerSide) && metadata._terrainSectionOverviewSampleCountX == _terrainSectionOverviewSampleCountPerSide && metadata._terrainSectionOverviewSampleCountZ == _terrainSectionOverviewSampleCountPerSide);
    const float2 overviewPosition = TerrainGridPosition(localSample, fraction, _terrainOverviewLod, cells);
    const float2 overviewOffset = float2(section.x % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE, section.y % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE) * _terrainSectionOverviewSampleCountPerSide;
    const float2 overviewUV = (overviewOffset + overviewPosition + 0.5f) / float2(_terrainOverviewSampleCountX, _terrainOverviewSampleCountZ);
    float3 result = float3(getTexture(_terrainOverviewHeightSRV).SampleLevel(bilinearRepeatSampler, overviewUV, 0.0f).x, getTexture(_terrainOverviewNormalSRV).SampleLevel(bilinearRepeatSampler, overviewUV, 0.0f).xy);
    const float distanceToCamera = length(relativeXZ - float2(_terrainCameraAnchorRelativeWorldX, _terrainCameraAnchorRelativeWorldZ));
    const float desiredLod = clamp(log2(max(distanceToCamera / _terrainLod0WorldRadius, 1.0f)), 0.0f, float(_terrainOverviewLod));
    const uint lower = uint(desiredLod);
    for (int level = int(_terrainOverviewLod) - 1; level >= int(lower); --level)
    {
        const uint lod = uint(level);
        const uint tileCells = _terrainChunkCellCountPerSide << lod;
        GPUASSERT(all(cells % tileCells == 0u));
        const uint2 tileCount = cells / tileCells;
        const uint2 tile = min(uint2(localSample) / tileCells, tileCount - 1u);
        const float4 pageEntry = LoadTerrainPageEntry(tile, lod, sectionSlot);
        if (pageEntry.x == 0.0f)
        {
            continue;
        }
        const float2 local = TerrainGridPosition(localSample, fraction, lod, cells) - float2(tile * _terrainChunkCellCountPerSide);
        float fade = pageEntry.y;
        for (int z = -1; z <= 1; ++z)
        {
            for (int x = -1; x <= 1; ++x)
            {
                const float dx = x < 0 ? local.x : (x > 0 ? float(_terrainChunkCellCountPerSide) - local.x : 0.0f);
                const float dz = z < 0 ? local.y : (z > 0 ? float(_terrainChunkCellCountPerSide) - local.y : 0.0f);
                if ((x == 0 && z == 0) == false && max(dx, dz) < 2.0f)
                {
                    fade = min(fade, lerp(TerrainTileFade(int2(tile) + int2(x, z), lod, section), 1.0f, smoothstep(0.0f, 2.0f, max(dx, dz))));
                }
            }
        }
        const uint cacheSlotIndex = uint(pageEntry.x) - 1u;
        GPUASSERT(cacheSlotIndex < _terrainCacheSlotColumnCount * _terrainCacheSlotRowCount && pageEntry.y >= 0.0f && pageEntry.y <= 1.0f && all(local >= 0.0f) && all(local <= float(_terrainChunkCellCountPerSide)));
        const uint2 offset = uint2(cacheSlotIndex % _terrainCacheSlotColumnCount, cacheSlotIndex / _terrainCacheSlotColumnCount) * _terrainCacheSlotTexelCountPerSide;
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
