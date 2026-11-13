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

#endif