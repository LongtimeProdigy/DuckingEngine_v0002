#ifndef __DEFINE_ATMOSPHERE_HLSL__
#define __DEFINE_ATMOSPHERE_HLSL__

#include "CommonRendering.hlsl"
#include "CommonTexture.hlsl"

//#define OPTIMZE_OPTICALDEPTH

struct AtmosphereSegment
{
    float3 transmittance;
    float3 inScattering;
};

cbuffer AtmosphereConstantBuffer : register(b1)
{
	uint _numInScatteringPoints;
	uint _opticalDepthPointCount;
	uint _densityFallOff;
	uint _scatteringStrength;

	float _sunDegree = 7.f;
    uint _sunIntensity = 20;
    uint _sunRadius = 10;
    uint _padding;	

    int _planetRadius;
    int3 _planetCentre;

    int _atmosphereRadius;
};

struct VS_INPUT
{
    float2 position : POSITION;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.position = float4(input.position, 0, 1);
    output.uv = ((input.position) + 1) / 2;
    output.uv.y = 1 - output.uv.y;
    return output;
}

float2 raySphere(const float3 sphereCentre, const float sphereRadius, const float3 rayOrigin, const float3 rayDir)
{
    /*
    *   https://gohen.tistory.com/79 ... 조금 잘못된 식들이 있긴한데.. 얼추 개념은 이해하기 쉬움. 진짜 식은 아래를 따라야함
    *   결국 3차원 직선의 방정식에서 t에 대한 2차방정식을 세우고 판별식을 통해서 intersection여부를 판단한다.
    */
    // https://stackoverflow.com/questions/6533856/ray-sphere-intersection
    const float3 dirCenterToOrigin = rayOrigin - sphereCentre;

    const float c = dot(dirCenterToOrigin, dirCenterToOrigin) - sphereRadius * sphereRadius;
#if 0// no optimize
    const float a = dot(rayDir, rayDir);
    const float b = 2 * dot(dirCenterToOrigin, rayDir);
    const float det = b * b - 4 * a * c;

    // 판별식이 0미만이면 접하지 않음
    if(det < 0)
        return float2(FLOAT_MAX, 0);

    const float denominator = 2 * a;
    const float rootDet = sqrt(det);
    const float destToSphere1 = (-b - rootDet) * denominator;
    const float destToSphere2 = (-b + rootDet) * denominator;
#else
    const float b = dot(dirCenterToOrigin, rayDir);
    const float det = b * b - c;

    // 판별식이 0미만이면 접하지 않음
    if(det < 0)
        return float2(FLOAT_MAX, 0);
    
    const float rootDet = sqrt(det);
    const float destToSphere1 = (-b - rootDet);
    const float destToSphere2 = (-b + rootDet);
#endif

    // 여기까지 오면 b >= 0, rootDet > 0이다.
    // 따라서, destToSphere2는 항상 destToSphere1보다 크다. 때문에 둘 중 음수가 있다면 항상 destToSphere1은 음수일 수 밖에 없다.
    // 따라서, 둘 다 음수인 경우는 destToSphere2가 음수인 경우랑 동치이다.
    // 그리고 이 경우는 두 점이 rayOrigin부터 -rayDir방향에 있다는 것이기 때문에 여기선 교차하지 않는 것으로 처리한다.
    if(destToSphere2 < 0)
        return float2(FLOAT_MAX, 0);

    // destToSphere2는 양수이므로 두가지 경우의 수밖에 없다
    // destToSphere1이 음수인 경우: rayOrigin이 Sphere 내부에 있다. >> rayOrigin/Dir의 양수거리만큼만 반환한다. 그리고 Through는 rayOrigin부터 HitPoint까지만 반환한다.
    // destToSphere1이 양수인 경우: rayOrigin이 Sphere 외부에 있다. >> 이경우엔 destToSphere1이 무조건 더 작으므로 가장 가까운 거리를 반환한다.
    const float nearLength = destToSphere1 > 0 ? destToSphere1 : 0;
    const float farLength = destToSphere2 - nearLength;

    return float2(nearLength, farLength);
}

