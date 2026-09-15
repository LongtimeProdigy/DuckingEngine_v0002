#ifndef __DEFINE_COMMONMATH_HLSL__
#define __DEFINE_COMMONMATH_HLSL__

//based on google's omni-directional stereo rendering thread
#define FLOAT_EPSILON (1e-6)
#define FLOAT_MAX (3.402823466e+38F)

#define PI 3.14159265358979
#define PI2 (2 * PI)


// // 예전부터 가장 많이 쓰인 방식으로, 속도는 빠르지만 패턴이 반복되는 단점이 있습니다. (오션 시뮬레이션의 초기값 정도로 쓰기엔 적당합니다.)
// float rand(float2 uv)
// {
//     return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453123);
// }

// // 비트 연산을 사용하여 Sine 방식보다 훨씬 균일한 분포를 보여줍니다. 추천하는 방식입니다.
// float rand(uint2 pixel)
// {
//     // PCG Hash의 간단한 버전
//     pixel = pixel * 1103515245U + 12345U;
//     uint h32 = ((pixel.x ^ (pixel.y >> 3U)) * 1103515245U);
//     uint h = h32 ^ (h32 >> 16U);
//     return float(h) / 4294967295.0; // 0.0 ~ 1.0 범위로 변환
// }

// // 오션 시뮬레이션에서는 단순 0~1 난수보다 가우시안(정규) 분포를 따르는 난수가 훨씬 자연스럽습니다. 위에서 만든 기본 rand를 이용해 만들 수 있습니다.
// // 두 개의 균등 분포 난수를 받아 두 개의 가우시안 난수(float2)를 반환
// float2 randGaussian(uint2 id)
// {
// #if 0   // claude
//     float u1 = rand(id);          // 0~1 난수 1
//     float u2 = rand(id + 100);    // 0~1 난수 2 (시드 다르게)

//     float r = sqrt(-2.0 * log(max(u1, 1e-6)));
//     float theta = PI2 * u2;

//     return float2(r * cos(theta), r * sin(theta));
// #else   // gemini
//     float r1 = max(rand(float2(id.x, id.y)), FLOAT_EPSILON);
//     float r2 = max(rand(float2(id.y, id.x)), FLOAT_EPSILON);
    
//     float theta = PI2 * r2;
//     float rho = sqrt(-2.0 * log(r1));
    
//     return float2(rho * cos(theta), rho * sin(theta));
// #endif
// }

// 1. 기초가 되는 균등 난수 생성기 (Hash function)
float2 hash(float2 p)
{
    p = float2(dot(p, float2(127.1, 311.7)), dot(p, float2(269.5, 183.3)));
    return frac(sin(p) * 43758.5453123);
}

// 2. Box-Muller Transform을 이용한 가우스 난수 생성
float2 randGaussian(float2 seed)
{
    float2 u = hash(seed);
    
    // u.x와 u.y는 (0, 1] 범위여야 하므로 아주 작은 값으로 보정
    u = max(float2(FLOAT_EPSILON, FLOAT_EPSILON), u);

    float r = sqrt(-2.0 * log(u.x));
    float theta = PI2 * u.y;

    // 서로 독립적인 두 개의 가우스 난수 반환
    return float2(r * cos(theta), r * sin(theta));
}

#endif