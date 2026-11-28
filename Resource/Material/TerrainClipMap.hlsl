#ifndef __DEFINE_TERRAINCLIPMAP__HLSL__
#define __DEFINE_TERRAINCLIPMAP__HLSL__

#include "Terrain.hlsl"
#include "CommonRendering.hlsl"
#include "ClipmapCommon.hlsl"
#include "GBufferCommon.hlsl"

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

// X, Z는 AnchorX, Z기준의 상대위치이지만 AnchorY는 없기 때문에 y값은 world 기준의 y값이 나옵니다.
float3 EvaluateSurfacePosition(float2 relativeXZ, uint level)
{
    return float3(relativeXZ.x, SampleTerrainHeightRelative(relativeXZ), relativeXZ.y);
}

// ClipMap Vertex를 Camera Anchored Local 위치이동 (SRT)
float2 GetClipmapBaseXZ(float2 vertexPosition)
{
    float2 p = vertexPosition * _baseXY_scale_rotate.z;

    // 기존 셰이더의 회전과 동일하다.
    const uint rotation = (uint)_baseXY_scale_rotate.w;
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

    // 사각형 Clipmap이므로 각 축 거리 중 큰 값을 사용한다. 거리 계산에는 수평 변위 적용 전의 좌표를 사용한다.
    const float2 delta = abs(relativeXZ - _morphCenter_start_end.xy);
    const float deltaMax = max(delta.x, delta.y);
    return smoothstep(_morphCenter_start_end.z, _morphCenter_start_end.w, deltaMax);
}
// relativeXZ의 위치를 삼각형 barycentric으로 보간하여 위치 산출
float3 EvaluateParentTriangle(float2 relativeXZ)
{
    const float2 parentOriginRelativeWorldXZ = _parentOriginRelativeWorldXZ;
    const float parentSpacing = _baseXY_scale_rotate.z * 2.0f;
    const uint parentLevel = _level + 1u;

    const float2 grid = (relativeXZ - parentOriginRelativeWorldXZ) / parentSpacing;

    const float2 cell = floor(grid);
    const float2 f = grid - cell;

    const float2 p0 = parentOriginRelativeWorldXZ + cell * parentSpacing;
    const float3 p00 = EvaluateSurfacePosition(p0, parentLevel);
    const float3 p11 = EvaluateSurfacePosition(p0 + float2(parentSpacing, parentSpacing), parentLevel);

    // 現 Patch 메시의 대각선은 00 → 11이다.
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
// 촘촘한 메시가 거친 메시로 바뀌는 경계의 형상 변화를 완화하기 위해 morph가 필요하면 부모의 위치와 Blend하여 위치 산출
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

    output.relativeXZ = relativePos.xz;
    const float2 cameraAnchorWorldXZ = TerrainCameraAnchorWorldXZ();
    output.worldPosition = relativePos + float3(cameraAnchorWorldXZ.x, 0.0f, cameraAnchorWorldXZ.y);
    // const float2 viewAnchorRelativeXZ = getViewPosition().xz - cameraAnchorWorldXZ; // 부동소수점 문제로 아래와같이 계산함
    const float2 viewAnchorRelativeXZ = getViewPosition().xz - (float2(_terrainCameraWorldX, _terrainCameraWorldZ) - float2(_terrainCameraLocalX, _terrainCameraLocalZ)); // 현재 렌더링에 사용하는 카메라 위치를 지형 앵커 기준 상대 좌표로 바꾸는 계산
    const float3 positionViewRelative = float3(relativePos.x - viewAnchorRelativeXZ.x, relativePos.y - getViewPosition().y, relativePos.z - viewAnchorRelativeXZ.y);
    output.position = float4(mul(positionViewRelative, (float3x3)_cameraWorldMatrixInv), 1.0f);
    output.position = mul(output.position, _cameraProjectionMatrix);

    return output;
}

float3 DecodeTerrainNormal(float2 normalXZ)
{
    return normalize(float3(normalXZ.x, sqrt(saturate(1.0f - dot(normalXZ, normalXZ))), normalXZ.y));
}

struct TerrainMaterialSample
{
    float3 _albedo;
    float3 _normal;
    float _roughnessSquared;
};

