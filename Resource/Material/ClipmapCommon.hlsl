#ifndef DK_CLIPMAP_COMMON_HLSL
#define DK_CLIPMAP_COMMON_HLSL

// 현재 엔진의 바인딩 이름을 유지한다.
cbuffer TerrainMeshConstantBuffer : register(b2)
{
    float4 _baseXY_scale_rotate;
    float4 _morphCenter_start_end;
    float4 _parentOrigin_spacing;

    uint _type;
    uint _level;
    uint _hasParent;
    uint _padding;
};

// Terrain/Ocean 셰이더가 구현한다.
//
// 반환값: 변위를 적용한 최종 월드 XYZ.
//
// worldXZ: 변위를 적용하기 전의 월드 좌표. 텍스처 좌표와 파형 위상은 이 좌표를 기준으로 계산한다.
//
// level: 해당 레벨이 실제 렌더링에 사용하는 변위 데이터를 선택한다.
float3 EvaluateSurfacePosition(float2 worldXZ, uint level);

float2 GetClipmapBaseXZ(float2 vertexPosition)
{
    float2 p = vertexPosition * _baseXY_scale_rotate.z;
    const uint rotation = (uint)_baseXY_scale_rotate.w;

    // 기존 셰이더의 회전과 동일하다.
    if (rotation == 1u)
        p = float2(p.y, -p.x);
    else if (rotation == 2u)
        p = float2(-p.y, p.x);
    else if (rotation == 3u)
        p = -p;

    return p + _baseXY_scale_rotate.xy;
}

float GetClipmapMorphWeight(float2 worldXZ)
{
    if (_hasParent == 0u)
        return 0.0f;

    // 사각형 Clipmap이므로 각 축 거리 중 큰 값을 사용한다.
    // 거리 계산에는 수평 변위 적용 전의 좌표를 사용한다.
    const float2 delta = abs(worldXZ - _morphCenter_start_end.xy);
    const float radius = max(delta.x, delta.y);
    return smoothstep(_morphCenter_start_end.z, _morphCenter_start_end.w, radius);
}

float3 EvaluateParentTriangle(float2 worldXZ)
{
    const float2 parentOrigin = _parentOrigin_spacing.xy;
    const float spacing = _parentOrigin_spacing.z;
    const uint parentLevel = _level + 1u;

    const float2 grid = (worldXZ - parentOrigin) / spacing;

    const float2 cell = floor(grid);
    const float2 f = grid - cell;

    const float2 p0 = parentOrigin + cell * spacing;
    const float3 p00 = EvaluateSurfacePosition(p0, parentLevel);
    const float3 p11 = EvaluateSurfacePosition(p0 + float2(spacing, spacing), parentLevel);

    // 現 Tile 메시의 대각선은 00 → 11이다.
    // 실제 삼각형의 정점 위치를 barycentric 보간한다.
    if (f.x >= f.y)
    {
        const float3 p10 = EvaluateSurfacePosition(p0 + float2(spacing, 0.0f), parentLevel);
        return p00 * (1.0f - f.x) + p10 * (f.x - f.y) + p11 * f.y;
    }

    const float3 p01 = EvaluateSurfacePosition(p0 + float2(0.0f, spacing), parentLevel);

    return p00 * (1.0f - f.y) + p01 * (f.y - f.x) + p11 * f.x;
}

float3 EvaluateClipmapPosition(float2 worldXZ)
{
    const float morph = GetClipmapMorphWeight(worldXZ);

    // 전환 영역 밖에서는 부모 변위를 평가하지 않는다.
    if (morph <= 0.0f)
        return EvaluateSurfacePosition(worldXZ, _level);

    const float3 parentPosition = EvaluateParentTriangle(worldXZ);

    // 경계에서는 부모의 변위된 변에 정확히 맞춘다.
    if (morph >= 1.0f)
        return parentPosition;

    const float3 finePosition = EvaluateSurfacePosition(worldXZ, _level);

    // Ocean의 수평 변위를 포함하여 XYZ 전체를 보간한다.
    return lerp(finePosition, parentPosition, morph);
}

#endif