AtmosphereSegment calculateAtmosphere(const float3 rayOrigin, const float3 rayDir, const float rayLength, const float3 sunDir)
{
    static const float M_PI = 3.141592f;
    // https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/simulating-sky/simulating-colors-of-the-sky.html
    // https://www.shadertoy.com/view/tdSXzD
    // Rayleigh
    static const float3 betaR = float3(3.8e-6f, 13.5e-6f, 33.1e-6f);
    static const float HrR = 7994;
    // Mie
    static const float3 betaM = float3(21e-6, 21e-6, 21e-6);
    static const float HrM = 1200;
    static const float g = 0.76;

    if (rayLength <= 0.0f)
    {
        AtmosphereSegment result;
        result.transmittance = float3(1, 1, 1);
        result.inScattering = float3(0, 0, 0);
        return result;
    }

    const uint viewCount = max(_numInScatteringPoints, 1u);
    const uint lightCount = max(_opticalDepthPointCount, 1u);
    const float segmentLength = rayLength / viewCount;

    const float mu = dot(rayDir, sunDir);
    const float phaseR = (3.f / (16.f * M_PI)) * (1 + mu * mu);
    const float phaseM = (1.0 - g * g) / (4.0 * M_PI * pow(1.0 + g * g - 2.0 * g * mu, 1.5));

    float3 sumR = float3(0, 0, 0);
    float3 sumM = float3(0, 0, 0);
    float opticalDepthR = 0;
    float opticalDepthM = 0;
    for (uint i = 0; i < viewCount; ++i)
    {
        const float3 samplePosition = rayOrigin + ((i + 0.5f) * segmentLength) * rayDir;
        const float height = max(length(samplePosition - _planetCentre) - _planetRadius, 0.0f);

        // View 방향으로의 공기 밀도 누적
        const float hrR = exp(-height / HrR);
        const float hrM = exp(-height / HrM);
        const float segmentDepthR = hrR * segmentLength;
        const float segmentDepthM = hrM * segmentLength;
        // 현재 샘플까지의 깊이: 이전 구간 전체 + 현재 구간 절반.
        const float viewDepthR = opticalDepthR + segmentDepthR * 0.5f;
        const float viewDepthM = opticalDepthM + segmentDepthM * 0.5f;
        // 최종 투과율용 누적에는 현재 구간 전체를 포함한다.
        opticalDepthR += segmentDepthR;
        opticalDepthM += segmentDepthM;

        const float sunRayLength = raySphere(_planetCentre, _atmosphereRadius, samplePosition, sunDir).y; // 태양빛이 대기를 가로지르는 길이
        if (sunRayLength <= 0.0f)   // samplePosition이 대기 밖이면 누적 건너뜀
            continue;

        const float segmentLengthLight = sunRayLength / lightCount;

        // Light 방향으로의 공기 밀도 계산
        float opticalDepthLightR = 0;
        float opticalDepthLightM = 0;
        for (uint j = 0; j < lightCount; ++j)
        {
            const float3 samplePositionLight = samplePosition + ((j + 0.5f) * segmentLengthLight) * sunDir;
            const float heightLight = max(length(samplePositionLight - _planetCentre) - _planetRadius, 0.0f);

            opticalDepthLightR += exp(-heightLight / HrR) * segmentLengthLight;
            opticalDepthLightM += exp(-heightLight / HrM) * segmentLengthLight;
        }

#if defined(OPTIMZE_OPTICALDEPTH)
        // Camera방향, Sun방향 공기밀도를 합해서 투과율로 변환
        const float3 tau = betaR * (viewDepthR + opticalDepthLightR) + betaM * (viewDepthM + opticalDepthLightM);
        const float3 attenuation = exp(-tau);

        // 투과율을 리만적분 누적 (segmentLength를 곱해야 리만합이 되지만 최적화로 for문 밖에서 한번에 곱함)
        sumR += attenuation * hrR;
        sumM += attenuation * hrM;
#else
        // Camera방향, Sun방향 공기밀도를 합해서 투과율로 변환
        const float3 sunTransmittanceR = exp(-betaR * opticalDepthLightR);
        const float3 sunTransmittanceM = exp(-betaM * opticalDepthLightM);
        const float3 viewTransmittanceR = exp(-betaR * viewDepthR);
        const float3 viewTransmittanceM = exp(-betaM * viewDepthM);

        // betaR: 들어온 빛이 단위 밀도에서 단위 길이당 얼마나 산란되는지 나타내는 계수
        // hrR: betaR에 곱해져서 현재 밀도에서 산란되는 양을 구할 수 있도록 함
        // phase: 방향에 따라 산란되는 양을 계산하는 함수
        // 결국 3개를 다 곱해야 해당 밀도에서 원하는 방향으로 산란되어 나가는 빛의 양을 구할 수 있음
        const float3 scatterCeofficientCurrentHeightR = betaR * hrR;
        const float3 scatterCeofficientCurrentHeightM = betaM * hrM;
        const float3 scatterCepffocoemtCurrentHeightWithDirR = scatterCeofficientCurrentHeightR * phaseR;
        const float3 scatterCepffocoemtCurrentHeightWithDirM = scatterCeofficientCurrentHeightM * phaseM;

        const float3 remainSunIntensityAtSamplePosition = _sunIntensity * sunTransmittanceR * sunTransmittanceM;
        const float3 ramainSunIntensityAtSamplePositionToRayOriginR = remainSunIntensityAtSamplePosition * scatterCepffocoemtCurrentHeightWithDirR;
        const float3 ramainSunIntensityAtSamplePositionToRayOriginM = remainSunIntensityAtSamplePosition * scatterCepffocoemtCurrentHeightWithDirM;
        const float3 remainSunIntensityFromSamplePositionAtRayOriginR = ramainSunIntensityAtSamplePositionToRayOriginR * viewTransmittanceR * viewTransmittanceM;
        const float3 remainSunIntensityFromSamplePositionAtRayOriginM = ramainSunIntensityAtSamplePositionToRayOriginM * viewTransmittanceR * viewTransmittanceM;

        // samplePosition에서 rayOrigin으로 들어오는 빛의 양(레일리, 미)을 더함
        sumR += remainSunIntensityFromSamplePositionAtRayOriginR * segmentLength;
        sumM += remainSunIntensityFromSamplePositionAtRayOriginM * segmentLength;
#endif
    }

    // 리만적분에 필요했던 상수 부분들 마지막에 곱(위상함수, beta)
    AtmosphereSegment result;
#if defined(OPTIMZE_OPTICALDEPTH)
    result.transmittance = exp(-betaR * opticalDepthR - betaM * opticalDepthM);
    result.inScattering = _sunIntensity * segmentLength * (betaR * phaseR * sumR + betaM * phaseM * sumM);
#else
    result.transmittance = exp(-betaR * opticalDepthR - betaM * opticalDepthM);
    result.inScattering = sumR + sumM;
#endif
    return result;
}

