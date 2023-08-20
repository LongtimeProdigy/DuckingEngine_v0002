#ifndef __DEFINE_POSTPROCESS_HLSL__
#define __DEFINE_POSTPROCESS_HLSL__

#include "CommonRendering.hlsl"
#include "CommonTexture.hlsl"

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

float2 raySphere(float3 sphereCentre, float sphereRadius, float3 rayOrigin, float3 rayDir)
{
    float3 offset = rayOrigin - sphereCentre;
    float a = 1;
    float b = 2 * dot(offset, rayDir);
    float c = dot(offset, offset) - sphereRadius * sphereRadius;
    float d = b * b - 4 * a * c;    // 2차 방정식 판별식

    if (d > 0)  // 판별식이 0이거나 허수면 원과 접선이거나 원과 겹치지 않음
    {
        float s = sqrt(d);
        float destToSphereNear = max(0, (-b - s) / (2 * a));
        float destToSphereFar = (-b + s) / (2 * a);

        if(destToSphereFar >= 0)
            return float2(destToSphereNear, destToSphereFar - destToSphereNear);
    }

    return float2(9999999999999, 0);
}

float LinearEyeDepth(float depthValue, float nearClip, float farClip)
{
    // 기본적으로 depthTexture의 Value(depthValue)는 log한 값입니다.
    // 이를 선형적 값으로 변환합니다.
    return (2.0f * nearClip) / (farClip + nearClip - depthValue * (farClip - nearClip));
}

float densityFallOff = 10;
float densityAtPoint(float pointHeight, float atmosphereHeight)
{
    float height01 = pointHeight / atmosphereHeight;
    float localDensity = exp(-height01 * densityFallOff) * (1 - height01);

    return localDensity;
}
uint opticalDepthPointCount = 5;
float opticalDepth(float3 rayOrigin, float3 rayDir, float rayLength, float atmosphereHeight)
{
    float3 densitySamplePoint = rayOrigin;
    float stepSize = rayLength / (opticalDepthPointCount - 1);
    float opticalDepth = 0;

    for(int i = 0; i < opticalDepthPointCount; ++i)
    {
        float localDensity = densityAtPoint(densitySamplePoint.y, atmosphereHeight);
        opticalDepth += localDensity * stepSize;
        densitySamplePoint += rayDir * stepSize;
    }

    return opticalDepth;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    float worldSpaceNearPlaneHalfHeight = _nearDistance * getFOVTangent();
    float worldSpaceNearPlaneHalfWidth = worldSpaceNearPlaneHalfHeight * ((float)_resolution.x / (float)_resolution.y);
    float2 worldNearPlaneUV = float2(input.uv.x, 1 - input.uv.y) * 2 - 1;
    float3 viewDir = getViewForwardDirection() * _nearDistance
        + getViewRightDirection() * worldSpaceNearPlaneHalfWidth * worldNearPlaneUV.x
        + getViewUpDirection() * worldSpaceNearPlaneHalfHeight * worldNearPlaneUV.y;

    Texture2D renderTexture = getRenderTargetTexture(_frameIndex.x);
    Texture2D depthTexture = getDepthStencilTexture();
    float4 originalCol = renderTexture.Sample(normalSampler, input.uv);
    float depth = depthTexture.Sample(normalSampler, input.uv).x;
    float linearDepth = LinearEyeDepth(depth, _nearDistance, _farDistance);

    float3 rayOrigin = getViewPosition();
    float3 rayDir = normalize(viewDir);
    
    // Atmosphere PostProcessing
    // https://www.youtube.com/watch?v=DxfEbulyFcY&t=653s&ab_channel=SebastianLague
#if 0
    // 행성 원점 (0, 0, 0) 가져오기
    float3 planetCentre = float3(0, 0, 0);
    // 대기 반지름 (1unit = 1m >> 30000  = 30km) [10km 대류권, 50km 성층권, 80km 중간권, 600km 열권]
    float atmosphereRadius = 100;
    float2 hitInfo = raySphere(planetCentre, atmosphereRadius, rayOrigin, rayDir);
    float destToAtmosphere = hitInfo.x;
    float destThroughAtrmosphere = min(hitInfo.y, linearDepth - destToAtmosphere);

    float density = destThroughAtrmosphere / (atmosphereRadius * 2);
    return originalCol;
#else
    static uint inScatteringPointCount = 5;
    // 대기 반지름 (1unit = 1m >> 30000  = 30km) [10km 대류권, 50km 성층권, 80km 중간권, 600km 열권]
    float atmosphereHeight = 30000;
    float3 inScatterPoint = rayOrigin;
    float stepSize = min(10000, atmosphereHeight) / (inScatteringPointCount);
    float inScatteredLight = 0;

    float sunRadian = (2 * 3.141593f / 4) / 2;
    float3 sunDir = mul(float3(1, 0, 0), float3x3(cos(sunRadian), -sin(sunRadian), 0, 0, 0, 0, 0, 0, 0));

    for(int i = 0; i < inScatteringPointCount; ++i)
    {
        float sunRayLength = (atmosphereHeight - rayOrigin.y) / acos(cos((3.141592f / 2) - sunRadian));
        float sunRayOpticalDepth = opticalDepth(inScatterPoint, sunDir, sunRayLength, atmosphereHeight);
        float transmittance = exp(-sunRayOpticalDepth);
        float localDensity = densityAtPoint(inScatterPoint.y, atmosphereHeight);

        inScatteredLight += localDensity * transmittance * stepSize;
        inScatterPoint += rayDir * stepSize;
    }

    return originalCol;
#endif
}

#endif