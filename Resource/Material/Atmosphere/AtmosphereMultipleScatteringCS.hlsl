#ifndef ATMOSPHERE_MULTIPLESCATTERING_CS_HLSL
#define ATMOSPHERE_MULTIPLESCATTERING_CS_HLSL

#define ATMOSPHERE_FORCE_LUT

#include "Atmosphere/AtmosphereMultipleScattering.hlsl"

#define MULTIPLESCATTERING_COUNT 32u

cbuffer MultipleScatteringOutputConstants : register(b2)
{
    uint _multipleOutputUav;
    uint _multipleInputSrv;
    uint _multipleIteration;
};

// uv.y를 따라 행성 중심에서 _atmosphereRadius높이까지 _densityReferenceRadius기준으로 반환
// 0~0.25까지는 반전되어서 transitoinHeightAtReference -> 0 높이까지 낮은 해상도로 반환
// 0.25~1까지는 transitoinHeightAtReference -> atmosHeightAtReference까지 높은 해상도로 반환
float atmosphereMultipleScatteringHeightAtReference(float v)
{
    if (v < 0.25f)
    {
        const float u = 1.0f - v * 4.0f;
        return atmosphereTransitionHeight() - _densityTransitionRadius * u * u;
    }
    
    const float u = (v - 0.25f) / 0.75f;
    return atmosphereTransitionHeight() + (_atmosphereRadius - _densityTransitionRadius) * u * u;
}

