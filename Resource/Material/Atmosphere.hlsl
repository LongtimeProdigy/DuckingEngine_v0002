#ifndef __DEFINE_ATMOSPHERE_HLSL__
#define __DEFINE_ATMOSPHERE_HLSL__

#include "CommonRendering.hlsl"
#include "CommonTexture.hlsl"

#define APPLY_MIE_SCATTERING

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

    // b >= 0, rootDet > 0 이기 때문에
    // destToSphere2은 항상 destToSphere1보다 크다. 때문에 둘 중에 하나만 음수여야 한다면 항상 destToSphere1가 음수일 수 밖에 없다.
    // 따라서 destToSphere2가 음수라는 것은 항상 rayOrigin보다 뒤에 Sphere가 있다는 것 (이 경우엔 intersection하지 않는다고 본다)
    if(destToSphere2 < 0)
        return float2(FLOAT_MAX, 0);

#if 0
    const float nearestPoint = destToSphere1 >= 0 ? destToSphere1 : destToSphere2;

    // destToSphere1이 음수인 경우: rayOrigin이 Sphere 내부에 있다. >> rayOrigin/Dir의 양수거리만큼만 반환한다. 그리고 Through는 rayOrigin부터 HitPoint까지만 반환한다.
    // destToSphere1이 양수인 경우: rayOrigin이 Sphere 외부에 있다. >> 이경우엔 destToSphere1이 무조건 더 작으므로 가장 가까운 거리를 반환한다.
#if 0
    const float throughLength = abs(destToSphere2 - destToSphere1);
#else
    const float throughLength = destToSphere1 >= 0 ? destToSphere2 - destToSphere1 : destToSphere2;
#endif
    return float2(nearestPoint, throughLength);
#else
    // destToSphere1이 음수인 경우: rayOrigin이 Sphere 내부에 있다. >> rayOrigin/Dir의 양수거리만큼만 반환한다. 그리고 Through는 rayOrigin부터 HitPoint까지만 반환한다.
    // destToSphere1이 양수인 경우: rayOrigin이 Sphere 외부에 있다. >> 이경우엔 destToSphere1이 무조건 더 작으므로 가장 가까운 거리를 반환한다.
    const float3 nearPoint = float3(
        rayOrigin.x + (destToSphere1 < 0 ? 0 : destToSphere1) * rayDir.x, 
        rayOrigin.y + (destToSphere1 < 0 ? 0 : destToSphere1) * rayDir.y, 
        rayOrigin.z + (destToSphere1 < 0 ? 0 : destToSphere1) * rayDir.z
        );
    const float3 farPoint = float3(
        rayOrigin.x + destToSphere2 * rayDir.x, 
        rayOrigin.y + destToSphere2 * rayDir.y, 
        rayOrigin.z + destToSphere2 * rayDir.z
        );

    const float nearLength = length(rayOrigin - nearPoint);
    const float farLength = length(rayOrigin - farPoint);

    return float2(nearLength, farLength - nearLength);
#endif
}

float3 calculateLight(const float3 rayOrigin, const float3 rayDir, const float rayLength, const float3 originalCol, const float3 sunDir)
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

    // rayLength를 너무 길게 가져가면 레일리산란이 너무 누적되어서 지평선에 붉은 띠가 형성됩니다.
    const float maxViewDistance = 120000.0; // 120 km 정도
    const float effectiveRayLength = min(rayLength, maxViewDistance);
    const float segmentLength = effectiveRayLength / _numInScatteringPoints;
    float tCurrent = 0;
    float3 sumR = float3(0, 0, 0);
    float3 sumM = float3(0, 0, 0);
    float opticalDepthR = 0;
    float opticalDepthM = 0;

    for(int i = 0; i < _numInScatteringPoints; ++i)
    {
        const float3 samplePosition = rayOrigin + (tCurrent + segmentLength * 0.5f) * rayDir; // Segment 중간에서 Sample하기 위해 semgentLength * 0.5를 더함
        float height = length(samplePosition - _planetCentre) - _planetRadius;
        height = max(height, 0.0);

        // View 방향으로의 공기 밀도 누적
        const float hrR = exp(-height / HrR);
        opticalDepthR += hrR * segmentLength;
        const float hrM = exp(-height / HrM);
        opticalDepthM += hrM * segmentLength;

        // 태양 위치부터 SamplePosition까지의 공기 밀도 계산
        const float sunRayLength = raySphere(_planetCentre, _atmosphereRadius, samplePosition, sunDir).y; // 태양빛이 대기를 가로지르는 길이

        // Self Shadow 제외
        if (sunRayLength <= 0.0)
            continue;

        const float segmentLengthLight = sunRayLength / _opticalDepthPointCount;
        float tCurrentLight = 0;
        float opticalDepthLightR = 0;
        float opticalDepthLightM = 0;
        for(int j = 0; j < _opticalDepthPointCount; ++j )
        {
            const float3 samplePositionLight = samplePosition + (tCurrentLight + segmentLengthLight * 0.5f) * sunDir;
            float heightLight = length(samplePositionLight - _planetCentre) - _planetRadius;
            heightLight = max(heightLight, 0.0);

            opticalDepthLightR += exp(-heightLight / HrR) * segmentLengthLight;
            opticalDepthLightM += exp(-heightLight / HrM) * segmentLengthLight;

            tCurrentLight += segmentLengthLight;
        }

        // Camera방향, Sun방향 공기밀도를 합해서 투과율로 변환
#if !defined(APPLY_MIE_SCATTERING)
        const float3 tau = betaR * (opticalDepthR + opticalDepthLightR);
#else
        const float3 tau = betaR * (opticalDepthR + opticalDepthLightR) + betaM * (opticalDepthM + opticalDepthLightM);
#endif
        const float3 attenuation = exp(-tau);

        // 투과율을 리만적분 누적
        sumR += attenuation * hrR * segmentLength;
        sumM += attenuation * hrM * segmentLength;

        tCurrent += segmentLength;
    }

    // 리만적분에 필요했던 상수 부분들 마지막에 곱(위상함수, beta)
    const float mu = dot(rayDir, sunDir);
    const float phaseR = (3.f / (16.f * M_PI)) * (1 + mu * mu);
