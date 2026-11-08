#include "CommonRendering.hlsl"
#include "GBufferCommon.hlsl"
#include "TerrainSurface.hlsl"
#include "GrassTiles.h"

cbuffer GrassPatchConstants : register(b2)
{
    int _grassTileX;
    int _grassTileZ;
    uint _grassCellStride;
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

float GrassDistanceFade(uint2 relativeCell, float distanceToRoot)
{
    if (any((relativeCell & 1u) != 0u))
        return 1.0f - smoothstep(14.0f, GRASS_LOD0_END, distanceToRoot);
    if (any((relativeCell & 2u) != 0u))
        return 1.0f - smoothstep(32.0f, GRASS_LOD1_END, distanceToRoot);
    if (any((relativeCell & 4u) != 0u))
        return 1.0f - smoothstep(62.0f, GRASS_LOD2_END, distanceToRoot);
    if (any((relativeCell & 8u) != 0u))
        return 1.0f - smoothstep(111.0f, GRASS_LOD3_END, distanceToRoot);
    return 1.0f - smoothstep(175.0f, GRASS_LOD4_END, distanceToRoot);
}

GrassVertex VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    GrassVertex output = (GrassVertex)0;
    output.position = float4(0.0f, 0.0f, -1.0f, 1.0f);

    const int2 tile = int2(_grassTileX, _grassTileZ);
    const uint tileHash = HashTerrainCell(tile);
    const uint2 phase = uint2(tileHash, tileHash >> 8) & 15u;
    const uint gridSide = GRASS_TILE_GRID_SIDE / _grassCellStride;
    const uint2 gridCell = uint2(instanceID % gridSide, instanceID / gridSide);
    // Coarser draws keep a nested subset of the same world cells.
    const uint2 tileCell = gridCell * _grassCellStride + (phase & (_grassCellStride - 1u));
    const int2 worldCell = tile * int(GRASS_TILE_GRID_SIDE) + int2(tileCell);
    const uint seed = HashTerrainCell(worldCell);
    const float2 jitter = float2(GrassRandom(seed), GrassRandom(seed >> 16));
    const float2 rootXZ = (float2(worldCell) + lerp(0.08f, 0.92f, jitter)) * (GRASS_TILE_METERS / GRASS_TILE_GRID_SIDE);
    const float rootHeight = SampleTerrainHeight(rootXZ);
    if (rootHeight < 0.09f)
        return output;

    const float distanceToRoot = length(float3(rootXZ.x, rootHeight, rootXZ.y) - getViewPosition());
    const float distanceFade = GrassDistanceFade(tileCell ^ phase, distanceToRoot);
    if (distanceFade <= 0.0f)
        return output;

    const float moss = TerrainMossCoverage(TerrainMacro(rootXZ));
    if (GrassRandom(HashTerrainCell(worldCell + int2(183, -71))) >= TerrainGrassDensity(rootXZ, moss))
        return output;

    const float sampleStep = 0.5f;
    const float2 slope = (float2(SampleTerrainHeight(rootXZ + float2(sampleStep, 0.0f)),
                                 SampleTerrainHeight(rootXZ + float2(0.0f, sampleStep))) - rootHeight) / sampleStep;
    if (dot(slope, slope) > 0.64f)
        return output;

    // The indexed clump has two quads and one tip triangle per blade.
    const uint blade = vertexID / 7u;
    const uint bladeVertex = vertexID % 7u;
    const uint row = bladeVertex / 2u;
    const float bladeRows[4] = { 0.0f, 0.36f, 0.72f, 1.0f };
    const float bladeWidths[4] = { 0.66f, 1.0f, 0.57f, 0.0f };
    const float t = bladeRows[row];
    const float sideSign = (bladeVertex & 1u) != 0u ? 1.0f : -1.0f;

    const uint shape = HashTerrainCell(worldCell + int2(-71, 313));
    const uint profileCount = sizeof(grassProfiles) / sizeof(grassProfiles[0]);
    const GrassProfile profile = grassProfiles[uint(GrassRandom(shape) * profileCount)];
    if (blade >= profile.bladeCount)
        return output;
    const uint bladeSeed = HashTerrainCell(worldCell + int2(97 + int(blade) * 23, -43));
    const float angle = (float(blade) + GrassRandom(bladeSeed) * 0.85f) * 1.04719755f
                      + GrassRandom(shape >> 16) * 6.28318531f;
    float2 heading;
    sincos(angle, heading.y, heading.x);
    const float2 side = float2(-heading.y, heading.x);

    const float height = profile.height * distanceFade * lerp(0.67f, 1.24f, GrassRandom(bladeSeed >> 16));
    const float width = profile.width * lerp(0.75f, 1.24f, GrassRandom(bladeSeed >> 8));
    const float2 bladeRoot = heading * profile.spread * lerp(0.14f, 1.0f, GrassRandom(bladeSeed >> 6));
    const float2 windDirection = float2(0.84f, 0.5426f);
    const float gust = 0.57f + 0.28f * sin(1.85f * _time - 0.39f * dot(rootXZ, windDirection))
                     + 0.15f * sin(3.4f * _time + 0.27f * rootXZ.y + angle);
    const float2 bend = (heading * (profile.lean * height) + windDirection * (0.25f * height * gust)) * (t * t);
    const float2 worldXZ = rootXZ + bladeRoot + side * (sideSign * width * 0.5f * bladeWidths[row]) + bend;
    const float rootDepth = lerp(0.018f, 0.12f, smoothstep(30.0f, 150.0f, distanceToRoot));
    const float worldY = rootHeight + dot(slope, bladeRoot) - rootDepth + height * t;
    const float4 viewPosition = mul(float4(worldXZ.x, worldY, worldXZ.y, 1.0f), _cameraWorldMatrixInv);
    output.position = mul(viewPosition, _cameraProjectionMatrix);

    const float dry = saturate(profile.dryness + 0.28f * (GrassRandom(bladeSeed >> 11) - 0.5f)
                             + 0.14f * (0.5f - moss));
    const float3 rootColor = lerp(float3(0.026f, 0.043f, 0.009f), float3(0.050f, 0.048f, 0.013f), dry);
    const float3 tipColor = lerp(float3(0.086f, 0.154f, 0.030f), float3(0.194f, 0.170f, 0.047f), dry);
    output.albedo = lerp(rootColor, tipColor, t);
    output.normal = normalize(float3(-0.55f * heading.x, 1.0f, -0.55f * heading.y));
    return output;
}

GBufferOutput PSMain(GrassVertex input)
{
    return writeGBuffer(input.albedo, input.normal, 0.0f, 0.88f);
}
