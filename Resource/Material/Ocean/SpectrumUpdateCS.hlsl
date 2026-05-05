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

    const float2 k = float2((x - N / 2) * (2.0 * PI / _length), (y - N / 2) * (2.0 * PI / _length));
    const float kLen = length(k);

    // 분산 관계 (Deep Water 모드)
    /*  w는 유체역학의 선형 파동 이론에서 도출된 결과로 밑에서 오일러 공식으로 파동을 회전시킬때 각속도로 사용될 변수다.
    *   참고로 중력이 강할 수록 파도는 더 요동치려는 성향이 있다. -> 각속도가 빠르다.
    *   주파수가 빠르다(k 길이가 크다) -> 당연히 각속도도 빠르다
    */
    const float w = sqrt(_g * kLen);
    // 오일러 공식을 이용해서 복소평면에서 시간 변화량 계산 (아직 회전하는 게 아니고 quaternion같은 걸 구했다고 생각하면됨. 곱해지면 회전함)
    float sinwt, coswt;
    sincos(w * _timeForOcean, sinwt, coswt);

    Texture2D<float4> H0_Tex = getTexture(_h0SRV);
    const float2 h0 = H0_Tex[id.xy].xy;

    // 반대편 주파수 -k의 인덱스 계산 (N - id)
    const uint2 targetIdx = (uint2(N, N) - id.xy);// % uint2(N, N);
    const float2 h0_minus_k = H0_Tex[targetIdx].xy;
    // -k 주파수의 켤레 복소수
    const float2 h0_minus_k_conj = float2(h0_minus_k.x, -h0_minus_k.y);

    const float2 exp_iwt = float2(coswt, sinwt);        // 구한 회전량을 복소수로 만들기, H0에 곱하면 복소평면에서 H0를 회전시킴
    const float2 exp_neg_iwt = float2(coswt, -sinwt);   // 구한 회전량을 반대로 회전시키는 복소수를 만듦

    // 최종 H(t) 계산: h0(k)*exp(iwt) + h0^*(-k)*exp(-iwt)
    // h0^*(-k)*exp(-iwt)는 IFFT수행할 -k 위치와 더해지는 때에 복소수가 사라지게 하기 위해서 더해줌
    const float2 h_t = ComplexMul(h0, exp_iwt) + ComplexMul(h0_minus_k_conj, exp_neg_iwt);

    RWTexture2D<float4> Ht = getTextureRW(_htUAV);
    Ht[id.xy] = float4(h_t, 0, 0);
}

#endif