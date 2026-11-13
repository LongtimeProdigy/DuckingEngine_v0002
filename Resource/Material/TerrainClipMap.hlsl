#ifndef __DEFINE_TERRAINCLIPMAP__HLSL__
#define __DEFINE_TERRAINCLIPMAP__HLSL__

#include "Assert.hlsl"
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
    float3 worldPosition : TEXCOORD1;
    float2 relativeXZ : TEXCOORD2;
};

float3 EvaluateSurfacePosition(float2 relativeXZ, uint level)
{
    return float3(relativeXZ.x, SampleTerrainHeightRelative(relativeXZ), relativeXZ.y);
}

float2 GetClipmapBaseXZ(float2 vertexPosition)
{
    float2 p = vertexPosition * _baseXY_scale_rotate.z;
    const uint rotation = (uint)_baseXY_scale_rotate.w;

    // 기존 셰이더의 회전과 동일하다.
    if (rotation == 1u)
        p = float2(p.y, -p.x);  // -90도 회전
    else if (rotation == 2u)
        p = float2(-p.y, p.x);  // 90도 회전
    else if (rotation == 3u)
        p = -p;                 // 180도 회전

    return p + _baseXY_scale_rotate.xy;
}

// morph를 적용할 weight를 반환합니다. 부모가 없거나 외곽에서 morphCell(현재는 4)보다 작은 위치는 0을 반환합니다. morphCell에 포함되는 경우 0~1 사이의 값으로 모핑합니다.
float GetClipmapMorphWeight(float2 relativeXZ)
{
    if (_hasParent == 0u)
        return 0.0f;

    // 사각형 Clipmap이므로 각 축 거리 중 큰 값을 사용한다.
    // 거리 계산에는 수평 변위 적용 전의 좌표를 사용한다.
    const float2 delta = abs(relativeXZ - _morphCenter_start_end.xy);
    const float radius = max(delta.x, delta.y);
    return smoothstep(_morphCenter_start_end.z, _morphCenter_start_end.w, radius);
}
float3 EvaluateParentTriangle(float2 relativeXZ)
{
    const float2 parentOrigin = _parentOrigin_spacing.xy;
    const float parentSpacing = _parentOrigin_spacing.z;
    const uint parentLevel = _level + 1u;

    const float2 grid = (relativeXZ - parentOrigin) / parentSpacing;

    const float2 cell = floor(grid);
    const float2 f = grid - cell;

    const float2 p0 = parentOrigin + cell * parentSpacing;
    const float3 p00 = EvaluateSurfacePosition(p0, parentLevel);
    const float3 p11 = EvaluateSurfacePosition(p0 + float2(parentSpacing, parentSpacing), parentLevel);

    // 現 Tile 메시의 대각선은 00 → 11이다.
    // 실제 삼각형의 정점 위치를 barycentric 보간한다.
    if (f.x >= f.y)
    {
        const float3 p10 = EvaluateSurfacePosition(p0 + float2(parentSpacing, 0.0f), parentLevel);
        return p00 * (1.0f - f.x) + p10 * (f.x - f.y) + p11 * f.y;
    }
    else
    {
        const float3 p01 = EvaluateSurfacePosition(p0 + float2(0.0f, parentSpacing), parentLevel);
        return p00 * (1.0f - f.y) + p01 * (f.y - f.x) + p11 * f.x;
    }
}
float3 EvaluateClipmapPosition(float2 relativeXZ)
{
    // 현재 격자의 정점을 부모 격자의 삼각형 표면에 점차 맞추는 함수입니다. 
    // terrain의 위치가 이동함으로써 자식-부모 경계에서는 sampling 해상도 차이로 인해 같은 relativeXZ여도 위치가 상이합니다. 상세한 메시와 거친 메시의 경계가 벌어지는 것을 막기 위한 모핑입니다.

    const float morph = GetClipmapMorphWeight(relativeXZ);

    // 전환 영역 밖에서는 부모 변위를 평가하지 않는다.
    if (morph <= 0.0f)
        return EvaluateSurfacePosition(relativeXZ, _level);

    const float3 parentPosition = EvaluateParentTriangle(relativeXZ);
    // 경계에서는 부모의 변위된 변에 정확히 맞춘다.
    if (morph >= 1.0f)
        return parentPosition;

    const float3 finePosition = EvaluateSurfacePosition(relativeXZ, _level);

    // Ocean의 수평 변위를 포함하여 XYZ 전체를 보간한다.
    return lerp(finePosition, parentPosition, morph);
}

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    const float2 baseXZ = GetClipmapBaseXZ(input.position);
    const float3 relativePos = EvaluateClipmapPosition(baseXZ);

    // relativePos는 X, Z는 AnchorX, Z기준의 상대위치이지만 AnchorY는 없기 때문에 y값은 world 기준의 y값이 나옵니다.

    output.relativeXZ = relativePos.xz;
    output.worldPosition = relativePos + float3(float(_terrainAnchorX) * _terrainSampleSpacing, 0.0f, float(_terrainAnchorZ) * _terrainSampleSpacing);
    const float2 viewRelativeXZ = getViewPosition().xz - float2(_terrainCameraWorldX, _terrainCameraWorldZ) + float2(_terrainCameraX, _terrainCameraZ);
    const float3 relativeViewPosition = float3(relativePos.x - viewRelativeXZ.x, relativePos.y - getViewPosition().y, relativePos.z - viewRelativeXZ.y);
    output.position = float4(mul(relativeViewPosition, (float3x3)_cameraWorldMatrixInv), 1.0f);
    output.position = mul(output.position, _cameraProjectionMatrix);

    return output;
}

