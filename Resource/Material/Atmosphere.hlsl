#ifndef __DEFINE_ATMOSPHERE_HLSL__
#define __DEFINE_ATMOSPHERE_HLSL__

#include "CommonRendering.hlsl"
#include "CommonTexture.hlsl"
#include "AtmosphereParameters.hlsl"
#include "AtmosphereTransmittance.hlsl"

cbuffer AtmosphereInputConstants : register(b2)
{
    uint _materialIndex;
};

//#define OPTIMZE_OPTICALDEPTH

struct AtmosphereSegment
{
    float3 transmittance;
    float3 inScattering;
    uint limitReached;
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

AtmosphereSegment atmosphereIdentity()
{
    AtmosphereSegment result;
    result.transmittance = float3(1, 1, 1);
    result.inScattering = float3(0, 0, 0);
    result.limitReached = 0u;
    return result;
}

AtmosphereSegment composeAtmosphereIntervals(AtmosphereSegment nearPart, AtmosphereSegment farPart)
{
    AtmosphereSegment result;
    result.transmittance = nearPart.transmittance * farPart.transmittance;
    result.inScattering = nearPart.inScattering + nearPart.transmittance * farPart.inScattering;
    result.limitReached = nearPart.limitReached | farPart.limitReached;
    return result;
}

AtmosphereSegment evaluateAtmosphereInterval(float3 origin, float3 direction, float start, float end, float3 sunDir, float phaseR, float phaseM)
{
    AtmosphereSegment result = atmosphereIdentity();
    const float segmentLength = end - start;
    const float3 p = origin + direction * (0.5f * (start + end));
    const float height = atmosphereHeight(p);
    const float2 segmentDepth = atmosphereDensity(height) * segmentLength;
    const float3 betaR = gStandardBetaR * _scatteringScale;
    const float3 betaM = gStandardBetaM * _scatteringScale;
    result.transmittance = exp(-betaR * segmentDepth.x - betaM * segmentDepth.y);

    const float sunLength = raySphere(_planetCentre, _atmosphereRadius, p, sunDir).y;
    if (sunLength <= 0.0f) 
        return result;

    const AtmosphereOpticalDepth light = atmosphereSunOpticalDepth(height, dot(p - _planetCentre, sunDir), sunLength);
    result.limitReached = light.limitReached;
#if defined(OPTIMZE_OPTICALDEPTH)
    const float3 tau = betaR * (0.5f * segmentDepth.x + light.value.x) + betaM * (0.5f * segmentDepth.y + light.value.y);
    const float3 attenuation = exp(-tau);
    result.inScattering = _sunIntensity * attenuation * (betaR * phaseR * segmentDepth.x + betaM * phaseM * segmentDepth.y);
#else
    const float3 sunTransmittanceR = exp(-betaR * light.value.x);
    const float3 sunTransmittanceM = exp(-betaM * light.value.y);
    const float3 viewTransmittanceR = exp(-betaR * (0.5f * segmentDepth.x));
    const float3 viewTransmittanceM = exp(-betaM * (0.5f * segmentDepth.y));
    const float3 scatteringWeightR = betaR * segmentDepth.x * phaseR;
    const float3 scatteringWeightM = betaM * segmentDepth.y * phaseM;
    const float3 incidentLight = _sunIntensity * sunTransmittanceR * sunTransmittanceM;
    result.inScattering = incidentLight * (scatteringWeightR + scatteringWeightM) * viewTransmittanceR * viewTransmittanceM;
#endif
    return result;
}

AtmosphereSegment calculateAtmosphere(float3 rayOrigin, float3 rayDir, float rayLength, float3 sunDir)
{
    AtmosphereSegment result = atmosphereIdentity();
    if (rayLength <= 0.0f) 
        return result;

    const float mu = clamp(dot(rayDir, sunDir), -1.0f, 1.0f);
    const float g = 0.76f;
    const float phaseR = (3.0f / (16.0f * PI)) * (1.0f + mu * mu);
    const float phaseM = (1.0f - g * g) / (4.0f * PI * pow(1.0f + g * g - 2.0f * g * mu, 1.5f));
    const uint requestedSeeds = clamp(_numInScatteringPoints, 1u, 128u);
    // Keep a denser direct reference while production lets the error estimator allocate work.
    const uint seeds = _atmosphereSamplingMode == ATEMOSPHERE_PRODUCTION ? requestedSeeds : max(requestedSeeds, 32u);
    const uint maxDepth = min(_integrationMaxDepth, 10u);
    const float relativeTolerance = max(_integrationRelativeTolerance, 1e-5f);
    const float height = atmosphereHeight(rayOrigin);
    const float radialDot = dot(rayOrigin - _planetCentre, rayDir);

    // Global work budget, including pending nodes and remaining seeds.
    const uint nodeBudget = 256u;
    uint visitedNodes = 0u;
    float3 stack[12];
    AtmosphereSegment estimateStack[12];
    [loop]
    for (uint seed = 0u; seed < seeds; ++seed)
    {
        uint size = 1u;
        // Resolve the visible near end of long, dense gas paths without raising the budget.
        const float u0 = float(seed) / seeds;
        const float u1 = float(seed + 1u) / seeds;
        stack[0] = float3(rayLength * u0 * u0, rayLength * u1 * u1, 0.0f);
        estimateStack[0] = evaluateAtmosphereInterval(rayOrigin, rayDir, stack[0].x, stack[0].y, sunDir, phaseR, phaseM);
        [loop]
        while (size > 0u)
        {
            ++visitedNodes;

            const float3 node = stack[--size];
            const float a = node.x;
            const float b = node.y;
            const float midpoint = 0.5f * (a + b);
            const uint depth = uint(node.z);
            const float geometrySplit = atmosphereSplitPoint(height, radialDot, a, b);
            const float split = geometrySplit >= 0.0f ? geometrySplit : midpoint;
            // The parent already evaluated this interval when it formed its two-half estimate.
            const AtmosphereSegment coarse = estimateStack[size];
            const AtmosphereSegment left = evaluateAtmosphereInterval(rayOrigin, rayDir, a, split, sunDir, phaseR, phaseM);
            const AtmosphereSegment right = evaluateAtmosphereInterval(rayOrigin, rayDir, split, b, sunDir, phaseR, phaseM);
            const AtmosphereSegment fine = composeAtmosphereIntervals(left, right);
            const float fraction = (b - a) / rayLength;
            const bool transmittanceError = any(abs(fine.transmittance - coarse.transmittance) > 1e-5f * fraction + relativeTolerance * abs(fine.transmittance));
            const bool radianceError = any(abs(fine.inScattering - coarse.inScattering) > 1e-5f * max(_sunIntensity, 1.0f) * fraction + relativeTolerance * abs(fine.inScattering));
            // Two nearly-zero midpoint estimates can otherwise miss all near-end light
            // in an optically thick interval. Keep each accepted optical step small.
            const bool opticallyThick = any(coarse.transmittance < 0.60653066f); // exp(-0.5)
            const bool needsSplit = geometrySplit >= 0.0f || transmittanceError || radianceError || opticallyThick;
            if (needsSplit && depth < maxDepth && split > a && split < b && visitedNodes + size + (seeds - seed - 1u) + 2u <= nodeBudget)
            {
                estimateStack[size] = right;
                stack[size++] = float3(split, b, depth + 1u);
                estimateStack[size] = left;
                stack[size++] = float3(a, split, depth + 1u);
            }
            else
            {
                result = composeAtmosphereIntervals(result, fine);
                result.limitReached |= needsSplit ? 1u : 0u;
                // Approximate termination: remaining camera-path contribution is attenuated
                // by at least one million. Keep the residual T for background composition.
                if (all(result.transmittance < 1e-6f))
                    return result;
            }
        }
    }
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

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    const float nearPlaneHalfHeightWS = _nearDistance * getFOVTangent();
    const float nearPlaneHalfWidthWS = nearPlaneHalfHeightWS * ((float)_resolution.x / (float)_resolution.y);
    const float2 nearPlaneUV = float2(input.uv.x, 1 - input.uv.y) * 2 - 1;
    const float3 rayDirToNearPlane = getViewForwardDirection() * _nearDistance +  getViewRightDirection() * nearPlaneHalfWidthWS * nearPlaneUV.x +  getViewUpDirection() * nearPlaneHalfHeightWS * nearPlaneUV.y;

    // Camera-relative origin.
    const float3 rayOrigin = float3(0, 0, 0);
    const float3 rayDir = normalize(rayDirToNearPlane);

    const Texture2D<float4> renderTexture = getRenderTargetTexture(_frameIndex.x, 0);
    const Texture2D<float4> depthTexture = getDepthStencilTexture(_frameIndex.x, 0);
    const float4 originalCol = renderTexture.Load(uint3(input.uv * _resolution, 0));
    // Debug primitives use the unlit channel and must survive sky/ground composition.
    if (getTexture(_materialIndex).Load(uint3(input.uv * _resolution, 0)).z > 0.5f)
        return float4(originalCol.rgb, 1);

    float3 sunDir = float3(0, -1, 0);
    const float sunCoverage = renderSun(rayDir, sunDir);
    // Uniform disk: irradiance on a facing plane is L * pi * sin(radius)^2.
    const float sinRadius = sin(radians(_sunAngularRadius));
    const float sunRadiance = _sunIntensity * _sunRadianceScale / max(PI * sinRadius * sinRadius, 1e-8f);
    
    const float depth = depthTexture.Load(uint3(input.uv * _resolution, 0)).x;
    // 기본적으로 depthTexture의 Value는 log된 값입니다. 이를 선형적 값으로 변환합니다.
    const float viewZ = (_nearDistance * _farDistance) / (_farDistance - depth * (_farDistance - _nearDistance));
    const float linearDepthDistance = viewZ / max(dot(rayDir, getViewForwardDirection()), 1e-6f);

    const float2 hitInfo = raySphere(_planetCentre, _atmosphereRadius, rayOrigin, rayDir);
    const float destToAtmosphere = hitInfo.x;   // 대기 near 거리 (만약 카메라가 대기 안에 있다면 0)
    const float lengthAtmosphere = hitInfo.y;   // 대기 관통 거리 (만약 카메라가 대기 안에 있다면 far - camerapos)

    float3 backgroundRadiance = depth < 1.0f ? originalCol.rgb : /*float3(1, 1, 1) * */sunRadiance * sunCoverage;

    if(lengthAtmosphere == 0)
        return float4(backgroundRadiance, 1);

    const float atmosphereEnd = destToAtmosphere + lengthAtmosphere;
    const float rayEnd = depth < 1.0f ? min(linearDepthDistance, atmosphereEnd) : atmosphereEnd;
    const float integrationLength = max(0.0f, rayEnd - destToAtmosphere);
    const float3 atmosphereStart = rayOrigin + rayDir * destToAtmosphere;
    
    const AtmosphereSegment atmosphere = calculateAtmosphere(atmosphereStart, rayDir, integrationLength, sunDir);

    if (_atmosphereSamplingMode == ATEMOSPHERE_LIMIT_DEBUG)
        return float4(atmosphere.limitReached != 0u ? float3(1, 0, 0) : float3(0, 0.25f, 0), 1);
        
    const float3 composedHDR = backgroundRadiance * atmosphere.transmittance + atmosphere.inScattering;
    return float4(composedHDR, 1);
}
#endif
