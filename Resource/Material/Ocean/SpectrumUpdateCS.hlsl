#ifndef __DEFINE_SPECTRUMUPDATE_HLSL__
#define __DEFINE_SPECTRUMUPDATE_HLSL__

#include "CommonComplex.hlsl"
#include "CommonTexture.hlsl"
#include "Ocean/OceanCommon.hlsl"

[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const int x = id.x;
    const int y = id.y;

    const float2 k = float2((x - _N/2) * 2.0 * 3.14159 / _L, (y - _N/2) * 2.0 * 3.14159 / _L);

    const float kLen = length(k);
    const float w = sqrt(_g * kLen);

    Texture2D<float4> H0 = getTextureRW(_h0SRV);

    const float2 h0 = H0[id.xy];
    const float2 h0_conj = float2(h0.x, -h0.y);

    const float coswt = cos(w * _timeForOcean);
    const float sinwt = sin(w * _timeForOcean);

    const float2 exp_iwt = float2(coswt, sinwt);
    const float2 exp_neg = float2(coswt, -sinwt);

    const float2 h = complexMul(h0, exp_iwt) + complexMul(h0_conj, exp_neg);

    RWTexture2D<float4> Ht = getTextureRW(_htUAV);
    Ht[id.xy] = h;
}

#endif