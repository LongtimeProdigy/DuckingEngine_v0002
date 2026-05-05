#ifndef __DEFINE_INITIALSPECTRUM_HLSL__
#define __DEFINE_INITIALSPECTRUM_HLSL__

#include "CommonMath.hlsl"
#include "CommonTexture.hlsl"
#include "Ocean/OceanCommon.hlsl"

#define WindDependency (0.07)

// 필립스 스펙트럼의 결과값 P(k)는 특정 파수(주파수) k를 가진 파동이 어느 정도의 에너지 밀도를 가지는지를 나타냅니다.
float Phillips(float2 k)
{
    ///////////////////////////////////////////////////////////// 기본 Phillips 공식
    const float kLen = length(k);
    if (kLen < FLOAT_EPSILON) 
        return 0.0;

    const float k2 = kLen * kLen;

    /*  오웬 필립스의 차원 분석에 의해 유도된 항입니다. 
    *   파동이 에너지를 더 이상 흡수하지 못하고 부서지는 '포화 상태'에서, 에너지는 파수 k의 4제곱에 반비례하여 줄어듭니다. 
    *   즉, 긴 파장(작은 k)에 압도적인 에너지가 쏠리게 합니다.
    */
    const float k4 = k2 * k2;

    const float2 kNorm = k / kLen;
    const float kw = dot(kNorm, normalize(_windDir));

    /*  exp(-1/(kL)^2) (차단 항): L은 풍속에 의해 결정되는 최대 파장입니다. 
    *   k가 매우 작아지면(파장이 L보다 커지면) 이 지수 항이 0으로 급격히 수렴하여, 물리적으로 존재할 수 없는 무한한 크기의 파도를 제거합니다.
    */
    const float resistance = exp(-1.0 / (k2 * _L * _L));
    float P = _A * (resistance / k4);

    ///////////////////////////////////////////////////////////// 바람 방향으로 강화
    // 바람 반대 방향 일 때 에너지를 WindDependency%만큼으로 줄여 파도를 억제
    if (kw < 0.0) P *= WindDependency;

    // 바람 방향의 파도가 강해지게함. 단 kw * kw는 항상 양수라서 반대방향의 파도도 강해지는 현상이 있어서 바로 위에서 에너지의 %를 극단적으로 줄임
    P *= (kw * kw);

    ///////////////////////////////////////////////////////////// 단파장 억제 (High-frequency damping)
    /*  수치 해석적으로 아주 작은 k 값들은 화면에서 "지직거리는" 에러(Aliasing)처럼 보일 수 있습니다. 
    *   이를 방지하기 위해 l = L * 0.1이라는 임계치를 설정하여, 아주 미세한 물결들을 부드럽게 지워버리는 시각적 트릭입니다.
    */
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
    // PI2 / _length를 곱하는 이유는 텍스처 중앙으로부터의 텍셀위치 k(또는 x, y)를 월드 길이의 주파수로 바꾸는 작업
    const float2 k = float2((x - int(_N / 2)) * PI2 / _length, (y - int(_N / 2)) * PI2 / _length);

    const float P = Phillips(k); // P값은 어떤 주파수 k가 여러 요인(wind 등)에 의해 계산된 통계적으로 가질 수 있는 평균적인 에너지 밀도
    const float2 gaussian = randGaussian(id.xy); // 무작위성 부여를 위하여 gaussian 생성

    // 최종 스펙트럼 값 계산
    /*  에너지(P)는 진폭(A)의 제곱에 비례하므로, 진폭을 얻기 위해 루트(sqrt)를 씌웁니다.
    *   복소수 에너지 분산을 위해 각 항에 0.5를 곱해야함
    *   주파수(k)가 가지는 평균 에너지 정도가 P라는 것이지 항상 그러면 패턴이 보이기 때문에 에너지를 랜덤으로 줄이기 위해 gaussian을 곱함
    *
    *   gaussian을 곱한 결과 복소수 a + bi가 나온다고 가정하면
    *   복소수 크기(root(a^2 + b^2))는 진폭을 여전히 나타낼 수 있음
    *   복소스 atan2(b, a)는 랜덤으로 생성된 결과이므로 랜덤한 주파수의 시작위치를 나타낼 수 있음
    */
    const float amplitude = sqrt(P * 0.5);
    const float2 value = gaussian * amplitude;      // 복소평면에서 길이가 진폭, 각도가 랜덤한 시작위치를 나타내는 복소수(벡터)를 만듦. 이후 UpdateCS에서 해당 복소수(벡터)를 회전시키기 위해서 여기서 미리 복소수 형태로 변환함
    RWTexture2D<float4> H0 = getTextureRW(_h0UAV);
    H0[id.xy] = float4(value, 0, 0);
}

#endif