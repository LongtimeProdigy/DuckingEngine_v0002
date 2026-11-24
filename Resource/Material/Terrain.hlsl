#ifndef DK_TERRAIN_HLSL
#define DK_TERRAIN_HLSL

#include "CommonTexture.hlsl"
#include "Assert.hlsl"
#include "TerrainShared.h"

#define DK_SHADER_TERRAIN_STREAMING
#include "ShaderStructures.h"
#undef DK_SHADER_TERRAIN_STREAMING

// 카메라 XZ를 LOD0 샘플 격자에 맞춰 내림한 앵커의 월드 좌표
float2 TerrainCameraAnchorWorldXZ()
{
    return float2(_terrainOriginWorldX, _terrainOriginWorldZ) + float2(_terrainCameraAnchorSampleX, _terrainCameraAnchorSampleZ) * _terrainSampleWorldSpacing;
}

// 2D sectionIndex를 1D (Linear) sectionIndex로 반환
uint TerrainSectionSlotIndex(uint2 sectionIndex)
{
    return (sectionIndex.y % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE) * TERRAIN_SECTION_SLOT_COUNT_PER_SIDE + sectionIndex.x % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE;
}
// section의 Data가 valid한지 체크
bool TerrainSectionAvailable(uint2 sectionIndex)
{
    const TerrainSectionConstants metadata = _terrainSections[TerrainSectionSlotIndex(sectionIndex)];
    return metadata._terrainSectionResident != 0u && all(uint2(metadata._terrainSectionIndexX, metadata._terrainSectionIndexZ) == sectionIndex);
}

// sampleIndex를 sectionIndex로 변환. 만약 ~~~section이 valid하지않고 경계에 걸친 sample은 주변 section이 같은 sample을 공유할 수도 있으므로 주변도 탐색~~~
// 일단 Section로딩이 없으면 Proxy가 보이므로 당장은 옆 Section을 검사하지 않는다.
bool ResolveTerrainSection(uint2 lod0SampleIndex, float2 fraction, out uint2 sectionIndex)
{
    const uint2 sectionCount = uint2(_terrainSectionCountX, _terrainSectionCountZ);
    sectionIndex = min(lod0SampleIndex / _terrainSectionCellCountPerSide, sectionCount - 1u);
    if (TerrainSectionAvailable(sectionIndex))
        return true;

#if 0
    // Section 경계에 정확히 놓인 샘플인 경우 주변에 valid한 똑같은 section이 있을수도 있으므로 검사
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
#endif
    return false;
}

// 현재위치의 Section이 Valid한지 체크
bool TerrainAvailableRelative(float2 relativeXZ)
{
    const float2 lod0SamplePosition = relativeXZ / _terrainSampleWorldSpacing;
    const int2 lod0SampleRelativeIndex = int2(floor(lod0SamplePosition));
    const int2 lod0SampleIndex = int2(_terrainCameraAnchorSampleX, _terrainCameraAnchorSampleZ) + lod0SampleRelativeIndex;
    const int2 minimumSample = int2(_terrainMinimumSampleX, _terrainMinimumSampleZ);
    if (any(lod0SampleIndex < minimumSample) || any(lod0SampleIndex > minimumSample + int2(_terrainCellCountX, _terrainCellCountZ)))
        return false;

    uint2 sectionIndex;
    return ResolveTerrainSection(uint2(lod0SampleIndex - minimumSample), frac(lod0SamplePosition), sectionIndex);
}

// PageTable에서 Tile의 정보 조회 (x가 0이면 invalid함)
// 성분	저장 값
// x	Height 캐시 슬롯 인덱스 +1. 0이면 미로딩
// y	Detail이 나타나는 보간 가중치 fade
// z, w	Section 내부 Tile X/Z 인덱스. 검증용
float4 LoadTerrainPageEntry(uint2 sectionLocalTileIndex, uint lod, uint sectionSlotIndex)
{
    GPUASSERT(lod < _terrainOverviewLod && sectionSlotIndex < TERRAIN_SECTION_SLOT_COUNT && _terrainPageTableWidth > 0u);
    GPUASSERT(all(sectionLocalTileIndex < (_terrainPageTableWidth >> lod)));
    const uint sectionPageTableRowCount = terrainPageTableLodRowOffset(_terrainPageTableWidth, _terrainOverviewLod);
    const uint lodPageTableRowOffset = terrainPageTableLodRowOffset(_terrainPageTableWidth, lod);
    const uint2 pageTableTexel = uint2(sectionLocalTileIndex.x, sectionLocalTileIndex.y + sectionSlotIndex * sectionPageTableRowCount + lodPageTableRowOffset);
    const float4 pageEntry = getTexture(_terrainPageTableSRV).Load(int3(pageTableTexel, 0));
    GPUASSERT(pageEntry.x == 0.0f || all(uint2(pageEntry.zw) == sectionLocalTileIndex));
    return pageEntry;
}