// worldXZ에 해당하는 normal.xyz를 반환
float3 SampleTerrainNormal(float2 worldXZ)
{
    return DecodeTerrainNormal(SampleTerrainStream(worldXZ).yz);
}

float3 DecodeTerrainSRGB(float3 color)
{
    // The current image loader exposes UNORM views.
    return lerp(color / 12.92f, pow((color + 0.055f) / 1.055f, 2.4f), step(0.04045f, color));
}

float3 SampleTerrainPatch(Texture2D<float4> textureMap, int2 cell, float2 uv, float2 uvDx, float2 uvDy)
{
    // 항상 같은 해시와 같은 변형을 사용하므로 카메라가 움직여도 패턴이 유지됨
    const uint hash = HashTerrainCell(cell);
    
    // 변형UV = 축교환(UV) * 반전값 + 이동값
    // 변형된 UV에 맞는 필터링과 mip 선택을 유지하기 위해서 uvDx, uvDy에도 동일한 축 교환과 반전 적용
    const float2 offset = float2(hash & 255u, (hash >> 8) & 255u) / 256.0f;
    const float2 flip = float2((hash & 0x10000u) != 0u ? -1.0f : 1.0f, (hash & 0x20000u) != 0u ? -1.0f : 1.0f);
    const bool swapAxes = (hash & 0x40000u) != 0u;
    const float2 patchUV = (swapAxes ? uv.yx : uv) * flip + offset;
    const float2 patchDx = (swapAxes ? uvDx.yx : uvDx) * flip;
    const float2 patchDy = (swapAxes ? uvDy.yx : uvDy) * flip;
    return DecodeTerrainSRGB(textureMap.SampleGrad(bilinearRepeatSampler, patchUV, patchDx, patchDy).rgb);
}

// 반복 무늬를 줄이기 위해 위치마다 UV 이동·반전·축 교환을 적용한 세 가지 샘플을 섞습니다
float3 SampleTerrainDetail(Texture2D<float4> textureMap, float2 uv, float2 uvDx, float2 uvDy)
{
    // 0.57735027f == 1/root(3), 1.15470054f == 2/root(3)
    // 3개 패턴을 섞는 기준점 사이의 거리를 균등하게 만들기 위해 정삼각형 격자의 두 방향을 기준으로 UV 위치를 다시 표현하는 식 (위치는 같으나 축이 사각형에서 정삼각형으로 바뀜)
    const float2 grid = float2(uv.x - 0.57735027f * uv.y, 1.15470054f * uv.y);
    const int2 cell = int2(floor(grid));        // grid 공간의 인덱스
    const float2 fraction = frac(grid);         // grid 공간의 local 위치
    const float sum = fraction.x + fraction.y;  // grid 공간에서 y=-x+1 아래인 경우는 lower 삼각형, 위인 경우는 uppwer 삼각형. 이를 바꾸면 x+y이 1보다 작으면 lower, 크면 upper
    const bool upper = sum > 1.0f;
    const int2 firstCell = cell + (upper ? int2(1, 1) : int2(0, 0));
    float3 weights = upper ? float3(sum - 1.0f, 1.0f - fraction.y, 1.0f - fraction.x) : float3(1.0f - sum, fraction.x, fraction.y);
    weights *= weights;
    weights /= weights.x + weights.y + weights.z;
    return weights.x * SampleTerrainPatch(textureMap, firstCell, uv, uvDx, uvDy)
         + weights.y * SampleTerrainPatch(textureMap, cell + int2(1, 0), uv, uvDx, uvDy)
         + weights.z * SampleTerrainPatch(textureMap, cell + int2(0, 1), uv, uvDx, uvDy);
}

