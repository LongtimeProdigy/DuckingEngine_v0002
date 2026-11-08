#ifndef __DEFINE_TERRAINCLIPMAP__HLSL__
#define __DEFINE_TERRAINCLIPMAP__HLSL__

#include "TerrainSurface.hlsl"
#include "CommonRendering.hlsl"
#include "ClipmapCommon.hlsl"
#include "GBufferCommon.hlsl"
#include "GrassTiles.h"

struct VS_INPUT
{
    float2 position : POSITION;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 worldPosition : TEXCOORD1;
};

float3 EvaluateSurfacePosition(float2 worldXZ, uint level)
{
    return float3(worldXZ.x, SampleTerrainHeight(worldXZ), worldXZ.y);
}

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    const float2 baseXZ = GetClipmapBaseXZ(input.position);
    const float3 worldPosition = EvaluateClipmapPosition(baseXZ);

    output.worldPosition = worldPosition;
    output.position = mul(float4(worldPosition, 1.0f), _cameraWorldMatrixInv);

    output.position = mul(output.position, _cameraProjectionMatrix);

    static const float4 colors[5] =
    {
        float4(0, 0, 0, 1), // Cross
        float4(1, 0, 0, 1), // Tile
        float4(0, 1, 0, 1), // Filler
        float4(0, 0, 1, 1), // Seam
        float4(1, 1, 0, 1)  // Trim
    };

    output.color = colors[_type];

    return output;
}

float3 DecodeTerrainSRGB(float3 color)
{
    // The current image loader exposes UNORM views.
    return lerp(color / 12.92f, pow((color + 0.055f) / 1.055f, 2.4f), step(0.04045f, color));
}

float3 SampleTerrainPatch(Texture2D<float4> textureMap, int2 cell, float2 uv, float2 uvDx, float2 uvDy)
{
    const uint hash = HashTerrainCell(cell);
    const float2 offset = float2(hash & 255u, (hash >> 8) & 255u) / 256.0f;
    const float2 flip = float2((hash & 0x10000u) != 0u ? -1.0f : 1.0f,
                              (hash & 0x20000u) != 0u ? -1.0f : 1.0f);
    const bool swapAxes = (hash & 0x40000u) != 0u;
    const float2 patchUV = (swapAxes ? uv.yx : uv) * flip + offset;
    const float2 patchDx = (swapAxes ? uvDx.yx : uvDx) * flip;
    const float2 patchDy = (swapAxes ? uvDy.yx : uvDy) * flip;
    return DecodeTerrainSRGB(textureMap.SampleGrad(bilinearRepeatSampler, patchUV, patchDx, patchDy).rgb);
}

float3 SampleTerrainDetail(Texture2D<float4> textureMap, float2 uv, float2 uvDx, float2 uvDy)
{
    // Three neighboring patches share weights across the triangular grid edges.
    const float2 grid = float2(uv.x - 0.57735027f * uv.y, 1.15470054f * uv.y);
    const int2 cell = int2(floor(grid));
    const float2 fraction = frac(grid);
    const float sum = fraction.x + fraction.y;
    const bool upper = sum > 1.0f;
    const int2 firstCell = cell + (upper ? int2(1, 1) : int2(0, 0));
    float3 weights = upper ? float3(sum - 1.0f, 1.0f - fraction.y, 1.0f - fraction.x)
                           : float3(1.0f - sum, fraction.x, fraction.y);
    weights *= weights;
    weights /= weights.x + weights.y + weights.z;
    return weights.x * SampleTerrainPatch(textureMap, firstCell, uv, uvDx, uvDy)
         + weights.y * SampleTerrainPatch(textureMap, cell + int2(1, 0), uv, uvDx, uvDy)
         + weights.z * SampleTerrainPatch(textureMap, cell + int2(0, 1), uv, uvDx, uvDy);
}

float3 EvaluateTerrainAlbedo(float3 worldPosition, float normalY)
{
    const float2 worldXZ = worldPosition.xz;
    const float2 dx = ddx(worldXZ);
    const float2 dy = ddy(worldXZ);
    const float footprint = max(length(dx), length(dy));
    const float macro = TerrainMacro(worldXZ);
    const float moss = TerrainMossCoverage(macro);

    // Fade sub-pixel detail to each source texture's linear average until image mips are available.
    const float detailWeight = 1.0f - smoothstep(0.008f, 0.065f, footprint);
    const float3 mossTint = float3(0.70f, 1.08f, 1.16f);
    const float3 mossMean = float3(0.2970f, 0.1784f, 0.0588f) * mossTint;
    const float3 soilMean = float3(0.0957f, 0.0596f, 0.0278f);
    float3 albedo = lerp(soilMean, mossMean, moss);
    if (detailWeight > 0.0f)
    {
        const float3 mossDetail = SampleTerrainDetail(getTexture(_diffuseTexture), worldXZ / 3.0f, dx / 3.0f, dy / 3.0f) * mossTint;
        const float3 soilDetail = SampleTerrainDetail(getTexture(_soilDiffuseTexture), worldXZ / 2.2f, dx / 2.2f, dy / 2.2f);
        albedo = lerp(albedo, lerp(soilDetail, mossDetail, moss), detailWeight);
    }
    const float distanceToGround = length(worldPosition - getViewPosition());
    const float distantGrass = smoothstep(GRASS_LOD1_END, GRASS_LOD4_END, distanceToGround)
                             * smoothstep(0.09f, 0.85f, worldPosition.y) * smoothstep(0.73f, 0.88f, normalY);
    if (distantGrass > 0.0f)
    {
        const float coverage = TerrainGrassDensity(worldXZ, moss) * distantGrass;
        albedo *= lerp(1.0f, float3(0.78f, 1.11f, 0.72f), coverage);
    }
    return albedo * lerp(0.92f, 1.10f, macro);
}

GBufferOutput PSMain(VS_OUTPUT input)
{
    float3 normal = safeNormal(cross(ddy(input.worldPosition), ddx(input.worldPosition)));
    if (normal.y < 0) 
        normal = -normal;

    return writeGBuffer(EvaluateTerrainAlbedo(input.worldPosition, normal.y), normal, _metallic, _roughness);
}
#endif