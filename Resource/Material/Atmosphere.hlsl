#ifndef __DEFINE_ATMOSPHERE_HLSL__
#define __DEFINE_ATMOSPHERE_HLSL__

#include "CommonRendering.hlsl"
#include "CommonTexture.hlsl"
#include "AtmosphereParameters.hlsl"

cbuffer AtmosphereInputConstants : register(b2)
{
    uint _materialIndex;
};

//#define OPTIMZE_OPTICALDEPTH

struct AtmosphereSegment
{
    float3 transmittance;
    float3 inScattering;
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

// All positions in this shader are relative to the camera. The CPU supplies
// camera altitude in double precision; this form preserves small surface heights.
float atmosphereHeight(float3 cameraRelativePosition)
{
    const float3 cameraRadial = -_planetCentre;
    const float numerator = dot(cameraRelativePosition, cameraRelativePosition) + 2.0f * dot(cameraRelativePosition, cameraRadial) + _cameraAltitude * (2.0f * _planetRadius + _cameraAltitude);
    return numerator / max(length(cameraRadial + cameraRelativePosition) + _planetRadius, 1.0f);
}

float2 raySphere(const float3 sphereCentre, const float sphereRadius, const float3 rayOrigin, const float3 rayDir)
{
    const float3 radial = rayOrigin - sphereCentre;
    const float altitude = atmosphereHeight(rayOrigin) + (_planetRadius - sphereRadius);
    const float c = altitude * (2.0f * sphereRadius + altitude);
    const float b = dot(radial, rayDir);
    const float determinant = b * b - c;
    if (determinant < 0.0f)
        return float2(FLOAT_MAX, 0);
    const float root = sqrt(determinant);
    // Stable quadratic roots avoid subtracting two nearly equal values at the ground.
    const float q = -b - (b >= 0.0f ? root : -root);
    const float t0 = q;
    const float t1 = abs(q) > 1e-10f ? c / q : -b;
    const float nearHit = min(t0, t1);
    const float farHit = max(t0, t1);
    if (farHit <= 0.0f)
        return float2(FLOAT_MAX, 0);
    const float entry = max(nearHit, 0.0f);
    return float2(entry, max(farHit - entry, 0.0f));
}

AtmosphereSegment calculateAtmosphere(const float3 rayOrigin, const float3 rayDir, const float rayLength, const float3 sunDir)
{
    static const float M_PI = 3.141592f;
    // https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/simulating-sky/simulating-colors-of-the-sky.html
    // https://www.shadertoy.com/view/tdSXzD
    // Rayleigh
    const float3 betaR = float3(3.8e-6f, 13.5e-6f, 33.1e-6f) * _scatteringScale;
    const float HrR = max(_rayleighHeight, 1e-6f);
    // Mie
    const float3 betaM = float3(21e-6, 21e-6, 21e-6) * _scatteringScale;
    const float HrM = max(_mieHeight, 1e-6f);
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
        const float height = max(atmosphereHeight(samplePosition), 0.0f);

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

        // Keep view extinction, but suppress direct sunlight behind the planet.
        const float2 groundShadow = raySphere(_planetCentre, _planetRadius, samplePosition, sunDir);
        if (groundShadow.y > 0.0f && groundShadow.x < sunRayLength)
            continue;

        const float segmentLengthLight = sunRayLength / lightCount;

        // Light 방향으로의 공기 밀도 계산
        float opticalDepthLightR = 0;
        float opticalDepthLightM = 0;
        for (uint j = 0; j < lightCount; ++j)
        {
            const float3 samplePositionLight = samplePosition + ((j + 0.5f) * segmentLengthLight) * sunDir;
            const float heightLight = max(atmosphereHeight(samplePositionLight), 0.0f);

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

float renderSun(const float3 rayDir, out float3 sunDir)
{
    const float sunDegree = radians(_sunDegree);
    sunDir = float3(0, sin(sunDegree), cos(sunDegree));
    // atan2 avoids the precision loss of acos(dot) around a tiny solar disk.
    const float angle = atan2(length(cross(rayDir, sunDir)), dot(rayDir, sunDir));
    const float pixelAngle = max(fwidth(angle), 1e-6f);
    const float radius = radians(_sunAngularRadius);
    return 1.0f - smoothstep(radius - pixelAngle * 0.5f, radius + pixelAngle * 0.5f, angle);
}

float3 groundSunTransmittance(float3 surfacePosition, float3 normal, float3 sunDir)
{
    if (dot(normal, sunDir) <= 0.0f)
        return float3(0, 0, 0);
        
    // Offset off the surface to avoid self-intersection after float rounding.
    const float3 origin = surfacePosition + normal * max(_planetRadius * 1e-6f, 1e-6f);
    const float pathLength = raySphere(_planetCentre, _atmosphereRadius, origin, sunDir).y;
    const uint count = max(_opticalDepthPointCount, 1u);
    const float stepLength = pathLength / count;
    float2 opticalDepth = float2(0, 0);
    for (uint i = 0; i < count; ++i)
    {
        const float3 position = origin + sunDir * ((i + 0.5f) * stepLength);
        const float height = max(atmosphereHeight(position), 0.0f);
        opticalDepth += exp(-height / max(float2(_rayleighHeight, _mieHeight), 1e-6f)) * stepLength;
    }
    
    const float3 betaR = float3(3.8e-6f, 13.5e-6f, 33.1e-6f) * _scatteringScale;
    const float3 betaM = float3(21e-6f, 21e-6f, 21e-6f) * _scatteringScale;
    return exp(-betaR * opticalDepth.x - betaM * opticalDepth.y);
}
float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    const float nearPlaneHalfHeightWS = _nearDistance * getFOVTangent();
    const float nearPlaneHalfWidthWS = nearPlaneHalfHeightWS * ((float)_resolution.x / (float)_resolution.y);
    const float2 nearPlaneUV = float2(input.uv.x, 1 - input.uv.y) * 2 - 1;
    const float3 rayDirToNearPlaneWS = getViewForwardDirection() * _nearDistance + 
        getViewRightDirection() * nearPlaneHalfWidthWS * nearPlaneUV.x + 
        getViewUpDirection() * nearPlaneHalfHeightWS * nearPlaneUV.y;

    const float3 rayOriginWS = float3(0, 0, 0); // Camera-relative origin.
    const float3 rayDirWS = normalize(rayDirToNearPlaneWS);

    float3 sunDir = float3(0, -1, 0);
    const float sunCoverage = renderSun(rayDirWS, sunDir);
    // Uniform disk: irradiance on a facing plane is L * pi * sin(radius)^2.
    const float sinRadius = sin(radians(_sunAngularRadius));
    const float sunRadiance = _sunIntensity * _sunRadianceScale / max(3.141592f * sinRadius * sinRadius, 1e-8f);

    const Texture2D<float4> renderTexture = getRenderTargetTexture(_frameIndex.x, 0);
    const Texture2D<float4> depthTexture = getDepthStencilTexture(_frameIndex.x, 0);
    const float4 originalCol = renderTexture.Load(uint3(input.uv * _resolution, 0));
    // Debug primitives use the unlit channel and must survive sky/ground composition.
    if (getTexture(_materialIndex).Load(uint3(input.uv * _resolution, 0)).z > 0.5f)
        return float4(originalCol.rgb, 1);
    
    const float depth = depthTexture.Load(uint3(input.uv * _resolution, 0)).x;
    // 기본적으로 depthTexture의 Value는 log된 값입니다. 이를 선형적 값으로 변환합니다.
    const float viewZ = (_nearDistance * _farDistance) / (_farDistance - depth * (_farDistance - _nearDistance));
    const float linearDepthDistanceWS = viewZ / max(dot(rayDirWS, getViewForwardDirection()), 1e-6f);

    const float2 hitInfo = raySphere(_planetCentre, _atmosphereRadius, rayOriginWS, rayDirWS);
    const float destToAtmosphere = hitInfo.x;   // 대기 near 거리 (만약 카메라가 대기 안에 있다면 0)
    const float lengthAtmosphere = hitInfo.y;   // 대기 관통 거리 (만약 카메라가 대기 안에 있다면 far - camerapos)

    float3 backgroundRadiance = depth < 1.0f ? originalCol.rgb : float3(1, 1, 1) * sunRadiance * sunCoverage;

    if(lengthAtmosphere == 0)
        return float4(backgroundRadiance, 1);

    const float atmosphereEnd = destToAtmosphere + lengthAtmosphere;
    float rayEnd = depth < 1.0f ? min(linearDepthDistanceWS, atmosphereEnd) : atmosphereEnd;
    // Actual terrain wins where it has depth; the reference sphere fills uncovered pixels.
    if (depth >= 1.0f)
    {
        const float2 groundHit = raySphere(_planetCentre, _planetRadius, rayOriginWS, rayDirWS);
        if (groundHit.y > 0.0f && groundHit.x < rayEnd)
        {
            rayEnd = groundHit.x;
            const float3 normal = normalize(rayOriginWS + rayDirWS * rayEnd - _planetCentre);
            // Direct sunlight crosses the atmosphere before reaching the surface.
            const float3 surfacePosition = rayOriginWS + rayDirWS * rayEnd;
            backgroundRadiance = float3(0.12f, 0.14f, 0.16f) * (_sunIntensity / 3.141592f) * saturate(dot(normal, sunDir)) * groundSunTransmittance(surfacePosition, normal, sunDir);
        }
    }
    
    const float integrationLength = max(0.0f, rayEnd - destToAtmosphere);
    const float3 atmosphereStart = rayOriginWS + rayDirWS * destToAtmosphere;
    
    const AtmosphereSegment atmosphere = calculateAtmosphere( atmosphereStart, rayDirWS, integrationLength, sunDir);

    const float3 composedHDR = backgroundRadiance * atmosphere.transmittance + atmosphere.inScattering;
    return float4(composedHDR, 1);
}
#endif