float3 EvaluateTerrainAlbedo(float3 worldPosition, float normalY)
{
    const float2 worldXZ = worldPosition.xz;

    // 넓은 노이즈로 흙과 이끼 계열의 분포를 정함
    const float macro = TerrainMacro(worldXZ);
    const float moss = TerrainMossCoverage(macro);

    // 먼저 두 재질의 평균색으로 기본 색상을 만듦
    const float3 mossTint = float3(0.70f, 1.08f, 1.16f);                    // R을 줄이고 G·B를 조금 높이는 색상 보정.
    const float3 mossMean = float3(0.2970f, 0.1784f, 0.0588f) * mossTint;   // 이끼 역할을 하는 텍스처의 평균색에 mossTint를 적용한 값
    const float3 soilMean = float3(0.0957f, 0.0596f, 0.0278f);              // 흙 텍스처의 평균색
    float3 albedo = lerp(soilMean, mossMean, moss);

    // 화면 한 픽셀이 지형에서 얼마나 넓은 범위를 차지하는지 계산
    const float2 dx = ddx(worldXZ);
    const float2 dy = ddy(worldXZ);
    const float footprint = max(length(dx), length(dy));
    const float detailWeight = 1.0f - smoothstep(0.008f, 0.065f, footprint);
    if (detailWeight > 0.0f)
    {
        // 이끼 계열 UV = worldXZ / 3.0
        // 흙 UV       = worldXZ / 2.2
        // 기본 반복 주기를 각각 3m, 2.2m로 잡은 것
        const float3 mossDetail = SampleTerrainDetail(getTexture(_diffuseTexture), worldXZ / 3.0f, dx / 3.0f, dy / 3.0f) * mossTint;
        const float3 soilDetail = SampleTerrainDetail(getTexture(_soilDiffuseTexture), worldXZ / 2.2f, dx / 2.2f, dy / 2.2f);
        albedo = lerp(albedo, lerp(soilDetail, mossDetail, moss), detailWeight);
    }

    // 멀리 있는 풀을 지형 색으로 보완
    // const float distantGrass = 거리 조건 × 높이 조건 × 경사 조건;
    // 거리 44~220m에서 효과를 점차 강화합니다.
    // 지형 높이 0.09~0.85m에서 효과를 점차 허용합니다.
    // normalY가 0.73~0.88인 구간에서 효과를 점차 허용해, 급경사에는 덜 적용합니다.
    const float distanceToGround = length(worldPosition - getViewPosition());
    const float distantGrass = smoothstep(GRASS_LOD1_END, GRASS_LOD4_END, distanceToGround) * smoothstep(0.09f, 0.85f, worldPosition.y) * smoothstep(0.73f, 0.88f, normalY);
    if (distantGrass > 0.0f)
    {
        const float coverage = TerrainGrassDensity(worldXZ, moss) * distantGrass;
        albedo *= lerp(1.0f, float3(0.78f, 1.11f, 0.72f), coverage);
    }

    return albedo * lerp(0.92f, 1.10f, macro);
}

GBufferOutput PSMain(VS_OUTPUT input)
{
#if defined(DK_GPU_ASSERT_ENABLED)
    const float2 minimum = -float2(_terrainAnchorX, _terrainAnchorZ) * _terrainSampleSpacing;
    const float2 maximum = float2(int2(_terrainCellsX, _terrainCellsZ) - int2(_terrainAnchorX, _terrainAnchorZ)) * _terrainSampleSpacing;
    const float4 boundaryDistances = float4(input.relativeXZ - minimum, maximum - input.relativeXZ);
    GPUASSERT(all(boundaryDistances >= 0.0f), boundaryDistances);
#endif
    
    float3 normal;
    if (_terrainStreamEnabled)
        normal = DecodeTerrainNormal(SampleTerrainStreamRelative(input.relativeXZ).yz);
    else
    {
        normal = safeNormal(cross(ddy(input.worldPosition), ddx(input.worldPosition)));
        if (normal.y < 0)
            normal = -normal;
    }

    return writeGBuffer(EvaluateTerrainAlbedo(input.worldPosition, normal.y), normal, _metallic, _roughness);
}
#endif
