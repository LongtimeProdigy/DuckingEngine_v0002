#define ATMOSPHERE_FORCE_LUT
#include "Atmosphere/AtmosphereTransmittance.hlsl"
#include "Atmosphere/AtmosphereSurfaceLighting.hlsl"
#include "CommonMathConstant.hlsl"

cbuffer SurfaceBounceOutputConstants : register(b2)
{
    uint _surfaceBounceOutputUav;
};

// 카메라 기준 공간 위치
float3 atmosphereSurfacePoint(float3 uvw)
{
    return _surfaceOrigin.xyz + _surfaceRight.xyz * ((2.0f * uvw.x - 1.0f) * _surfaceRight.w)
        + _surfaceUp.xyz * ((1.0f - 2.0f * uvw.y) * _surfaceUp.w) + _surfaceForward.xyz * (uvw.z * _surfaceForward.w);
}

// 3점 Gauss–Legendre 적분입니다. 임의로 고른 위치와 가중치가 아니라, 적은 샘플로 적분을 근사하도록 정해진 조합
// column은 한 지점의 밀도가 아니라 경로 전체에 걸쳐 누적한 성분별 밀도 적분값
// 투과율 반환
float3 surfaceSegmentTransmission(float3 a, float3 b)
{
    const float3 dir = b - a;
    const float distance = length(dir);
    const float4 column = distance * (
        atmosphereDensity(atmosphereHeight(a + dir * 0.1127016654f)) * (5.0f / 18.0f)
        + atmosphereDensity(atmosphereHeight(a + dir * 0.5f)) * (8.0f / 18.0f)
        + atmosphereDensity(atmosphereHeight(a + dir * 0.8872983346f)) * (5.0f / 18.0f));
    return exp(-atmosphereOpticalExtinction(column));
}

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= SURFACE_BOUNCE_GRID * SURFACE_BOUNCE_GRID || id.y >= SURFACE_BOUNCE_GRID) 
        return;

    // 지형이나 물체가 반사한 태양빛이 주변 대기에 얼마나 도달하는지 미리 계산
    // 현재 구현의 주요 한계는 Q → P 사이에 다른 지형이나 물체가 가로막는지 검사하지 않는다는 점
    // 방향 정보를 저장하지 않으므로, 추후 Atmosphere에서 사용하는 이 반사광의 산란에는 모든 방향으로 동일하게 퍼진다는 **등방성 근사 1 / (4π)**를 사용

    const uint3 cell = uint3(id.x % SURFACE_BOUNCE_GRID, id.y, id.x / SURFACE_BOUNCE_GRID);
    const float3 uvw = (float3(cell) + 0.5f) / SURFACE_BOUNCE_GRID;
    const float3 p = atmosphereSurfacePoint(uvw);
    //const float angle = radians(_sunDegree);
    //const float3 sunDir = float3(0, sin(angle), cos(angle));
    const float3 sunDir = -normalize(_surfaceForward.xyz);
    GPUASSERT(_surfaceBounceSamples <= 64);
    const float scale = 2.0f * max(_surfaceRight.w, _surfaceUp.w) / SURFACE_BOUNCE_GRID;    // 긴 축 기준 격자 셀 한 칸의 크기입니다.
    GPUASSERT(scale >= 0.01);
    GPUASSERT(_surfaceMapSize >= 1);
    // 캡처 평면에서 텍셀 하나가 차지하는 면적 : 캡처 전체 면적 = (가로 반폭 × 2) × (세로 반폭 × 2) -> 텍셀 하나 면적 = 캡처 전체 면적 / 텍셀 개수
    float3 bounce = 0.0f;
    if (_surfaceMapEnabled != 0u)
    {
        [loop] 
        for (uint i = 0u; i < _surfaceBounceSamples; ++i)
        {
            const float u = (float(i) + 0.5f) / _surfaceBounceSamples;
            GPUASSERT((1.0f - u) > FLOAT_EPSILON);

            // 2.3283064365386963e-10: 1 / 2³²입니다. 이를 곱해 0~1 값으로 바꾸고, 2π를 곱해 각도로 바꿉니다.
            // 의도: 연속된 샘플이 한쪽 각도에 몰리지 않도록 합니다. 난수를 매번 생성하는 방식은 아닙니다.
            const float phi = 2.0f * PI * (float(reversebits(i)) * 2.3283064365386963e-10f);
            // u = r²/(scale²+r²)를 r에 대하여 푼 것. 가까울 수록 많이 멀수록 덜 샘플링하는 식 (단 최대가 제한은 없음)
            const float radius = scale * sqrt(u / (1.0f - u));
            const float2 sampleUV = uvw.xy + float2(cos(phi), sin(phi)) * radius / (2.0f * float2(_surfaceRight.w, _surfaceUp.w));
            if (any(sampleUV < 0.0f) || any(sampleUV >= 1.0f)) 
                continue;

            GPUASSERT(all(uint2(sampleUV * _surfaceMapSize) <= _surfaceMapSize - 1u));  // 위 if에서 걸러졌어야함
            const uint2 pixel = uint2(sampleUV * _surfaceMapSize);
            const float4 albedo = getTexture(_surfaceAlbedoIndex).Load(int3(pixel, 0));
            if (albedo.a <= 0.0f) 
                continue;

            const float3 normalRaw = getTexture(_surfaceNormalIndex).Load(int3(pixel, 0)).xyz;
            const float3 normal = normalRaw * rsqrt(max(dot(normalRaw, normalRaw), 1e-12f));
            if (dot(normal, sunDir) <= FLOAT_EPSILON)   // 직접 햇빛을 받아 확산 반사할 수 있는 표면만 조사
                continue;

            const float depth = getTexture(_surfaceDepthIndex).Load(int3(pixel, 0)).x;
            const float metallic = saturate(getTexture(_surfaceMaterialIndex).Load(int3(pixel, 0)).x);

            //const float sunCosine = saturate(dot(normal, sunDir));
            //const float3 captureForward = normalize(_surfaceForward.xyz); // -sunDir;
            //const float projectedCosine = abs(dot(normal, captureForward));
            //const float surfaceAreaJacobian = 1.0f / max(projectedCosine, FLOAT_EPSILON);
            // 원래 수식은 sunCosine과 surfaceAreaJacobian을 곱해야 하지만 sunDir은 captureForward와 180방향의 벡터이므로 cos값은 같아서 상쇄된다

            const float3 q = atmosphereSurfacePoint(float3((float2(pixel) + 0.5f) / _surfaceMapSize, depth));
            const float3 dir = p - q;
            const float distanceSQ = dot(dir, dir);
            GPUASSERT(distanceSQ > 0.0f);
            const float cosine = saturate(dot(normal, dir * rsqrt(distanceSQ)));
            // radius인 원의 면적에 대한 u의 식으로 변형하면 u = r² / (s² + r2)이고, u가 0~1까지 100% 확률이므로
            // 반지금 r 이내에 샘플이 들어갈 확률 cdf(r) = r² / (s² + r2)이 됨 (실제로 r을 s로 두면 50%)
            // cdf를 미분하면 pdf가 되고 r² = A/PI를 이용하면 아래식을 유도할 수 있음
            const float inversePdf = PI * scale * scale / ((1.0f - u) * (1.0f - u));
            const float3 diffuseFlux = 
                atmosphereSunIrradiance()           // 도달한 태양 조사량
                * getSunTransmittance(q, sunDir)    // 태양에서 q까지의 투과율
                //* sunCosine                         // q에 들어가는 태양 조사량을 각도에 영향을 받게
                * (albedo.rgb / PI)                 // q에서 Lambert 확산 반사
                * (1.0f - metallic);                // 금속의 확산 반사 기여를 감소
            bounce += diffuseFlux 
                // * V(p, q)                        // q에서 p사이의 가시성 검사. 0 또는 1. 비용이 크므로 사용안함
                * surfaceSegmentTransmission(q, p)  // q → p 사이의 대기 감쇠
                // dA는 normal인 평면이고 실제 샘플링하는 표면은 projectedA이기때문에 둘 사이의 면적을 변환 surfaceAreaJacobian를 곱함
                //* surfaceAreaJacobian
                // dw = cos / r² * dA이기 때문에, cos / r²가 추가됨 : 실제 표면이 P에서 얼마나 크게 보이는가
                * (cosine * inversePdf              // 표면이 p를 향할수록, 가까울수록 기여가 커집니다. (cosine / distanceSQ)항
                / (distanceSQ * _surfaceBounceSamples)); // 일부 표면을 불균등하게 선택한 것을 보정해 전체 표면 적분을 근사 (inversePdf / _surfaceBounceSamples) 항
        }
    }

    // 상한을 제한. 샘플 개수가 적을 때 가까운 밝은 표면 하나가 너무 큰 기여를 만드는 현상을 완화하기 위한 안전장치
    // 4는 이상적인 Lambert 표면의 최대 radiance E/π를 구 전체 입체각 4π에 걸쳐 적분하면 4E가 된다는 데서 해석할 수 있습니다
    // 어떤 위치(P)에 모든 방향(4PI)에 들어오는 모든 빛의 양은 4.0f * atmosphereSunIrradiance()이기 때문에 해당 값이 최대값이다
    bounce = min(max(bounce, 0.0f), 4.0f * atmosphereSunIrradiance());

    // P로 들어오는 표면 반사광을 방향에 대해 합친 값의 근사치
    const RWTexture2D<float4> output = getTextureRW(_surfaceBounceOutputUav);
    output[id.xy] = float4(bounce, 1);
}
