#ifndef __DEFINE_SPECTRUMUPDATE_HLSL__
#define __DEFINE_SPECTRUMUPDATE_HLSL__

#include "CommonMath.hlsl"
#include "CommonComplex.hlsl"
#include "CommonTexture.hlsl"
#include "Ocean/OceanCommon.hlsl"

[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const int N = (int)_N;
    const int x = (int)id.x;
    const int y = (int)id.y;

    // 1. 주파수 k 계산
    const float2 k = float2((x - N / 2) * (2.0 * PI / _length), (y - N / 2) * (2.0 * PI / _length));
    const float kLen = length(k);

    // 2. 분산 관계 (Deep Water 모드)
    const float w = sqrt(_g * kLen);

    // 3. 텍스처 데이터 가져오기
    Texture2D<float4> H0_Tex = getTexture(_h0SRV);
    const float2 h0 = H0_Tex[id.xy].xy;
    const float2 h0_conj = float2(h0.x, -h0.y);

    // 반대편 주파수 -k의 인덱스 계산 (N - id)
    // 인덱스가 범위를 벗어나지 않도록 % 처리를 하거나 대칭 축을 정확히 잡아야 합니다.
    const uint2 targetIdx = (uint2(N, N) - id.xy) % uint2(N, N);
    const float2 h0_minus_k = H0_Tex[targetIdx].xy;
    // -k 주파수의 켤레 복소수
    float2 h0_minus_k_conj = float2(h0_minus_k.x, -h0_minus_k.y);

    // 4. 오일러 공식을 이용한 시간 변화량 계산
    float sinwt, coswt;
    sincos(w * _timeForOcean, sinwt, coswt);

    const float2 exp_iwt = float2(coswt, sinwt);
    const float2 exp_neg_iwt = float2(coswt, -sinwt);

    // 5. 최종 H(t) 계산: h0(k)*exp(iwt) + h0*(-k)*exp(-iwt)
    const float2 h_t = ComplexMul(h0, exp_iwt) + ComplexMul(h0_minus_k_conj, exp_neg_iwt);

    RWTexture2D<float4> Ht = getTextureRW(_htUAV);
    Ht[id.xy] = float4(h_t, 0, 0);
}

#endif