TerrainMaterialSample SampleTerrainDetail(Texture2D<float4> albedoTexture, Texture2D<float4> surfaceTexture, float2 uv, float2 uvDx, float2 uvDy)
{
    const float2 grid = float2(uv.x - 0.57735027f * uv.y, 1.15470054f * uv.y);
    const int2 cell = int2(floor(grid));
    const float2 fraction = frac(grid);
    const float sum = fraction.x + fraction.y;
    const bool upper = sum > 1.0f;
    const int2 cells[3] = { cell + (upper ? int2(1, 1) : int2(0, 0)), cell + int2(1, 0), cell + int2(0, 1) };
    float3 weights = upper ? float3(sum - 1.0f, 1.0f - fraction.y, 1.0f - fraction.x) : float3(1.0f - sum, fraction.x, fraction.y);
    weights *= weights;
    weights /= weights.x + weights.y + weights.z;

    TerrainMaterialSample result = (TerrainMaterialSample)0;
    [unroll]
    for (uint patch = 0; patch < 3; ++patch)
    {
        const uint hash = HashGridCell(cells[patch]);
        const float2 offset = float2(hash & 255u, (hash >> 8) & 255u) / 256.0f;
        const float2 flip = float2((hash & 0x10000u) != 0u ? -1.0f : 1.0f, (hash & 0x20000u) != 0u ? -1.0f : 1.0f);
        const bool swapAxes = (hash & 0x40000u) != 0u;
        const float2 patchUV = (swapAxes ? uv.yx : uv) * flip + offset;
        const float2 patchDx = (swapAxes ? uvDx.yx : uvDx) * flip;
        const float2 patchDy = (swapAxes ? uvDy.yx : uvDy) * flip;
        const float4 surface = surfaceTexture.SampleGrad(bilinearRepeatSampler, patchUV, patchDx, patchDy);
        float2 uvNormal = (surface.xy * 2.0f - 1.0f) * float2(1.0f, -1.0f) * flip;
        uvNormal = swapAxes ? uvNormal.yx : uvNormal;
        const float weight = weights[patch];
        result._albedo += weight * albedoTexture.SampleGrad(bilinearRepeatSampler, patchUV, patchDx, patchDy).rgb;
        result._normal += weight * float3(uvNormal, surface.z * 2.0f - 1.0f);
        result._roughnessSquared += weight * surface.w * surface.w;
    }
    return result;
}

TerrainMaterialSample BlendTerrainMaterials(TerrainMaterialSample a, TerrainMaterialSample b, float weight)
{
    TerrainMaterialSample result;
    result._albedo = lerp(a._albedo, b._albedo, weight);
    result._normal = lerp(a._normal, b._normal, weight);
    result._roughnessSquared = lerp(a._roughnessSquared, b._roughnessSquared, weight);
    return result;
}