// 전체 구면에 비교적 고르게 퍼진 32개의 단위 방향 벡터 중, i번째 방향을 반환하는 함수 (Fibonacci sphere 배치)
float3 directionMultipleScattering(uint i)
{
    const float y = 1.0f - 2.0f * (float(i) + 0.5f) / float(MULTIPLESCATTERING_COUNT);    // y를 먼저 균등하게 선정
    GPUASSERT(1.0f - y * y >= 0);
    const float s = sqrt(1.0f - y * y);                         // 단위 구에서는 x² + z² = 1 - y²
    // 황금각 (Golden Angle): 원을 황금비로 분할한 각도로, 약 2.399963 라디안(137.5077°)
    const float angle = float(i) * 2.39996323f;
    return float3(s * cos(angle), y, s * sin(angle));           // y위치에서 xz평면에 놓인 원의 x, y를 구함
}

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= ATMOSPHERE_MULTIPLESCATTERING_WIDTH || id.y >= ATMOSPHERE_MULTIPLESCATTERING_HEIGHT) 
        return;

    // uv.y 높이에서 태양이 이 방향(uv.x)에 있을 때, 주변 대기에서 산란되어 들어오는 빛이 평균적으로 얼마나 되는지를 계산
    // uv.y는 heightAtReference를 담당 (0~_atmosphereRadius 까지)
    // uv.x는 sunDirection을 담당 (방사방향 기준 0~180도 까지)
        
    const float heightAtReference = atmosphereMultipleScatteringHeightAtReference(float(id.y) / (ATMOSPHERE_MULTIPLESCATTERING_HEIGHT - 1u));
    GPUASSERT(_densityReferenceRadius > 0);
    GPUASSERT(_densityReferenceRadius + heightAtReference >= 0);

    const float height = _densityReferenceRadius + heightAtReference;
    const float muSun = 2.0f * float(id.x) / (ATMOSPHERE_MULTIPLESCATTERING_WIDTH - 1u) - 1.0f;
    GPUASSERT(1.0f - muSun * muSun >= 0);
    const float3 sunDir = float3(sqrt(1.0f - muSun * muSun), muSun, 0);     // 행성중심이 0,0,0 이라는 가정하에 각도계산
    const float3 betaR = atmosphereRayleighScattering();
    const float3 betaM = atmosphereMieScattering();

    float3 sum = 0.0f;
    float2 phaseSum = 0.0f;
    [unroll] 
    for (uint phaseIndex = 0u; phaseIndex < MULTIPLESCATTERING_COUNT; ++phaseIndex)
    {
        const float mu = dot(directionMultipleScattering(phaseIndex), sunDir);
        phaseSum += float2(atmosphereRayleighPhase(mu), atmosphereMiePhase(mu));
    }

    GPUASSERT(all(phaseSum > 0));

    [loop]  // 내부 loop가 있기 때문에 unroll해도되지만 VGPR등의 영향이 커질 수 있으므로 성능 비교 후 바꾸어야함
    for (uint d = 0u; d < MULTIPLESCATTERING_COUNT; ++d)
    {
        const float3 dir = directionMultipleScattering(d);
        const float radialDot = height * dir.y;
        const float distance = atmosphereTopDistance(heightAtReference, radialDot);
        if (distance <= 0.0f)
            continue;
            
        GPUASSERT(heightAtReference <= (_atmosphereRadius - _densityReferenceRadius));
        const float mu = dot(dir, sunDir);

        // 그냥 phase함수를 바로 쓰면 phase함수의 총합은 1이어야 한다는 규칙이 깨집니다. (샘플링이 랜덤이니 값이 큰 곳이 여러번 샘플링되면 당연 모두 합쳤을 때 값이 커짐)
        // 이걸 방지하고자 미리 샘플링을 돌려서 그 값들을 다 더한 total을 나누고 현재 샘플링은 total로 나눈 ratio로 사용하면 모든 샘플링의 phase값들을 다 더해도 1로 유지됩니다.
        const float2 phase = float2(atmosphereRayleighPhase(mu), atmosphereMiePhase(mu)) * (32.0f / (4.0f * PI)) / phaseSum;

        float3 T = 1.0f;    // 시작 위치부터 현재 구간까지 누적된 투과율
        float3 L = 0.0f;
        [loop] 
        for (uint step = 0u; step < 32u; ++step)    // integrateAtmosphereOpticalDepth와 비슷한 형태. 여기서는 opticalDepth적분값이 아닌 빛의 양을 구하는 게 목표
        {
            const float u0 = float(step) / 32.0f;
            const float u1 = float(step + 1u) / 32.0f;
            const float start = distance * u0 * u0 * u0;    // u³으로 distance의 앞쪽을 더 많이 샘플링
            const float end = distance * u1 * u1 * u1;      // u³으로 distance의 앞쪽을 더 많이 샘플링
            const float width = end - start;
            const float mid = 0.5f * (start + end);
            const float heightAtReference2 = atmosphereHeightOnRay(heightAtReference, radialDot, mid);
            GPUASSERT(heightAtReference2 <= (_atmosphereRadius - _densityReferenceRadius), float4(heightAtReference2, _densityReferenceRadius, mid, distance));
            const float4 density = atmosphereDensity(heightAtReference2);       // heightAtReference2높이일 때 밀도
            const float3 sigma = atmosphereOpticalExtinction(density);          // heightAtReference2높이일 때 산란과 흡수로 빛이 줄어드는 소멸계수 (단위미터당 소멸하는 정도)
            GPUASSERT(all(sigma >= FLOAT_EPSILON_MIN));
            const float3 tau = sigma * width;                                   // width 구간의 opticalDepth
            const float3 transmittance = exp(-tau);                             // width 구간의 투과율 (구간 끝이 구간을 전부를 통과하여 start에 도달할 때 투과율)
            const float3 weight = float3(                                       // start -> end를 지나오면서 누적되는 투과율 (구간 모든 점에서 빛이 발생되고 해당 빛들이 각 구간을 지나 start에 도달할때 전체 투과율)
                tau.x < FLOAT_EPSILON ? width * (1.0f - 0.5f * tau.x) : (1.0f - transmittance.x) / sigma.x,
                tau.y < FLOAT_EPSILON ? width * (1.0f - 0.5f * tau.y) : (1.0f - transmittance.y) / sigma.y,
                tau.z < FLOAT_EPSILON ? width * (1.0f - 0.5f * tau.z) : (1.0f - transmittance.z) / sigma.z);
            
            const float radialSun = clamp(height * sunDir.y + mid * dot(dir, sunDir), -heightAtReference2 + _densityReferenceRadius, heightAtReference2 + _densityReferenceRadius); // 샘플 위치 Q = float3(0, height, 0) + mid * dir 를 태양 방향으로 투영한 값: dot(Q, sunDir)
            //GPUASSERT(abs(radialSun) <= heightAtReference2 + _densityReferenceRadius, float4(radialSun, heightAtReference2, _densityReferenceRadius, 0));
            //GPUASSERT(abs(radialSun) <= heightAtReference2 + _densityReferenceRadius, float4(height, sunDir.y, mid, dot(dir, sunDir)));
            GPUASSERT(abs(radialSun) <= heightAtReference2 + _densityReferenceRadius, float4(height * sunDir.y, mid, dot(dir, sunDir), mid * dot(dir, sunDir)));
            GPUASSERT(heightAtReference2 <= (_atmosphereRadius - _densityReferenceRadius), float4(heightAtReference2, atmosphereTransitionHeight(), _atmosphereRadius - _densityReferenceRadius, 0));
            const AtmosphereOpticalDepth optical = atmosphereSunOpticalDepth(heightAtReference2, radialSun, atmosphereTopDistance(heightAtReference2, radialSun));

            // direct = 태양→구간 투과율 × (Rayleigh 산란계수×밀도×위상함수 + Mie 산란계수×밀도×위상함수)
            // direct는 단위 태양 조사량(즉. sunIntensity가 1)을 기준으로 값을 설정하고 이후에 실제 랜더링에서 sunIntensity를 곱한다. sunIntensity에 의해 산란량이 linear증가하므로 이렇게해도 괜찮다.
            // 이 이점으로는 sunIntensity가 변해도 LUT를 새로 갱신하지 않아도 된다는 것이다.
            const float3 direct = exp(-atmosphereOpticalExtinction(optical.value)) * (betaR * density.x * phase.x + betaM * density.y * phase.y);
            float3 indirect = 0.0f; // 이미 대기에서 산란한 빛이 해당 구간에서 다시 산란한 기여
            if (_multipleIteration > 0u)
            {
                const float r = _densityReferenceRadius + heightAtReference2;
                GPUASSERT(r > FLOAT_EPSILON);
                const float3 scattering = betaR * density.x + betaM * density.y;
                GPUASSERT(abs(radialSun) <= r, float4(radialSun, r, radialSun / r, 0));
                // 위상함수 곱하는 처리가 없는 이유는 sun += MULTIPLESCATTERING_COUNT 을 하는 라인 위에 주석 참조
                indirect = scattering * sampleAtmosphereMultipleScattering(_multipleInputSrv, heightAtReference2, clamp(radialSun / r, -1, 1));  // 정밀도 이슈로 clamp
            }

            L += T * (direct + indirect) * weight;  // 구간 안에서 발생한 빛을 적분
            T *= transmittance;                     // 다음 구간까지의 누적 투과율 갱신
            if (all(T < FLOAT_EPSILON))                     // 투과율이 0에 가까우면 남는 빛이 없으므로 이후 계산은 의미가 없으니 종료
                break;
        }

        // 원래식은 sum += L * (4PI / MULTIPLESCATTERING_COUNT) 여야 맞다. (모든 방향에서 오는 빛을 MULTIPLESCATTERING_COUNT샘플링 했으므로 가중치 4PI/MULTIPLESCATTERING_COUNT를 곱해야함)
        // 그러나 다음 indirect에서 사용될 때 위상함수를 등방성 함수(1/4PI)를 사용하므로 약분처리되어 여기서 4PI를 제거한 MULTIPLESCATTERING_COUNT 나눗셈만 처리함
        // 워래 제대로 하기 위해서는 방향별 L을 모두 저장해야하므로 (WIDTH * HEIGHT * MULTIPLESCATTERING_COUNT)만큼 커져서 일단 등방성 위상함수를 사용하도록함
        sum += L / MULTIPLESCATTERING_COUNT;
    }

    const RWTexture2D<float4> output = getTextureRW(_multipleOutputUav);
    output[id.xy] = float4(max(sum, 0.0f), 1.0f);
}

#endif
