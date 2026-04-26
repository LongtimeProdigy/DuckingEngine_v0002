#include "CommonComplex.hlsl"

RWTexture2D<float2> Data : register(u0);

cbuffer FFTCB : register(b0)
{
    uint stage;
    uint N;
}

[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint j = id.x;
    uint i = id.y;

    uint m = 1 << stage;
    uint k = j & (m - 1);
    uint pair = j ^ m;

    float angle = -2.0 * 3.141592 * k / (2 * m);
    float2 w = float2(cos(angle), sin(angle));

    float2 a = Data[int2(j, i)];
    float2 b = Data[int2(pair, i)];

    float2 t = complexMul(w, b);

    Data[int2(j, i)] = a + t;
    Data[int2(pair, i)] = a - t;
}