#if !defined(APPLY_MIE_SCATTERING)
    return _sunIntensity * (betaR * sumR * phaseR);
#else
    const float phaseM = (1.0 - g * g) / (4.0 * M_PI * pow(1.0 + mu - 2.0 * g * mu, 1.5));
    return _sunIntensity * (betaR * sumR * phaseR + sumM * betaM * phaseM);
#endif
}

bool renderSun(const float3 rayOrigin, const float3 rayDir, out float3 sunDir)
{
#define SUN_PERIOD_TIME 0.003f
#define SUN_DISTANCE 100 // m단위
    const float sunPeriodTime = SUN_PERIOD_TIME * 3600;
    const float dayOfTimeSecond = _time % sunPeriodTime;
#if 1
    const float sunDegree = _sunDegree * (3.141592 / 180);// radians(360 / sunPeriodTime) * dayOfTimeSecond;// * (dayOfTimeSecond - (6 * 3600)); // 오전 6시부터 0도
#else
    const float sunDegree = radians(360 / sunPeriodTime) * dayOfTimeSecond;// * (dayOfTimeSecond - (6 * 3600)); // 오전 6시부터 0도
#endif
    sunDir = float3(0, sin(sunDegree), cos(sunDegree));
    const float4 sunPositionWS = float4(sunDir * SUN_DISTANCE, 1);

    const float2 hitInfo = raySphere(sunPositionWS.xyz, _sunRadius, rayOrigin, rayDir);
    if(hitInfo.y <= 0)
        return false;

    return true;
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
    const bool successSunRender = renderSun(rayOriginWS, rayDirWS, sunDir);
    if(successSunRender)
        return float4(255 / 255.0f, 221 / 255.0f, 64 / 255.0f, 1);

    const Texture2D renderTexture = getRenderTargetTexture(_frameIndex.x, 0);
    const Texture2D depthTexture = getDepthStencilTexture(_frameIndex.x, 0);
    const float4 originalCol = renderTexture.Load(uint3(input.uv * _resolution, 0));
    
    const float depth = depthTexture.Load(uint3(input.uv * _resolution, 0)).x;
    // 기본적으로 depthTexture의 Value는 log된 값입니다. 이를 선형적 값으로 변환합니다.
    const float linearDepth01 = (_nearDistance) / (_farDistance + _nearDistance - depth * (_farDistance - _nearDistance));
    const float linearDepthDistanceWS = linearDepth01 * (_farDistance - _nearDistance);

    // // TODO: 나중엔 stencil로 교체해야함
    // if(depth != 1.f)
    //     return originalCol;
    
    const float2 hitInfo = raySphere(_planetCentre, _atmosphereRadius, rayOriginWS, rayDirWS);
    const float destToAtmosphere = hitInfo.x;                       // 대기 near 거리 (만약 카메라가 대기 안에 있다면 0)
    const float lengthAtmosphere = hitInfo.y;                       // 대기 관통 거리 (만약 카메라가 대기 안에 있다면 far - camerapos)

    if(lengthAtmosphere == 0)
        return float4(1, 0, 0, 1);  // 우주공간

    const float lengthToSurface = linearDepthDistanceWS - destToAtmosphere;                 // near to surface 또는 camerapos to surface
    const float destThroughAtrmosphere = depth != 1.f ? lengthToSurface : lengthAtmosphere; // 지면에 안부딪힐 경우 대기 관통거리, 지면에 부딪힌 경우 카메라 또는 near부터 부딪힌 지면까지의 거리
    const float3 pointInAtmosphere = rayOriginWS + rayDirWS * (destThroughAtrmosphere/* + FLOAT_EPSILON*/);
    const float3 lightHDR = calculateLight(rayOriginWS, rayDirWS, destThroughAtrmosphere /*- FLOAT_EPSILON * 2*/, originalCol.xyz, sunDir);
    const float3 lightToneMap = toneMapACES(lightHDR);
    return originalCol * (1 - float4(lightToneMap, 1)) + float4(lightToneMap, 1);
}
#endif