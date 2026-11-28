#include "CommonRendering.hlsl"
#include "GBufferCommon.hlsl"
#include "Terrain.hlsl"
#include "GridHash.hlsl"
#include "GrassTiles.h"

cbuffer GrassPatchConstants : register(b2)
{
    int _grassTileX;
    int _grassTileZ;
    uint _grassCellStride;
    TextureParameter _grassPlacementTexture;
    int _grassAnchorX;
    int _grassAnchorZ;
    float _grassOffsetX;
    float _grassOffsetZ;
    uint _grassTileGridSide;
    float _grassMapOriginX;
    float _grassMapOriginZ;
    float _grassMapWidth;
    float _grassMapDepth;
    float _grassHeightMaskStrength;
    float _grassAlbedoScale;
}

struct GrassVertex
{
    float4 position : SV_POSITION;
    float3 albedo : COLOR;
    float3 normal : TEXCOORD0;
};

struct GrassProfile
{
    float height;
    float width;
    float spread;
    float lean;
    float dryness;
    uint bladeCount;
};

static const GrassProfile grassProfiles[] =
{
    { 0.17f, 0.012f, 0.050f, 0.24f, 0.08f, 6 }, // Fine low grass
    { 0.22f, 0.021f, 0.093f, 0.52f, 0.18f, 6 }, // Spreading short tuft
    { 0.34f, 0.008f, 0.040f, 0.14f, 0.10f, 6 }, // Upright needle blades
    { 0.39f, 0.031f, 0.075f, 0.39f, 0.07f, 5 }, // Broad green blades
    { 0.50f, 0.017f, 0.086f, 0.71f, 0.22f, 5 }, // Arching tuft
    { 0.45f, 0.036f, 0.080f, 0.45f, 0.14f, 4 }, // Broad fan
    { 0.71f, 0.012f, 0.052f, 0.26f, 0.22f, 5 }, // Tall fine grass
    { 0.82f, 0.018f, 0.087f, 0.64f, 0.27f, 6 }, // Tall bowed grass
    { 0.65f, 0.011f, 0.055f, 0.28f, 0.78f, 5 }, // Dry upright stems
    { 0.27f, 0.016f, 0.100f, 0.72f, 0.88f, 4 }, // Low straw fan
};

float GrassRandom(uint hash)
{
    return float(hash & 0xffffu) / 65536.0f;
}

float2 GrassFadeRange(uint2 relativeCell)
{
    if (any((relativeCell & 1u) != 0u))
        return float2(12.0f, GRASS_LOD0_END);
    if (any((relativeCell & 2u) != 0u))
        return float2(30.0f, GRASS_LOD1_END);
    if (any((relativeCell & 4u) != 0u))
        return float2(58.0f, GRASS_LOD2_END);
    if (any((relativeCell & 8u) != 0u))
        return float2(104.0f, GRASS_LOD3_END);
    if (any((relativeCell & 16u) != 0u))
        return float2(159.0f, GRASS_LOD4_END);
    return float2(219.0f, GRASS_LOD5_END);
}

