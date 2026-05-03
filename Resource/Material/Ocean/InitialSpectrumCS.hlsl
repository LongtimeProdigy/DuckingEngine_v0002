#ifndef __DEFINE_INITIALSPECTRUM_HLSL__
#define __DEFINE_INITIALSPECTRUM_HLSL__

#include "CommonMath.hlsl"
#include "CommonTexture.hlsl"
#include "Ocean/OceanCommon.hlsl"

#define WindDependency (0.07)

float Phillips(float2 k)
{
    const float kLen = length(k);
    if (kLen < FLOAT_EPSILON) 
        return 0.0;

    const float k2 = kLen * kLen;
    const float k4 = k2 * k2;

    const float2 kNorm = k / kLen;
    const float kw = dot(kNorm, normalize(_windDir));

    // 기본 Phillips 공식
    float P = _A * (exp(-1.0 / (k2 * _L * _L)) / k4);
    // 바람 반대 방향 파도 제거 (kw < 0일 때 억제)
    if (kw < 0.0) P *= WindDependency;
    // kw * kw는 방향성을 부여하지만 위 조건문이 없으면 양방향으로 파도가 침
    P *= (kw * kw);

    // 단파장 억제 (High-frequency damping)
    const float l = _L * 0.1;
    P *= exp(-k2 * l * l);

    return P;
}

[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const int x = int(id.x);
    const int y = int(id.y);

    // 정수 좌표를 기반으로 주파수 k 계산
    // _N/2를 빼주는 이유는 주파수 영역의 중심을 맞추기 위함
    const float2 k = float2((x - int(_N / 2)) * PI2 / _length, (y - int(_N / 2)) * PI2 / _length);

    const float P = Phillips(k);
    const float2 gaussian = randGaussian(id.xy);

    // 최종 스펙트럼 값 계산
    const float2 value = gaussian * sqrt(P * 0.5);

    RWTexture2D<float4> H0 = getTextureRW(_h0UAV);
    H0[id.xy] = float4(value.x, value.y, 0, 0);
}

#endif