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

float RandomValue(inout uint state) 
{
    state *= (state + 195439) * (state + 124395) * (state + 845921);
    return state / 4294967295.0;
}

// // Z축이 Up인 반구 기준으로 샘플링
// float3 consineHemisphereSampling(inout uint state) 
// {
//     float u1 = RandomValue(state);
//     float u2 = RandomValue(state);

//     float r = sqrt(u1);
//     float theta = PI2 * u2;

//     float x = r * cos(theta);
//     float y = r * sin(theta);
//     float z = sqrt(1.0 - u1);

//     return float3(x, z, y);
// }
// // Z축이 Up인 반구 기준으로 샘플링
// float3 uniformHemisphereSampling(inout uint state) {
//     float u1 = RandomValue(state);
//     float u2 = RandomValue(state);

//     float z = u1;
//     float r = sqrt(max(0.0, 1.0 - z * z));
//     float phi = 2.0 * PI * u2;

//     return float3(r * cos(phi), r * sin(phi), z);
// }
float3 RandomHemisphereDirection(uniform bool useCos, float3 normal, inout uint state) {
    float3 N = normalize(normal);

    // N을 로컬 Z축으로 하는 직교 기저.
    float s = 2.0 * step(0.0, N.z) - 1.0;       // s는 항상 -1 또는 +1이므로 N.z == 0에서도 안전합니다.
    float a = -1.0 / (s + N.z);                 // s의 특성 때문에 a는 항상 양수
    float b = N.x * N.y * a;
    float3 T = float3(1.0 + s * N.x * N.x * a, s * b, -s * N.x);
    float3 B = float3(b, s + N.y * N.y * a, -N.y);

    // +Z가 법선인 반구 기준 샘플링
    float u1 = RandomValue(state);
    float u2 = RandomValue(state);
    float sinPhi, cosPhi;
    sincos(PI2 * u2, sinPhi, cosPhi);
    
    float z = useCos ? sqrt(1.0 - u1) : u1;
    float r = sqrt(max(0.0, 1.0 - z * z));
    float x = (r * cosPhi);
    float y = (r * sinPhi);

    // 로컬 +Z를 표면 법선 N으로 변환합니다.
    return x * T + y * B + z * N;
}

#endif