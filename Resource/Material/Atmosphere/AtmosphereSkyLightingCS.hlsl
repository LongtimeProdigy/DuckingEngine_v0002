#define ATMOSPHERE_FORCE_LUT
#include "Atmosphere/AtmosphereTransmittance.hlsl"
#include "Atmosphere/AtmosphereSkyLighting.hlsl"
#include "Atmosphere/AtmosphereSurfaceLighting.hlsl"
#include "Atmosphere/AtmosphereMultipleScattering.hlsl"

cbuffer SkyLightingOutputConstants : register(b2)
{
    uint _skyOutputUav;
};

float3 skyDirection(float2 uv)
{
    const float phi = uv.x * 2.0f * PI;
    const float theta = uv.y * PI;
    return float3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
}

[numthreads(8, 8, 1)]
void CSRadiance(uint3 id : SV_DispatchThreadID)
{
    // 현재 카메라 위치에서 모든 방향을 바라봤을 때, 대기로부터 들어오는 빛을 계산해 환경 맵에 저장하는 함수 (카메라 위치가 물리적으로 특별해서가 아니라, 주변 장면을 대표하는 환경광 프로브 위치로 선택한 것)
    // 태양 직접광의 산란 + 지표에서 반사된 빛의 산란 + 대기에서 이미 산란된 빛의 추가 산란(MS) + 구간 내부 감쇠까지 고려하는 적분 가중치(weight)

    if (id.x >= SKY_WIDTH || id.y >= SKY_HEIGHT) 
        return;

    const float3 dir = skyDirection((float2(id.xy) + 0.5f) / float2(SKY_WIDTH, SKY_HEIGHT));
    const float2 hit = raySphere(_planetCentre, _atmosphereRadius, 0.0f, dir);
    const float3 betaR = atmosphereRayleighScattering();
    const float3 betaM = atmosphereMieScattering();
    const float angle = radians(_sunDegree);
    const float3 sunDir = float3(0, sin(angle), cos(angle));
    const float mu = dot(dir, sunDir);
    const float phaseR = atmosphereRayleighPhase(mu), phaseM = atmosphereMiePhase(mu);
    float3 T = 1.0f, radiance = 0.0f;

    // 대기가 없으면 누적할 게 없음
    if (hit.y > 0.0f && _scatteringScale > 0.0f && _sunIrradiance > 0.0f)
    {
        [loop]
        for (uint i = 0u; i < 64u; ++i)
        {
            const float u0 = float(i) / 64.0f, u1 = float(i + 1u) / 64.0f;
            const float a = hit.y * u0 * u0 * u0, b = hit.y * u1 * u1 * u1;
            const float width = b - a;
            const float3 p = dir * (hit.x + 0.5f * (a + b));
            const float4 density = atmosphereDensity(atmosphereHeight(p));
            const float3 sigma = atmosphereOpticalExtinction(density);
            GPUASSERT(all(sigma >= FLOAT_EPSILON_MIN));
            const float3 tau = sigma * width;
            const float3 transmittance = exp(-tau);
            const float3 weight = float3(       // start -> end를 지나오면서 누적되는 투과율 (구간 모든 점에서 빛이 발생되고 해당 빛들이 각 구간을 지나 start에 도달할때 전체 투과율)
                tau.x < FLOAT_EPSILON ? width * (1.0f - tau.x * 0.5f) : (1.0f - transmittance.x) / sigma.x,
                tau.y < FLOAT_EPSILON ? width * (1.0f - tau.y * 0.5f) : (1.0f - transmittance.y) / sigma.y,
                tau.z < FLOAT_EPSILON ? width * (1.0f - tau.z * 0.5f) : (1.0f - transmittance.z) / sigma.z);

            GPUASSERT(_densityReferenceRadius + atmosphereHeight(p) > 0);
            const float radius = _densityReferenceRadius + atmosphereHeight(p);
            GPUASSERT(abs(dot(p - _planetCentre, sunDir)) <= radius, float4(dot(p - _planetCentre, sunDir), length(p - _planetCentre), atmosphereHeight(p), length(p)));
            const float3 source = 
                atmosphereSunIrradiance()               // 태양의 밝기
                * getSunTransmittance(p, sunDir)        // 태양에서 p까지의 투과율
                * atmosphereSurfaceSunVisibility(p)     // p에서 태양방향으로 가려지는정도 (0 or 1)
                * (betaR * density.x * phaseR + betaM * density.y * phaseM) // 위상함수 ... 여기까지가 direct
                + atmosphereSurfaceBounce(p)            // 태양의 표면 반사광
                * (betaR * density.x + betaM * density.y) / (4.0f * PI) // SurfaceBounce는 방향별 정보가 없기 떄문에 등방성 위상함수를 사용(등방성 위상함수(4/PI))
                + atmosphereMultipleIncident(atmosphereHeight(p), dot(p - _planetCentre, sunDir)) * (betaR * density.x + betaM * density.y);    // 대기에서 산란된 빛의 양
            radiance += T * source * weight;
            T *= transmittance;

            if (all(T < 1e-6f))
                break;
        }
    }

    const RWTexture2D<float4> output = getTextureRW(_skyOutputUav);
    output[id.xy] = float4(radiance, 1);
}

