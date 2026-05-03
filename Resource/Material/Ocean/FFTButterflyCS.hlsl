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
        float2 a = Source[uint2(x, y)].xy;
        float2 b = Source[uint2(j, y)].xy;
        float2 t = ComplexMul(w, b);
        Target[uint2(x, y)] = float4(a + t, 0, 0);
    } 
    else 
    {
        // 하단 노드일 때: 상단 노드(x - m)의 값을 가져옴
        j = x - m;
        float2 a = Source[uint2(j, y)].xy; // 상단 값
        float2 b = Source[uint2(x, y)].xy; // 내 값
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
        float2 t = ComplexMul(w, b);
        Target[uint2(x, y)] = float4(a + t, 0, 0);
    } 
    else 
    {
        // 하단 노드일 때: 상단 노드(x - m)의 값을 가져옴
        j = y - m;
        float2 a = Source[uint2(x, j)].xy; // 상단 값
        float2 b = Source[uint2(x, y)].xy; // 내 값
        float2 t = ComplexMul(w, b);
        Target[uint2(x, y)] = float4(a - t, 0, 0);
    }
}

#endif