bool renderSun(const float3 rayDir, out float3 sunDir)
{
#define SUN_PERIOD_TIME 0.003f
    const float sunPeriodTime = SUN_PERIOD_TIME * 3600;
    const float dayOfTimeSecond = _time % sunPeriodTime;
    const float sunDegree = radians(_sunDegree);// radians(360 / sunPeriodTime) * dayOfTimeSecond;// * (dayOfTimeSecond - (6 * 3600)); // 오전 6시부터 0도
    sunDir = float3(0, sin(sunDegree), cos(sunDegree));

    // 예시 각반경. 기존 _sunRadius 대신 사용한다.
    const float angularRadius = radians(0.27f);
    return dot(rayDir, sunDir) >= cos(angularRadius);
}

float3 toneMapACES(float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}
float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    const float nearPlaneHalfHeightWS = _nearDistance * getFOVTangent();
    const float nearPlaneHalfWidthWS = nearPlaneHalfHeightWS * ((float)_resolution.x / (float)_resolution.y);
    const float2 nearPlaneUV = float2(input.uv.x, 1 - input.uv.y) * 2 - 1;
    const float3 rayDirToNearPlaneWS = getViewForwardDirection() * _nearDistance + 
        getViewRightDirection() * nearPlaneHalfWidthWS * nearPlaneUV.x + 
        getViewUpDirection() * nearPlaneHalfHeightWS * nearPlaneUV.y;

    const float3 rayOriginWS = getViewPosition();
    const float3 rayDirWS = normalize(rayDirToNearPlaneWS);

    float3 sunDir = float3(0, -1, 0);
    const bool successSunRender = renderSun(rayDirWS, sunDir);

    const Texture2D<float4> renderTexture = getRenderTargetTexture(_frameIndex.x, 0);
    const Texture2D<float4> depthTexture = getDepthStencilTexture(_frameIndex.x, 0);
    const float4 originalCol = renderTexture.Load(uint3(input.uv * _resolution, 0));
    
    const float depth = depthTexture.Load(uint3(input.uv * _resolution, 0)).x;
    // 기본적으로 depthTexture의 Value는 log된 값입니다. 이를 선형적 값으로 변환합니다.
    const float viewZ = (_nearDistance * _farDistance) / (_farDistance - depth * (_farDistance - _nearDistance));
    const float linearDepthDistanceWS = viewZ / max(dot(rayDirWS, getViewForwardDirection()), 1e-6f);

    const float2 hitInfo = raySphere(_planetCentre, _atmosphereRadius, rayOriginWS, rayDirWS);
    const float destToAtmosphere = hitInfo.x;   // 대기 near 거리 (만약 카메라가 대기 안에 있다면 0)
    const float lengthAtmosphere = hitInfo.y;   // 대기 관통 거리 (만약 카메라가 대기 안에 있다면 far - camerapos)

    if(lengthAtmosphere == 0)
        return float4(1, 0, 0, 1);  // 우주공간

    const float atmosphereEnd = destToAtmosphere + lengthAtmosphere;
    const float rayEnd = depth < 1.0f ? min(linearDepthDistanceWS, atmosphereEnd)  : atmosphereEnd; // 실제 지형의 높낮이는 깊이 버퍼가 반영한다.
    
    const float integrationLength = max(0.0f, rayEnd - destToAtmosphere);
    const float3 atmosphereStart = rayOriginWS + rayDirWS * destToAtmosphere;
    
    const AtmosphereSegment atmosphere = calculateAtmosphere( atmosphereStart, rayDirWS, integrationLength, sunDir);

    float3 backgroundRadiance = float3(0, 0, 0);
    if (depth < 1.0f)
        backgroundRadiance = originalCol.rgb;
    else if (successSunRender)
        backgroundRadiance = float3(1, 1, 1) * float(_sunIntensity);

    const float3 composedHDR = backgroundRadiance * atmosphere.transmittance + atmosphere.inScattering;
    const float3 test = toneMapACES(composedHDR);

    return float4(test, originalCol.a);
}
#endif
