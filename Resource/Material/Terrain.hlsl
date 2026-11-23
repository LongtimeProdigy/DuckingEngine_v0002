#ifndef DK_TERRAIN_HLSL
#define DK_TERRAIN_HLSL

#include "CommonTexture.hlsl"
#include "Assert.hlsl"
#include "TerrainShared.h"

#define DK_SHADER_TERRAIN_STREAMING
#include "ShaderStructures.h"
#undef DK_SHADER_TERRAIN_STREAMING

float2 TerrainCameraAnchorWorldXZ()
{
    return float2(_terrainOriginWorldX, _terrainOriginWorldZ) + float2(_terrainCameraAnchorSampleX, _terrainCameraAnchorSampleZ) * _terrainSampleWorldSpacing;
}

uint TerrainSectionSlotIndex(uint2 sectionIndex)
{
    return (sectionIndex.y % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE) * TERRAIN_SECTION_SLOT_COUNT_PER_SIDE + sectionIndex.x % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE;
}

bool TerrainSectionAvailable(uint2 sectionIndex)
{
    const TerrainSectionConstants metadata = _terrainSections[TerrainSectionSlotIndex(sectionIndex)];
    return metadata._terrainSectionResident != 0u && all(uint2(metadata._terrainSectionIndexX, metadata._terrainSectionIndexZ) == sectionIndex);
}

bool ResolveTerrainSection(uint2 sample, float2 fraction, out uint2 sectionIndex)
{
    const uint2 sectionCount = uint2(_terrainSectionCountX, _terrainSectionCountZ);
    sectionIndex = min(sample / _terrainSectionCellCountPerSide, sectionCount - 1u);
    if (TerrainSectionAvailable(sectionIndex))
    {
        return true;
    }
    const bool left = sectionIndex.x > 0u && sample.x == sectionIndex.x * _terrainSectionCellCountPerSide && fraction.x == 0.0f;
    const bool top = sectionIndex.y > 0u && sample.y == sectionIndex.y * _terrainSectionCellCountPerSide && fraction.y == 0.0f;
    if (left && TerrainSectionAvailable(sectionIndex - uint2(1u, 0u)))
    {
        sectionIndex.x -= 1u;
        return true;
    }
    if (top && TerrainSectionAvailable(sectionIndex - uint2(0u, 1u)))
    {
        sectionIndex.y -= 1u;
        return true;
    }
    if (left && top && TerrainSectionAvailable(sectionIndex - 1u))
    {
        sectionIndex -= 1u;
        return true;
    }
    return false;
}

bool TerrainAvailableRelative(float2 relativeXZ)
{
    const float2 relativeSample = relativeXZ / _terrainSampleWorldSpacing;
    const int2 whole = int2(floor(relativeSample));
    const int2 sample = int2(_terrainCameraAnchorSampleX, _terrainCameraAnchorSampleZ) + whole;
    const int2 minimumSample = int2(_terrainMinimumSampleX, _terrainMinimumSampleZ);
    if (any(sample < minimumSample) || any(sample > minimumSample + int2(_terrainCellCountX, _terrainCellCountZ)))
    {
        return false;
    }
    uint2 sectionIndex;
    return ResolveTerrainSection(uint2(sample - minimumSample), frac(relativeSample), sectionIndex);
}

float4 LoadTerrainPageEntry(uint2 sectionLocalTileIndex, uint lod, uint sectionSlotIndex)
{
    GPUASSERT(lod < _terrainOverviewLod && sectionSlotIndex < TERRAIN_SECTION_SLOT_COUNT && _terrainPageTableWidth > 0u);
    const uint sectionLodTileCountPerSide = _terrainPageTableWidth >> lod;
    GPUASSERT(all(sectionLocalTileIndex < sectionLodTileCountPerSide));
    const uint sectionPageTableRowCount = terrainPageTableLodRowOffset(_terrainPageTableWidth, _terrainOverviewLod);
    const uint lodPageTableRowOffset = terrainPageTableLodRowOffset(_terrainPageTableWidth, lod);
    const uint2 pageTableTexel = uint2(sectionLocalTileIndex.x, sectionLocalTileIndex.y + sectionSlotIndex * sectionPageTableRowCount + lodPageTableRowOffset);
    const float4 pageEntry = getTexture(_terrainPageTableSRV).Load(int3(pageTableTexel, 0));
    GPUASSERT(pageEntry.x == 0.0f || all(uint2(pageEntry.zw) == sectionLocalTileIndex));
    return pageEntry;
}

