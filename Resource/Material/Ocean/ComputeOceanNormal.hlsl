#ifndef __DEFINE_COMPUTEOCEANNORMAL_HLSL__
#define __DEFINE_COMPUTEOCEANNORMAL_HLSL__

#include "CommonTexture.hlsl"
#include "Ocean/OceanCommon.hlsl"

cbuffer ComputeOceanNormalCB : register(b0)
{
    TextureParameter _sourceSRV;
};

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    /*
    *   0.1~0.5 부드러운 바다
    *   0.8~1.5 적당한 바다
    *   2.0~5.0 거친 바다
    */
    static float _NormalStrength = 0.25;

    uint2 uv = id.xy;

    Texture2D<float4> HeightTexture = getTexture(_sourceSRV);
    // 주변 높이 샘플링 (경계면 처리를 위해 % 연산으로 타일링 지원)
    float hL = HeightTexture[uint2((uv.x + _N - 1) % _N, uv.y)].x; // 왼쪽
    float hR = HeightTexture[uint2((uv.x + 1) % _N, uv.y)].x;         // 오른쪽
    float hD = HeightTexture[uint2(uv.x, (uv.y + _N - 1) % _N)].x; // 아래
    float hU = HeightTexture[uint2(uv.x, (uv.y + 1) % _N)].x;         // 위

    // 중앙 차분법으로 기울기 계산
    float3 normal;
    normal.x = hL - hR;
    normal.z = hD - hU;
    normal.y = 2.0f / _NormalStrength; // 이 값이 작을수록 파도가 거칠게 표현됨

    normal = normalize(normal);

    RWTexture2D<float4> NormalRWTexture = getTextureRW(_normalUAV);
#if 1
    // 0~1 범위로 매핑하여 텍스처에 저장 (RGB 텍스처일 경우)
    NormalRWTexture[id.xy] = float4(normal * 0.5f + 0.5f, 1.0f);
#else    
    // 만약 월드 공간에서 바로 쓸 데이터라면 매핑 없이 저장
    NormalRWTexture[id.xy] = float4(normal, 0.0f);
#endif
}

#endif