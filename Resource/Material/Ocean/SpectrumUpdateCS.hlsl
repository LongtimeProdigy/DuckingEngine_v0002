#include "CommonComplex.hlsl"

Texture2D<float2> H0 : register(t0);
RWTexture2D<float2> Ht : register(u0);

cbuffer TimeCB : register(b0)
{
    float time;
    float g;
    uint N;
    float L;
}

[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID)
{
    int x = id.x;
    int y = id.y;

    float2 k = float2(
        (x - N/2) * 2.0 * 3.14159 / L,
        (y - N/2) * 2.0 * 3.14159 / L
    );

    float kLen = length(k);
    float w = sqrt(g * kLen);

    float2 h0 = H0[id.xy];
    float2 h0_conj = float2(h0.x, -h0.y);

    float coswt = cos(w * time);
    float sinwt = sin(w * time);

    float2 exp_iwt = float2(coswt, sinwt);
    float2 exp_neg = float2(coswt, -sinwt);

    float2 h = complexMul(h0, exp_iwt) + complexMul(h0_conj, exp_neg);

    Ht[id.xy] = h;
}
