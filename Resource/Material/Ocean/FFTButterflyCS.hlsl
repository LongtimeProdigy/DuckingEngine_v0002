#ifndef __DEFINE_FFTBUTTERFLY_HLSL__
#define __DEFINE_FFTBUTTERFLY_HLSL__

#include "CommonComplex.hlsl"
#include "CommonTexture.hlsl"
#include "Ocean/OceanCommon.hlsl"

[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint j = id.x;
    const uint i = id.y;

    const uint m = 1u << _stages;
    const uint k = j & (m - 1);
    const uint pair = j ^ m;

    const float angle = -2.0 * 3.141592 * k / (2 * m);
    const float2 w = float2(cos(angle), sin(angle));

    RWTexture2D<float4> Data = getTextureRW(_htUAV);

    const float2 a = Data[int2(j, i)].xy;
    const float2 b = Data[int2(pair, i)].xy;

    const float2 t = ComplexMul(w, b);

    const float2 value1 = a + t;
    const float2 value2 = a - t;

    Data[int2(j, i)] = float4(value1, 0, 0);
    Data[int2(pair, i)] = float4(value2, 0, 0);
}

#endif