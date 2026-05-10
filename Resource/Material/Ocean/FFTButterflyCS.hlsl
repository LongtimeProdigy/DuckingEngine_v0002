#ifndef __DEFINE_FFTBUTTERFLY_HLSL__
#define __DEFINE_FFTBUTTERFLY_HLSL__

#include "CommonMath.hlsl"
#include "CommonComplex.hlsl"
#include "CommonTexture.hlsl"
#include "Ocean/OceanCommon.hlsl"

cbuffer FFTCB : register(b0)
{
    uint _stage;
    TextureParameter _sourceSRV;
    TextureParameter _targetUAV;
};

[numthreads(8,8,1)]
void mainHorizontal(uint3 id : SV_DispatchThreadID)
{
    const uint x = id.x;
    const uint y = id.y;

    const int m = 1u << _stage;

    // x가 그룹 내에서 어디에 위치하는지 계산 (상단인지 하단인지)
    const uint groupIdx = x / (2 * m);
    const uint localIdx = x % (2 * m);
    const bool isUpper = localIdx < m;

    /*
    *   각 텍셀이 하나의 파동이고 upper는 lower위치와, lower는 upper위치와 더하는 과정이고 그걸 상상해보자
    *   여기서 x, y는 월드의 x,y라고 생각해도 무방하다. 만약 x, y위치에서 upper는 그대로 구한다고 약속했을 때
    *   lower 정현파는 x, y위치로 보정해줘야 하는 작업이 필요해서 angle을 구한 후 위상을 x, y위치로 보정해준다
    */
    // 주파수 공간 Ht를 월드 공간으로 변경해주는 angle이기도 하다.
    uint j;
    float2 w;
    float angle = -PI2 * (localIdx % m) / (2.0 * m);
    sincos(angle, w.y, w.x);

    Texture2D<float4> Source = getTexture(_sourceSRV);
    RWTexture2D<float4> Target = getTextureRW(_targetUAV);

    if (isUpper) 
    {
        // 상단 노드일 때: 하단 노드(x + m)의 값을 가져옴
        j = x + m;
        float2 a = Source[uint2(x, y)].xy;  // 상단 값 (현재 thread의 값)
        float2 b = Source[uint2(j, y)].xy;  // 하단 값
        float2 t = ComplexMul(w, b);
        Target[uint2(x, y)] = float4(a + t, 0, 0);
    } 
    else 
    {
        // 하단 노드일 때: 상단 노드(x - m)의 값을 가져옴
        j = x - m;
        float2 a = Source[uint2(j, y)].xy;  // 상단 값
        float2 b = Source[uint2(x, y)].xy;  // 하단 값 (현재 thread의 값)
        float2 t = ComplexMul(w, b);
        Target[uint2(x, y)] = float4(a - t, 0, 0);
    }
}

[numthreads(8,8,1)]
void mainVertical(uint3 id : SV_DispatchThreadID)
{
    const uint x = id.x;
    const uint y = id.y;

    const int m = 1u << _stage;

    // x가 그룹 내에서 어디에 위치하는지 계산 (상단인지 하단인지)
    const uint groupIdx = y / (2 * m);
    const uint localIdx = y % (2 * m);
    const bool isUpper = localIdx < m;

    uint j;
    float2 w;
    const float angle = -PI2 * (localIdx % m) / (2.0 * m);
    sincos(angle, w.y, w.x);

    Texture2D<float4> Source = getTexture(_sourceSRV);
    RWTexture2D<float4> Target = getTextureRW(_targetUAV);

    if (isUpper) 
    {
        // 상단 노드일 때: 하단 노드(x + m)의 값을 가져옴
        j = y + m;
        float2 a = Source[uint2(x, y)].xy;
        float2 b = Source[uint2(x, j)].xy;

        // upper인 경우 lower를 회전 시킨 그대로를 더함
        float2 t = ComplexMul(w, b);
        Target[uint2(x, y)] = float4(a + t, 0, 0);
    } 
    else 
    {
        // 하단 노드일 때: 상단 노드(x - m)의 값을 가져옴
        j = y - m;
        float2 a = Source[uint2(x, j)].xy; // 상단 값
        float2 b = Source[uint2(x, y)].xy; // 내 값

        // lower인 경우 lower를 반대로 회전해서 upper와 더하면 upper에서 lower를 정방향 외전한 것과 완전히 동일한 값을 가질 수 있음
        float2 t = ComplexMul(w, b);
        Target[uint2(x, y)] = float4(a - t, 0, 0);
    }
}

#endif