// 이웃 Tile의 Detail 로딩 전환 정도(fade)를 조회하는 함수
float GetNeighborTerrainTileFade(int2 sectionLocalTileIndex, uint lod, uint2 sectionIndex)
{
    const int tileCellCountPerSide = int(_terrainChunkCellCountPerSide << lod);
    const int2 lod0SampleIndex = int2(sectionIndex * _terrainSectionCellCountPerSide) + sectionLocalTileIndex * tileCellCountPerSide;
    if (any(lod0SampleIndex < 0) || any(lod0SampleIndex >= int2(_terrainCellCountX, _terrainCellCountZ)))
        return 1.0f;    // 현재 Tile의 fade를 낮추지 않음

    // Section 경계에 있는 타일은 근처 타일이 다른 Section일 수 있으니 새로운 SectionIndex를 계산해야함
    const uint2 neighborSectionIndex = uint2(lod0SampleIndex) / _terrainSectionCellCountPerSide;
    if (TerrainSectionAvailable(neighborSectionIndex) == false)
        return 0.0f;    // 경계에서 현재 Detail도 상위 LOD 쪽으로 전환

    const uint2 neighborTileSectionLocalIndex = (uint2(lod0SampleIndex) - neighborSectionIndex * _terrainSectionCellCountPerSide) / uint(tileCellCountPerSide);
    return LoadTerrainPageEntry(neighborTileSectionLocalIndex, lod, TerrainSectionSlotIndex(neighborSectionIndex)).y;
}

