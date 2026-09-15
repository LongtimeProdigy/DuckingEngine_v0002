#ifndef __DEFINE_FFTBUTTERFLY_HLSL__
#define __DEFINE_FFTBUTTERFLY_HLSL__

#include "CommonTexture.hlsl"
#include "Ocean/OceanCommon.hlsl"

cbuffer FinalizeCB : register(b0)
{
    TextureParameter _sourceSRV;
};

[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID)
{
    Texture2D<float4> FFTResult = getTexture(_sourceSRV);
    const float2 h = FFTResult[id.xy].xy;

    // FFT shift correction
    // 체스판처럼 -1, 1이 반복되면서 나온다
    /*
    *   FFT는 사실상 파동이 무한대로 퍼져있는 무한대의 텍스처라고 생각해야한다. 다만 우리는 N*N 크기이며 0, 0부터 파동이 시작한다고 보자
    *   여기서 문제는 0,0에서 파동이 시작된다는 것이다. 우리는 바다의 정중앙에서 파동이 퍼져나가길 바란다.
    *   (아마 0,0부터 다른 모서리((0, N), (N, 0), (N, N)) 부분까지 파동의 흐름이 저주파 -> 고주파로 이어질 것이며
    *   타일링 하면 티가 확연하게 나타날 수 있음)
    *   (0, 0 부분이 저주파가 모여있다는 건 FFT의 특성 때문인 것 같음. 아무튼 그래서 0,0이 pivot이면 0,0 위치가 저주파 높이만 모이게됨)
    *   그래서 Source Texture의 기준을 0,0에서 N/2, N/2로 맞추기 위해 아래 sign을 곱해준다
    */
    /*  어떻게 타일형식으로 sing을 곱하는 행위가 이미지를 N/2만큼 미루는 행동이 되는 걸까?
    *   공간에서 n을 n + N/2만큼 미루는 걸 생각해보자
    *       -- $$e^{i \frac{2\pi k (N/2)}{N}}$$ 라는 식으로 밀어야한다.
    *       -- N이 약분되고, 2가 약분된다.
    *       -- 결국 $e^{i \pi k}$만 남게된다.
    *       -- 결국 타일형식으로 1, -1이 반복되는 형태
    *           -- k == 0 --> 1 곱하기
    *           -- k == 1 --> -1 곱하기 ....
    *   결과. N/2만큼 회전한 파동은 결국 타일 형식으로 1, -1을 번갈아 곱하는 것과 같다.
    */
    const float sign = ((id.x + id.y) & 1) ? -1.0 : 1.0;
    float height = h.x * sign;

    // normalization
    // 텍스처(N*N)을 모두 더했으므로 N*N을 나눠서 normalize해줘야함. 
    // 허나 텍스처가 크면 그 값이 상당히 작게 normalize되기 떄문에 및에서 scaling해주어야함
    height /= (_N * _N);
    height *= _heightScale;

    RWTexture2D<float4> Height = getTextureRW(_heightUAV);
    Height[id.xy] = float4(height, 0, 0, 0);
}

#endif