GrassVertex VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    GrassVertex output = (GrassVertex)0;
    output.position = float4(0.0f, 0.0f, -1.0f, 1.0f);

    const int2 tile = int2(_grassTileX, _grassTileZ);
    const uint tileHash = HashGridCell(tile);
    const uint2 phase = uint2(tileHash, tileHash >> 8) & 31u;
    const uint gridSide = _grassTileGridSide / _grassCellStride;
    const uint2 gridCell = uint2(instanceID % gridSide, instanceID / gridSide);
    // Coarser draws keep a nested subset of the same world cells.
    const uint2 tileCell = gridCell * _grassCellStride + (phase & (_grassCellStride - 1u));
    const int2 worldCell = tile * int(_grassTileGridSide) + int2(tileCell);
    const uint seed = HashGridCell(worldCell);
    const float2 jitter = float2(GrassRandom(seed), GrassRandom(seed >> 16));
    const float2 rootRelativeXZ = (float2(worldCell - int2(_grassAnchorX, _grassAnchorZ)) + lerp(0.08f, 0.92f, jitter)) * (GRASS_TILE_METERS / _grassTileGridSide) - float2(_grassOffsetX, _grassOffsetZ);
    const float2 rootXZ = rootRelativeXZ + TerrainCameraAnchorWorldXZ();
    const float2 mapUV = (rootXZ - float2(_grassMapOriginX, _grassMapOriginZ)) / float2(_grassMapWidth, _grassMapDepth);
    const int2 minimumSample = int2(_terrainMinimumSampleX, _terrainMinimumSampleZ);
    const float2 minimum = float2(minimumSample - int2(_terrainCameraAnchorSampleX, _terrainCameraAnchorSampleZ)) * _terrainSampleWorldSpacing;
    const float2 maximum = float2(minimumSample + int2(_terrainCellCountX, _terrainCellCountZ) - int2(_terrainCameraAnchorSampleX, _terrainCameraAnchorSampleZ)) * _terrainSampleWorldSpacing;
    if (TerrainAvailableRelative(rootRelativeXZ) == false || any(rootRelativeXZ < minimum) || any(rootRelativeXZ > maximum) || any(mapUV < 0.0f) || any(mapUV > 1.0f))
    {
        return output;
    }

    const float2 viewRelativeXZ = getViewPosition().xz - float2(_terrainCameraWorldX, _terrainCameraWorldZ) + float2(_terrainCameraLocalX, _terrainCameraLocalZ);
    const float2 rootToViewXZ = rootRelativeXZ - viewRelativeXZ;
    const float2 fadeRange = GrassFadeRange(tileCell ^ phase);
    if (dot(rootToViewXZ, rootToViewXZ) >= fadeRange.y * fadeRange.y)
        return output;

    const uint blade = vertexID / GRASS_BLADE_VERTEX_COUNT;
    const uint shape = HashGridCell(worldCell + int2(-71, 313));
    const uint profileCount = sizeof(grassProfiles) / sizeof(grassProfiles[0]);
    const GrassProfile profile = grassProfiles[uint(pow(GrassRandom(shape), 1.35f) * profileCount)];
    if (blade >= profile.bladeCount)
        return output;

    const float4 placement = SampleTerrainMap(_grassPlacementTexture, mapUV);
    if (GrassRandom(HashGridCell(worldCell + int2(183, -71))) >= placement.r)
        return output;

    float2 slope;
    const float rootHeight = SampleTerrainHeightAndSlopeRelative(rootRelativeXZ, slope);
    if (rootHeight < 0.09f || dot(slope, slope) > 0.64f)
        return output;

    const float distanceToRoot = length(float3(rootToViewXZ.x, rootHeight - getViewPosition().y, rootToViewXZ.y));
    const float distanceFade = 1.0f - smoothstep(fadeRange.x, fadeRange.y, distanceToRoot);
    if (distanceFade <= 0.0f)
        return output;

    // The indexed clump has two quads and one tip triangle per blade.
    const uint bladeVertex = vertexID % GRASS_BLADE_VERTEX_COUNT;
    const uint row = bladeVertex / 2u;
    const float bladeRows[4] = { 0.0f, 0.36f, 0.72f, 1.0f };
    const float bladeWidths[4] = { 0.66f, 1.0f, 0.57f, 0.0f };
    const float t = bladeRows[row];
    const float sideSign = (bladeVertex & 1u) != 0u ? 1.0f : -1.0f;

    const uint bladeSeed = HashGridCell(worldCell + int2(97 + int(blade) * 23, -43));
    const float angle = (float(blade) + GrassRandom(bladeSeed) * 0.85f) * (PI2 / float(GRASS_BLADE_COUNT))
                      + GrassRandom(shape >> 16) * PI2;
    float2 heading;
    sincos(angle, heading.y, heading.x);
    const float2 side = float2(-heading.y, heading.x);

    const float heightScale = lerp(1.0f, placement.a, _grassHeightMaskStrength);
    const float height = profile.height * heightScale * distanceFade * lerp(0.67f, 1.24f, GrassRandom(bladeSeed >> 16));
    const float width = profile.width * lerp(0.83f, 1.38f, GrassRandom(bladeSeed >> 8));
    const float2 bladeRoot = heading * profile.spread * lerp(0.14f, 1.0f, GrassRandom(bladeSeed >> 6));
    const float2 windDirection = float2(0.84f, 0.5426f);
    const float gust = 0.57f + 0.28f * sin(1.85f * _time - 0.39f * dot(rootXZ, windDirection))
                     + 0.15f * sin(3.4f * _time + 0.27f * rootXZ.y + angle);
    const float2 bend = (heading * (profile.lean * height) + windDirection * (0.25f * height * gust)) * (t * t);
    const float2 worldXZ = rootRelativeXZ + bladeRoot + side * (sideSign * width * 0.5f * bladeWidths[row]) + bend;
    const float rootDepth = lerp(0.018f, 0.12f, smoothstep(30.0f, 150.0f, distanceToRoot));
    const float worldY = rootHeight + dot(slope, bladeRoot) - rootDepth + height * t;
    const float3 relativeViewPosition = float3(worldXZ.x - viewRelativeXZ.x, worldY - getViewPosition().y, worldXZ.y - viewRelativeXZ.y);
    const float4 viewPosition = float4(mul(relativeViewPosition, (float3x3)_cameraWorldMatrixInv), 1.0f);
    output.position = mul(viewPosition, _cameraProjectionMatrix);

    const float dry = saturate(profile.dryness * placement.g + 0.28f * (GrassRandom(bladeSeed >> 11) - 0.5f)
                             + 0.14f * (0.5f - placement.b));
    const float3 rootColor = lerp(float3(0.016f, 0.033f, 0.008f), float3(0.037f, 0.038f, 0.011f), dry);
    const float3 tipColor = lerp(float3(0.053f, 0.113f, 0.022f), float3(0.137f, 0.128f, 0.036f), dry);
    const float colorVariation = lerp(0.86f, 1.15f, GrassRandom(shape >> 8));
    output.albedo = lerp(rootColor, tipColor, t) * (colorVariation * _grassAlbedoScale);
    output.normal = normalize(float3(-0.55f * heading.x, 1.0f, -0.55f * heading.y));
    return output;
}

GBufferOutput PSMain(GrassVertex input)
{
    return writeGBuffer(input.albedo, input.normal, 0.0f, 0.88f);
}
