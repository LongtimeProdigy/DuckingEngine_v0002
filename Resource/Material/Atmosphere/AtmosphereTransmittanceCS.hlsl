#ifndef ATMOSPHERE_TRANSMITTANCE_CS_HLSL
#define ATMOSPHERE_TRANSMITTANCE_CS_HLSL

#include "Atmosphere/AtmosphereTransmittance.hlsl"

cbuffer AtmosphereLutOutputConstants : register(b2)
{
    uint _atmosphereLutUav;
};

// uv를 실제 값으로 변경.
float2 atmosphereLutParameters(float2 uv)
{
    // uv.y의 0은 transitionradius 높이를 가리키기 때문에 referenceheight로 변환하기 위해서는 atmosphereTransitionHeight()를 더해야함
    const float heightAtReference = atmosphereTransitionHeight() + uv.y * uv.y * (_atmosphereRadius - _densityTransitionRadius); //uv.y²로 높이를 배치해 전환 경계 근처에 샘플이 집중됨

    const float horizon = atmosphereHorizonCosine(heightAtReference);   // heightAtReference에서 transitionRadius구에 접선하는 선분과 방사방향과의 mu
    const float s = 2.0f * uv.x - 1.0f;
    // 아래는 mu = dot(normalize(p - planetCentre), rayDir);
    // uv.x >= 0.5인 구간은 0~90 까지 보간 
    // uv.x < 0.5인 구간은 90~180까지 보간
    const float mu = s >= 0.0f ? lerp(horizon, 1.0f, s * s) : lerp(horizon, -1.0f, s * s);   // s²를 이용해서 접선부근(uv.x == 0.5)에서 샘플이 집중됨, s==0은 구에 접함

    return float2(heightAtReference, mu);
}

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= ATMOSPHERE_LUT_WIDTH || id.y >= ATMOSPHERE_LUT_HEIGHT) 
        return;

    // uv.x의 의미는 방사방향과 rayDir의 내적 mu이다. 1은 방사방향, 0.5는 방사방향과 수직, 0은 행성 중심방향 (rayDir은 sunDir을 시뮬레이션 한 방향)
    // uv.y의 의미는 transitionRadius가 0, atmosphereRadius가 1이라고 할때 그 사이의 높이
    // 따라서 output texture의 x축 한 줄(0~1로 이동)이 의미하는 바는 uv.y * (atmosphereRadius - transitionRadius) 높이에서 sunDir이 행성 중심방향에서 -> 방사방향으로 이동할때 OpticalDepth의 값

    // 주의. output의 opticalDepth는 특정 높이에서 행성을 투과하는 OpticalDepth를 구하기 때문에
    // SunDir방향에 물체가 있다면 실제로는 OpticalDepth를 계산하는 거리가 p부터 물체까지의 거리이지만 texture를 샘플링하게되면 행성을 투과하는 OpticalDepth를 가져가므로 부정확함
        
    const float2 uv = float2(id.xy) / float2(ATMOSPHERE_LUT_WIDTH - 1u, ATMOSPHERE_LUT_HEIGHT - 1u);
    const float2 parameters = atmosphereLutParameters(uv);
    const float heightAtReference = parameters.x;
    const float height = _densityReferenceRadius + heightAtReference;
    const float radialDot = height * parameters.y;
    const float pathLength = atmosphereTopDistance(heightAtReference, radialDot);
    const AtmosphereOpticalDepth optical = integrateAtmosphereOpticalDepth(heightAtReference, radialDot, pathLength);

    const RWTexture2D<float4> output = getTextureRW(_atmosphereLutUav);
    output[id.xy] = optical.value;
    output[id.xy + uint2(0u, ATMOSPHERE_LUT_HEIGHT)] = float4(float(optical.limitReached), 0, 0, 0);
}

#endif