GBufferOutput EvaluateTerrainMaterial(float3 worldPosition, float3 terrainNormal)
{
    const float2 worldXZ = worldPosition.xz;
    const float2 dx = ddx(worldXZ);
    const float2 dy = ddy(worldXZ);
    const float macro = TerrainMacro(worldXZ);
    const float inclination = 1.0f - terrainNormal.y;
    const float wetness = (1.0f - smoothstep(1.5f, 22.0f, worldPosition.y)) * (1.0f - smoothstep(0.03f, 0.18f, inclination));
    const float moss = TerrainMossCoverage(macro) * (1.0f - smoothstep(0.06f, 0.27f, inclination));
    const float mossWeight = saturate(moss + 0.22f * wetness);
    const float alpineRock = smoothstep(_alpineRockStartWorldHeight, _alpineRockEndWorldHeight, worldPosition.y);
    const float rockWeight = max(alpineRock, smoothstep(0.10f, 0.33f, inclination + (macro - 0.5f) * 0.10f));

    TerrainMaterialSample litter = SampleTerrainDetail(getTexture(_diffuseTexture), getTexture(_mossSurfaceTexture), worldXZ / 3.0f, dx / 3.0f, dy / 3.0f);
    litter._albedo *= float3(0.70f, 1.08f, 1.16f);
    const TerrainMaterialSample soil = SampleTerrainDetail(getTexture(_soilDiffuseTexture), getTexture(_soilSurfaceTexture), worldXZ / 2.1f, dx / 2.1f, dy / 2.1f);
    TerrainMaterialSample material = BlendTerrainMaterials(soil, litter, mossWeight);
    if (rockWeight > 0.0f)
    {
        const TerrainMaterialSample rock = SampleTerrainDetail(getTexture(_rockDiffuseTexture), getTexture(_rockSurfaceTexture), worldXZ / 3.2f, dx / 3.2f, dy / 3.2f);
        material = BlendTerrainMaterials(material, rock, rockWeight);
    }

    const float distanceToGround = length(worldPosition - getViewPosition());
    const float2 coverageSize = float2(_groundCoverageWidth, _groundCoverageDepth);
    const bool hasCoverageBounds = all(coverageSize > 0.0f);
    const float2 mapSize = hasCoverageBounds ? coverageSize : float2(_terrainCellCountX, _terrainCellCountZ) * _terrainSampleWorldSpacing;
    const float2 mapOrigin = hasCoverageBounds ? float2(_groundCoverageOriginX, _groundCoverageOriginZ) : float2(_terrainOriginWorldX, _terrainOriginWorldZ) + float2(_terrainMinimumSampleX, _terrainMinimumSampleZ) * _terrainSampleWorldSpacing;
    const float2 mapXZ = worldXZ - mapOrigin;
    const float distanceOutside = length(mapXZ - clamp(mapXZ, 0.0f, mapSize));
    const float coverageFade = 1.0f - smoothstep(0.0f, max(_groundCoverageFadeMeters, 1e-4f), distanceOutside);
    const float groundCoverageFactor = (1.0f - alpineRock) * smoothstep(0.09f, 0.85f, worldPosition.y) * smoothstep(0.59f, 0.80f, terrainNormal.y);
    if (groundCoverageFactor > 0.0f)
    {
        const float exteriorMeadow = _groundCoverageExteriorMeadow * lerp(0.58f, 1.0f, mossWeight);
        float2 groundCoverage = float2(exteriorMeadow, 0.0f);
        if (coverageFade > 0.0f)
        {
            const float2 authoredCoverage = SampleTerrainMap(_groundCoverageTexture, mapXZ / mapSize).rg;
            groundCoverage = lerp(groundCoverage, authoredCoverage, coverageFade);
        }
        const float meadowBlend = groundCoverageFactor * groundCoverage.r;
        const float groundColorDistance = smoothstep(_groundColorNearDistance, _groundColorFarDistance, distanceToGround);
        const float detail = saturate(dot(material._albedo, float3(0.2126f, 0.7152f, 0.0722f)) / 0.22f);
        const float3 meadowColor = lerp(float3(0.023f, 0.046f, 0.009f), float3(0.060f, 0.107f, 0.023f), 0.27f * macro + 0.59f * groundColorDistance);
        const float woodlandBlend = groundCoverageFactor * groundCoverage.g;
        const float3 forestFloorColor = float3(0.054f, 0.038f, 0.021f);
        const float detailScale = lerp(0.69f, 1.22f, detail);
        material._albedo = lerp(material._albedo, forestFloorColor * detailScale, woodlandBlend);
        material._albedo = lerp(material._albedo, meadowColor * detailScale, meadowBlend);
    }

    const float3 tangent = normalize(float3(terrainNormal.y, -terrainNormal.x, 0.0f));
    const float3 bitangent = cross(tangent, terrainNormal);
    const float3 normal = normalize(tangent * material._normal.x + bitangent * material._normal.y + terrainNormal * material._normal.z);
    const float3 albedo = material._albedo * lerp(0.87f, 1.10f, macro) * lerp(1.0f, 0.76f, wetness);
    const float roughness = sqrt(material._roughnessSquared) * lerp(1.0f, 0.77f, wetness) * _roughness;
    return writeGBuffer(albedo, normal, _metallic, roughness);
}

GBufferOutput PSMain(VS_OUTPUT input)
{
    float3 normal;
    if (_terrainStreamingEnabled)
        normal = DecodeTerrainNormal(SampleTerrainStreamRelative(input.relativeXZ).yz);
    else
    {
        normal = safeNormal(cross(ddy(input.worldPosition), ddx(input.worldPosition)));
        if (normal.y < 0)
            normal = -normal;
    }

    return EvaluateTerrainMaterial(input.worldPosition, normal);
}
#endif