float TerrainTileFade(int2 sectionLocalTileIndex, uint lod, uint2 sectionIndex)
{
    const int tileLod0CellCountPerSide = int(_terrainChunkCellCountPerSide << lod);
    const int2 sample = int2(sectionIndex * _terrainSectionCellCountPerSide) + sectionLocalTileIndex * tileLod0CellCountPerSide;
    if (any(sample < 0) || any(sample >= int2(_terrainCellCountX, _terrainCellCountZ)))
    {
        return 1.0f;
    }
    const uint2 neighborSectionIndex = uint2(sample) / _terrainSectionCellCountPerSide;
    if (TerrainSectionAvailable(neighborSectionIndex) == false)
    {
        return 0.0f;
    }
    const uint2 neighborSectionLocalTileIndex = (uint2(sample) - neighborSectionIndex * _terrainSectionCellCountPerSide) / uint(tileLod0CellCountPerSide);
    return LoadTerrainPageEntry(neighborSectionLocalTileIndex, lod, TerrainSectionSlotIndex(neighborSectionIndex)).y;
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
    const int2 unclamped = int2(_terrainCameraAnchorSampleX, _terrainCameraAnchorSampleZ) + whole;
    const int2 minimumSample = int2(_terrainMinimumSampleX, _terrainMinimumSampleZ);
    const int2 maximumSample = minimumSample + int2(_terrainCellCountX, _terrainCellCountZ);
    const int2 sample = clamp(unclamped, minimumSample, maximumSample) - minimumSample;
    const float2 fraction = float2(unclamped.x >= minimumSample.x && unclamped.x < maximumSample.x ? frac(relativeSample.x) : 0.0f, unclamped.y >= minimumSample.y && unclamped.y < maximumSample.y ? frac(relativeSample.y) : 0.0f);
    uint2 sectionIndex;
    if (ResolveTerrainSection(uint2(sample), fraction, sectionIndex) == false)
        return float3(0.0f, 0.0f, 0.0f);
        
    const uint sectionSlotIndex = TerrainSectionSlotIndex(sectionIndex);
    const int2 sectionLocalSample = sample - int2(sectionIndex * _terrainSectionCellCountPerSide);
    const uint2 cells = uint2(_terrainSectionCellCountPerSide, _terrainSectionCellCountPerSide);
    const float2 overviewLodSamplePosition = TerrainGridPosition(sectionLocalSample, fraction, _terrainOverviewLod, cells);
    const float2 overviewSlotTexelOffset = float2(sectionIndex.x % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE, sectionIndex.y % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE) * _terrainSectionOverviewSampleCountPerSide;
    const float2 overviewUV = (overviewSlotTexelOffset + overviewLodSamplePosition + 0.5f) / float2(_terrainOverviewAtlasTexelCountX, _terrainOverviewAtlasTexelCountY);
    float3 result = float3(getTexture(_terrainOverviewHeightAtlasSRV).SampleLevel(bilinearRepeatSampler, overviewUV, 0.0f).x, getTexture(_terrainOverviewNormalAtlasSRV).SampleLevel(bilinearRepeatSampler, overviewUV, 0.0f).xy);
    const float distanceToCamera = length(relativeXZ - float2(_terrainCameraLocalX, _terrainCameraLocalZ));
    const float desiredLod = clamp(log2(max(distanceToCamera / _terrainLod0WorldRadius, 1.0f)), 0.0f, float(_terrainOverviewLod));
    const uint lower = uint(desiredLod);
    for (int level = int(_terrainOverviewLod) - 1; level >= int(lower); --level)
    {
        const uint lod = uint(level);
        const uint tileLod0CellCountPerSide = _terrainChunkCellCountPerSide << lod;
        GPUASSERT(all(cells % tileLod0CellCountPerSide == 0u));
        const uint2 tileCount = cells / tileLod0CellCountPerSide;
        const uint2 sectionLocalTileIndex = min(uint2(sectionLocalSample) / tileLod0CellCountPerSide, tileCount - 1u);
        const float4 pageEntry = LoadTerrainPageEntry(sectionLocalTileIndex, lod, sectionSlotIndex);
        if (pageEntry.x == 0.0f)
        {
            continue;
        }
        const float2 tileLocalLodSample = TerrainGridPosition(sectionLocalSample, fraction, lod, cells) - float2(sectionLocalTileIndex * _terrainChunkCellCountPerSide);
        float fade = pageEntry.y;
        for (int z = -1; z <= 1; ++z)
        {
            for (int x = -1; x <= 1; ++x)
            {
                const float dx = x < 0 ? tileLocalLodSample.x : (x > 0 ? float(_terrainChunkCellCountPerSide) - tileLocalLodSample.x : 0.0f);
                const float dz = z < 0 ? tileLocalLodSample.y : (z > 0 ? float(_terrainChunkCellCountPerSide) - tileLocalLodSample.y : 0.0f);
                if ((x == 0 && z == 0) == false && max(dx, dz) < 2.0f)
                {
                    fade = min(fade, lerp(TerrainTileFade(int2(sectionLocalTileIndex) + int2(x, z), lod, sectionIndex), 1.0f, smoothstep(0.0f, 2.0f, max(dx, dz))));
                }
            }
        }
        const uint cacheSlotIndex = uint(pageEntry.x) - 1u;
        GPUASSERT(cacheSlotIndex < _terrainCacheSlotColumnCount * _terrainCacheSlotRowCount && pageEntry.y >= 0.0f && pageEntry.y <= 1.0f && all(tileLocalLodSample >= 0.0f) && all(tileLocalLodSample <= float(_terrainChunkCellCountPerSide)));
        const uint2 cacheSlotTexelOffset = uint2(cacheSlotIndex % _terrainCacheSlotColumnCount, cacheSlotIndex / _terrainCacheSlotColumnCount) * _terrainCacheSlotTexelCountPerSide;
        const float2 uv = (float2(cacheSlotTexelOffset) + tileLocalLodSample + 0.5f) / float2(_terrainCacheSlotColumnCount * _terrainCacheSlotTexelCountPerSide, _terrainCacheSlotRowCount * _terrainCacheSlotTexelCountPerSide);
        const float3 fine = float3(getTexture(_terrainHeightCacheSRV).SampleLevel(bilinearRepeatSampler, uv, 0.0f).x, getTexture(_terrainNormalCacheSRV).SampleLevel(bilinearRepeatSampler, uv, 0.0f).xy);
        const float lodWeight = lod == lower ? 1.0f - frac(desiredLod) : 1.0f;
        result = lerp(result, fine, fade * lodWeight);
    }
    return result;
}

float3 SampleTerrainStream(float2 worldXZ)
{
    return SampleTerrainStreamRelative(worldXZ - TerrainCameraAnchorWorldXZ());
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
