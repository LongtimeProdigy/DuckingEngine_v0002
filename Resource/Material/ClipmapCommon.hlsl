#ifndef DK_CLIPMAP_COMMON_HLSL
#define DK_CLIPMAP_COMMON_HLSL

#define DK_SHADER_TERRAIN_MESH
#include "ShaderStructures.h"
#undef DK_SHADER_TERRAIN_MESH

// Terrain/Ocean 셰이더가 구현한다.
//
// 반환값: 변위를 적용한 최종 월드 XYZ.
//
// worldXZ: 변위를 적용하기 전의 월드 좌표. 텍스처 좌표와 파형 위상은 이 좌표를 기준으로 계산한다.
//
// level: 해당 레벨이 실제 렌더링에 사용하는 변위 데이터를 선택한다.
float3 EvaluateSurfacePosition(float2 worldXZ, uint level);

#endif