static const uint SKY_BANDS = 6u; // 0..4: GGX roughness, 5: diffuse irradiance.

[numthreads(8, 8, 1)]
void CSFilter(uint3 id : SV_DispatchThreadID)
{
    // CSRadiance가 만든 하늘을, 물체의 재질에 맞게 사용할 수 있도록 미리 가공하는 함수
    // - 매끈한 물체: 특정 반사 방향의 하늘이 선명하게 보입니다.
    // - 거친 물체: 반사 방향 주변의 여러 하늘 방향이 섞여 보입니다.
    // - 확산 표면: 표면 위쪽 반구에서 들어오는 빛을 넓게 받습니다.
    // 0: 원본 하늘 — roughness 0, 1: roughness 0.25의 반사용 하늘, 2: roughness 0.5의 반사용 하늘, 3: roughness 0.75의 반사용 하늘, 4: roughness 1.0의 반사용 하늘, 5: 확산 조명용 irradiance,     
    // (카메라 위치가 물리적으로 특별해서가 아니라, 주변 장면을 대표하는 환경광 프로브 위치로 선택한 것)

    if (id.x >= SKY_WIDTH || id.y >= SKY_HEIGHT * SKY_BANDS) 
        return;

    const uint band = id.y / SKY_HEIGHT;
    const float3 N = skyDirection((float2(id.x, id.y % SKY_HEIGHT) + 0.5f) / float2(SKY_WIDTH, SKY_HEIGHT));
    const float3 up = abs(N.y) < 0.99f ? float3(0, 1, 0) : float3(1, 0, 0);
    const float3 tangent = normalize(cross(up, N)), bitangent = cross(N, tangent);
    float3 sum = 0.0f;
    float weights = 0.0f;

    // 완전히 매끈한 반사는 주변 방향을 넓게 섞을 필요가 없으므로, 해당 방향의 원본 하늘을 저장
    if (band == 0u) 
    { 
        sum = sampleSkyBand(_skyRadianceIndex, N, 0u); weights = 1.0f; 
    }
    // 64개 방향의 하늘을 읽고 혼합
    else
    {
        [loop] 
        for (uint i = 0u; i < 64u; ++i)
        {
            // xi.x: 기준 방향에서 얼마나 기울어질지 결정.
            // xi.y: 기준 방향 주위를 도는 방위각(phi)
            const float2 xi = float2((float(i) + 0.5f) / 64.0f, radicalInverse(i));
            const float phi = 2.0f * PI * xi.y;
            float3 L;

            // 표면이 받는 하늘빛을 구함
            if (band == 5u)
            {
                // 코사인 가중 반구 샘플링 (xi.x를 sin²이라고 생각하면됨)
                const float sinTheta = sqrt(xi.x), cosTheta = sqrt(1.0f - xi.x);
                L = tangent * (cos(phi) * sinTheta) + bitangent * (sin(phi) * sinTheta) + N * cosTheta;
            }
            else
            {
                const float roughness = float(band) / 4.0f;
                // 거칠기를 GGX 분포의 폭으로 바꿉니다
                const float a2 = pow(roughness, 4.0f);

                // θ는 N과 H 사이의 각도입니다. 이 식은 균등한 샘플 xi.x를 GGX 분포를 따르는 기울기로 변환합니다. 정확히는 D(H) × cosθ에 비례하도록 법선을 샘플링합니다.
                const float cosTheta = sqrt((1.0f - xi.x) / (1.0f + (a2 - 1.0f) * xi.x));
                const float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

                // 거친 표면을 구성하는 작은 거울 면들의 법선, 즉 **미세면 법선(microfacet normal)**을 샘플링
                const float3 H = tangent * (cos(phi) * sinTheta) + bitangent * (sin(phi) * sinTheta) + N * cosTheta;

                // 이 프리필터는 관찰 방향 V = N을 가정합니다. 따라서 -N을 미세면 법선 H에 대해 반사한 방향 L의 하늘빛을 읽습니다. 여러 H의 결과를 섞으면 거친 반사가 됩니다.
                L = reflect(-N, H);
            }

            // band가 5인경우
            // PDF = \[p(\omega)=\frac{\cos\theta}{\pi}\]
            // 몬테카를로: \[E\approx\frac1K\sum_{k=1}^{K}\frac{L_{\mathrm{sky}}(\omega_k)\cos\theta_k}{p(\omega_k)}=\frac{\pi}{K}\sum_{k=1}^{K}L_{\mathrm{sky}}(\omega_k)\]
            // 따라서 cos이 상쇄되어 여기서는 cos을 빼고 64개 샘플의 평균에 마지막으로 \(\pi\)를 곱하면 됩니다. (마지막 output에 넣을때 PI를 곱해줌)
            const float weight = band == 5u ? 1.0f : saturate(dot(N, L));
            sum += sampleSkyBand(_skyRadianceIndex, L, 0u) * weight;
            weights += weight;
        }
    }

    GPUASSERT(weights > FLOAT_EPSILON);

    const RWTexture2D<float4> output = getTextureRW(_skyOutputUav);
    output[id.xy] = float4((sum / weights) * (band == 5u ? PI : 1.0f), 1);
}