// Section 내부의 LOD0 샘플 좌표를, 지정한 LOD의 샘플 좌표로 변환
float2 convertSampleDetailPosition(int2 lod0SampleSectionLocalIndex, float2 fraction, uint lod, uint2 cells)
{
    // LOD가 커질수록 lod0SampleSectionLocalIndex와 똑같은 위치를 읽으면 먼 거리의 sample을 읽는 것이기 떄문에 index를 2^lod만큼 나눠 같은 위치의 texel을 읽도록 index조절
    const uint step = 1u << lod;
    GPUASSERT(all(cells % step == 0u) && all(lod0SampleSectionLocalIndex >= 0) && all(lod0SampleSectionLocalIndex <= int2(cells)));
    const int2 index = lod0SampleSectionLocalIndex / int(step);
    const float2 localPosition = (float2(lod0SampleSectionLocalIndex % int(step)) + fraction) / float(step);
    return float2(index) + localPosition;
}
// 카메라 앵커 기준 상대 XZ 위치에서 지형의 높이와 노멀 정보를 읽는 함수 (정규화된 높이를 반환하므로 정확히 world 높이를 반환하는 게 아님)
// Detail Tile이 있으면 detail을, 없으면 overview를, overview도 없다면 float3::Zero를 반환. 만약 Detail이라면 각 주변 tile과의 fade를 고려
float3 SampleTerrainStreamRelative(float2 relativeXZ)
{
    const float2 lod0SamplePosition = relativeXZ / _terrainSampleWorldSpacing;
    const int2 lod0SampleRelativeIndex = int2(floor(lod0SamplePosition));
    const int2 lod0SampleUnclampedIndex = int2(_terrainCameraAnchorSampleX, _terrainCameraAnchorSampleZ) + lod0SampleRelativeIndex;
    const int2 minimumSample = int2(_terrainMinimumSampleX, _terrainMinimumSampleZ);
    const int2 maximumSample = minimumSample + int2(_terrainCellCountX, _terrainCellCountZ);
    const int2 lod0SampleIndex = clamp(lod0SampleUnclampedIndex, minimumSample, maximumSample) - minimumSample;  // heightmap 밖의 영역이면 warp하는 느낌이 나도록 clamp
    const float2 lod0Fraction = float2( // heightmap 밖의 영역이면 warp하는 느낌이 나도록 clamp
        lod0SampleUnclampedIndex.x >= minimumSample.x && lod0SampleUnclampedIndex.x < maximumSample.x ? frac(lod0SamplePosition.x) : 0.0f
        , lod0SampleUnclampedIndex.y >= minimumSample.y && lod0SampleUnclampedIndex.y < maximumSample.y ? frac(lod0SamplePosition.y) : 0.0f);
    uint2 sectionIndex;
    if (ResolveTerrainSection(uint2(lod0SampleIndex), lod0Fraction, sectionIndex) == false)
        return float3(0.0f, 0.0f, 0.0f);
        
    // overview result
    const uint sectionSlotIndex = TerrainSectionSlotIndex(sectionIndex);
    const int2 lod0SampleSectionLocalIndex = lod0SampleIndex - int2(sectionIndex * _terrainSectionCellCountPerSide);
    const uint2 sectionCellCount = uint2(_terrainSectionCellCountPerSide, _terrainSectionCellCountPerSide);
    const float2 overviewSamplePosition = convertSampleDetailPosition(lod0SampleSectionLocalIndex, lod0Fraction, _terrainOverviewLod, sectionCellCount);
    const float2 overviewSlotTexelOffset = float2(sectionIndex.x % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE, sectionIndex.y % TERRAIN_SECTION_SLOT_COUNT_PER_SIDE) * _terrainSectionOverviewSampleCountPerSide;
    const float2 overviewUV = (overviewSlotTexelOffset + overviewSamplePosition + 0.5f) / float2(_terrainOverviewAtlasTexelCountX, _terrainOverviewAtlasTexelCountY);
    float3 result = float3(getTexture(_terrainOverviewHeightAtlasSRV).SampleLevel(bilinearRepeatSampler, overviewUV, 0.0f).x, getTexture(_terrainOverviewNormalAtlasSRV).SampleLevel(bilinearRepeatSampler, overviewUV, 0.0f).xy);

    // detail result
    const float distanceToCamera = length(relativeXZ - float2(_terrainCameraLocalX, _terrainCameraLocalZ));
    const float desiredLod = clamp(log2(max(distanceToCamera / _terrainLod0WorldRadius, 1.0f)), 0.0f, float(_terrainOverviewLod));
    const uint lower = uint(desiredLod);
    for (int level = int(_terrainOverviewLod) - 1; level >= int(lower); --level)
    {
        const uint lod = uint(level);
        const uint tileCellCountPerSide = _terrainChunkCellCountPerSide << lod;
        GPUASSERT(all(sectionCellCount % tileCellCountPerSide == 0u));
        const uint2 tileCount = sectionCellCount / tileCellCountPerSide;
        const uint2 sectionLocalTileIndex = min(uint2(lod0SampleSectionLocalIndex) / tileCellCountPerSide, tileCount - 1u);    // 셀보다 샘플이 하나 더 있기 때문에 min해줘야함(마지막 sample은 tileCount - 1의 마지막 cell을 참조함)
        const float4 pageEntry = LoadTerrainPageEntry(sectionLocalTileIndex, lod, sectionSlotIndex);
        if (pageEntry.x == 0.0f)
            continue;

        const float2 sampleTileLocalPosition = convertSampleDetailPosition(lod0SampleSectionLocalIndex, lod0Fraction, lod, sectionCellCount) - float2(sectionLocalTileIndex * _terrainChunkCellCountPerSide);

        // 현재 Tile만 먼저 Detail로 전환하면, 아직 Overview나 상위 LOD를 쓰는 이웃과 경계 높이가 달라질 수 있습니다. 이를 줄이려고 호출부에서 Tile 가장자리의 fade를 이웃에 맞춰 낮춥니다.
        float fade = pageEntry.y;
        for (int z = -1; z <= 1; ++z)
        {
            for (int x = -1; x <= 1; ++x)
            {
                const float dx = x < 0 ? sampleTileLocalPosition.x : (x > 0 ? float(_terrainChunkCellCountPerSide) - sampleTileLocalPosition.x : 0.0f);
                const float dz = z < 0 ? sampleTileLocalPosition.y : (z > 0 ? float(_terrainChunkCellCountPerSide) - sampleTileLocalPosition.y : 0.0f);
                if ((x == 0 && z == 0) == false && max(dx, dz) < 2.0f)
                    fade = min(fade, lerp(GetNeighborTerrainTileFade(int2(sectionLocalTileIndex) + int2(x, z), lod, sectionIndex), 1.0f, smoothstep(0.0f, 2.0f, max(dx, dz))));
            }
        }

        const uint cacheSlotIndex = uint(pageEntry.x) - 1u;
        GPUASSERT(cacheSlotIndex < _terrainCacheSlotColumnCount * _terrainCacheSlotRowCount && pageEntry.y >= 0.0f && pageEntry.y <= 1.0f && all(sampleTileLocalPosition >= 0.0f) && all(sampleTileLocalPosition <= float(_terrainChunkCellCountPerSide)));
        const uint2 cacheSlotTexelOffset = uint2(cacheSlotIndex % _terrainCacheSlotColumnCount, cacheSlotIndex / _terrainCacheSlotColumnCount) * _terrainCacheSlotTexelCountPerSide;
        const float2 uv = (float2(cacheSlotTexelOffset) + sampleTileLocalPosition + 0.5f) / float2(_terrainCacheSlotColumnCount * _terrainCacheSlotTexelCountPerSide, _terrainCacheSlotRowCount * _terrainCacheSlotTexelCountPerSide);
        const float3 fine = float3(getTexture(_terrainHeightCacheSRV).SampleLevel(bilinearRepeatSampler, uv, 0.0f).x, getTexture(_terrainNormalCacheSRV).SampleLevel(bilinearRepeatSampler, uv, 0.0f).xy);
        const float lodWeight = lod == lower ? 1.0f - frac(desiredLod) : 1.0f;  // 카메라 거리로 결정한 LOD가 소수일 때, 두 LOD 사이를 부드럽게 보간하는 처리
        result = lerp(result, fine, fade * lodWeight);
    }

    return result;
}

// 카메라 앵커 기준 상대 XZ 위치에서 지형의 높이를 읽는 함수
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
