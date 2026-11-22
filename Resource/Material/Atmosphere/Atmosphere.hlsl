#ifndef __DEFINE_ATMOSPHERE_HLSL__
#define __DEFINE_ATMOSPHERE_HLSL__

#include "CommonRendering.hlsl"
#include "CommonTexture.hlsl"
#include "GridHash.hlsl"
#include "Atmosphere/AtmosphereParameters.hlsl"
#include "Atmosphere/AtmosphereTransmittance.hlsl"
#include "Atmosphere/AtmosphereSurfaceLighting.hlsl"
#include "Atmosphere/AtmosphereMultipleScattering.hlsl"
#include "Atmosphere/CloudDensity.hlsl"

cbuffer AtmosphereInputConstants : register(b2)
{
    uint _materialIndex;
    uint _lightingIndex;
    uint _sceneDepthIndex;
};

struct AtmosphereSegment
{
    float3 transmittance;
    float3 inScattering;
    uint limitReached;
    uint workNodes;
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
    result.workNodes = 0u;
    return result;
}

AtmosphereSegment composeAtmosphereIntervals(AtmosphereSegment nearPart, AtmosphereSegment farPart)
{
    AtmosphereSegment result;
    result.transmittance = nearPart.transmittance * farPart.transmittance;
    result.inScattering = nearPart.inScattering + nearPart.transmittance * farPart.inScattering;
    result.limitReached = nearPart.limitReached | farPart.limitReached;
    result.workNodes = nearPart.workNodes + farPart.workNodes;
    return result;
}

float cloudPhase(float mu, float anisotropy)
{
    const float denominator = 1.0f + anisotropy * anisotropy - 2.0f * anisotropy * mu;
    return (1.0f - anisotropy * anisotropy) / (4.0f * PI * denominator * sqrt(denominator));
}

AtmosphereSegment evaluateAtmosphereInterval(float3 origin, float3 direction, float start, float end, float3 sunDir, float phaseR, float phaseM, bool allowReference = true, bool integrateClouds = false, float sampleOffset = 0.0f)
{
    AtmosphereSegment result = atmosphereIdentity();

    const float segmentLength = end - start;
    const float3 p = origin + direction * (0.5f * (start + end) + segmentLength * sampleOffset);
    const float height = atmosphereHeight(p);
    const float4 segmentDepth = atmosphereDensity(height) * segmentLength;
    const float3 betaR = atmosphereRayleighScattering();
    const float3 betaM = atmosphereMieExtinction();
    const float3 betaMScattering = atmosphereMieScattering();
    const float cameraDistance = integrateClouds ? length(p) : 0.0f;
    const float pixelFootprint = 2.0f * getFOVTangent() * cameraDistance / float(_resolution.y);
    const float cloudVolumeFootprint = max(pixelFootprint, segmentLength);
    const float cloudWeatherFootprint = max(pixelFootprint, segmentLength * length(direction.xz));
    const float cloudDepth = integrateClouds ? cloudDensity(p, height, cloudVolumeFootprint, cloudWeatherFootprint) * _cloudExtinction * segmentLength : 0.0f;
    const float3 opticalDepth = atmosphereOpticalExtinction(segmentDepth) + cloudDepth;
    result.transmittance = exp(-opticalDepth);

    const float sunLength = raySphere(_planetCentre, _atmosphereRadius, p, sunDir).y;
    if (sunLength <= 0.0f) 
        return result;

    GPUASSERT(height <= (_atmosphereRadius - _densityReferenceRadius), float4(length(p), (start + end) * 0.5, 0, 0));
    const float sunRadialProjection = atmosphereRadialProjection(p, sunDir, height);
    const AtmosphereOpticalDepth light = atmosphereSunOpticalDepth(height, sunRadialProjection, sunLength, allowReference);
    result.limitReached = light.limitReached;

    const float sunVisibility = atmosphereSurfaceSunVisibility(p);
    const float3 intervalWeight = atmosphereIntervalWeight(opticalDepth);
    const float3 sunIrradiance = atmosphereSunIrradiance() * sunVisibility * exp(-atmosphereOpticalExtinction(light.value));
    float cloudSunTransmittance = 1.0f;
    const float cloudScatteringDepth = cloudDepth * _cloudScatteringAlbedo;
    if (cloudDepth > 0.0f && sunVisibility > 0.0f)
    {
        const float2 planetHit = raySphere(_planetCentre, _densityReferenceRadius, p, sunDir);
        if (planetHit.y > 0.0f && planetHit.x < sunLength)
            cloudSunTransmittance = 0.0f;
        else
        {
            const float cloudSunDepth = cloudSunOpticalDepth(p, sunDir, sunLength, cloudVolumeFootprint, cloudWeatherFootprint);
            cloudSunTransmittance = exp(-cloudSunDepth);
            const float mu = clamp(dot(direction, sunDir), -1.0f, 1.0f);
            // A weaker, broader lobe with reduced extinction approximates higher scattering orders.
            const float singleScattering = cloudPhase(mu, 0.8f) * cloudSunTransmittance;
            const float multipleScattering = 0.35f * cloudPhase(mu, 0.35f) * exp(-0.35f * cloudSunDepth);
            result.inScattering = intervalWeight * sunIrradiance * cloudScatteringDepth * (singleScattering + multipleScattering);
        }
    }
    result.inScattering += intervalWeight * sunIrradiance * cloudSunTransmittance * (betaR * phaseR * segmentDepth.x + betaMScattering * phaseM * segmentDepth.y);

    // Isotropic incident field from prior atmospheric scattering orders.
    const float3 multipleIncident = atmosphereMultipleIncident(height, sunRadialProjection);
    result.inScattering += intervalWeight * multipleIncident * (betaR * segmentDepth.x + betaMScattering * segmentDepth.y + cloudScatteringDepth);

    // First surface bounce, approximated as an isotropic local incident field.
    const float3 surfaceIncident = atmosphereSurfaceBounce(p);
    result.inScattering += intervalWeight * surfaceIncident * (betaR * segmentDepth.x + betaMScattering * segmentDepth.y + cloudScatteringDepth) / (4.0f * PI);
    return result;
}

AtmosphereSegment calculateAtmosphere(float3 rayOrigin, float3 rayDir, float rayLength, float3 sunDir, bool allowReference = true)
{
    AtmosphereSegment result = atmosphereIdentity();
    if (rayLength <= 0.0f) 
        return result;

    const float mu = clamp(dot(rayDir, sunDir), -1.0f, 1.0f);
    const float phaseR = atmosphereRayleighPhase(mu);
    const float phaseM = atmosphereMiePhase(mu);
    const uint requestedSeeds = clamp(_numInScatteringPoints, 1u, 128u);
    // Keep a denser direct reference while production lets the error estimator allocate work.
    const uint seeds = atmosphereUsesLut(allowReference) ? requestedSeeds : max(requestedSeeds, 32u);
    const float height = atmosphereHeight(rayOrigin);
    const float radialDot = atmosphereRadialProjection(rayOrigin, rayDir, height);

    // Global work budget, including pending nodes and remaining seeds.
    const uint nodeBudget = clamp(_viewNodeBudget, 128u, 256u);
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
        stack[0] = float3(rayLength * u0 * u0, rayLength * u1 * u1, 0.0f); // u0*u0, u1*u1을 곱하는 이유는 rayOrigin에 가깝게 sampling하기 위해서이다. 거리가 지수적으로 늘어나도록
        estimateStack[0] = evaluateAtmosphereInterval(rayOrigin, rayDir, stack[0].x, stack[0].y, sunDir, phaseR, phaseM, allowReference);

        [loop]
        while (size > 0u)
        {
            ++visitedNodes;

            const float3 node = stack[--size];
            const float start = node.x;
            const float end = node.y;
            const float midpoint = 0.5f * (start + end);
            const uint depth = uint(node.z);
            const float geometrySplit = atmosphereSplitPoint(height, radialDot, start, end);
            const float split = geometrySplit >= 0.0f ? geometrySplit : midpoint;
            // The parent already evaluated this interval when it formed its two-half estimate.
            const AtmosphereSegment coarse = estimateStack[size];
            const AtmosphereSegment left = evaluateAtmosphereInterval(rayOrigin, rayDir, start, split, sunDir, phaseR, phaseM, allowReference);
            const AtmosphereSegment right = evaluateAtmosphereInterval(rayOrigin, rayDir, split, end, sunDir, phaseR, phaseM, allowReference);
            const AtmosphereSegment fine = composeAtmosphereIntervals(left, right);

            const float fraction = (end - start) / rayLength;
            const bool transmittanceError = any(abs(fine.transmittance - coarse.transmittance) > 1e-5f * fraction + _integrationRelativeTolerance * abs(fine.transmittance));
            const bool radianceError = any(abs(fine.inScattering - coarse.inScattering) > 1e-5f * max(_sunIrradiance, 1.0f) * fraction + _integrationRelativeTolerance * abs(fine.inScattering));
            // Two nearly-zero midpoint estimates can otherwise miss all near-end light
            // in an optically thick interval. Keep each accepted optical step small.
            const bool opticallyThick = any(coarse.transmittance < 0.60653066f); // exp(-0.5)
            const bool needsSplit = geometrySplit >= 0.0f || transmittanceError || radianceError || opticallyThick;
            if (needsSplit && depth < _integrationMaxDepth && split > start && split < end && visitedNodes + size + (seeds - seed - 1u) + 2u <= nodeBudget)
            {
                estimateStack[size] = right;
                stack[size++] = float3(split, end, depth + 1u);
                estimateStack[size] = left;
                stack[size++] = float3(start, split, depth + 1u);
            }
            else
            {
                result = composeAtmosphereIntervals(result, fine);
                result.limitReached |= needsSplit ? 1u : 0u;
                result.workNodes = visitedNodes;
                // Approximate termination: remaining camera-path contribution is attenuated
                // by at least one million. Keep the residual T for background composition.
                if (all(result.transmittance < 1e-6f))
                    return result;
            }
        }
    }
    
    return result;
}

AtmosphereSegment calculateCloudInterval(float3 rayOrigin, float3 rayDir, float start, float end, float3 sunDir, float pixelSampleOffset, out float resolveWeight)
{
    AtmosphereSegment result = atmosphereIdentity();
    resolveWeight = 0.0f;
    const float mu = clamp(dot(rayDir, sunDir), -1.0f, 1.0f);
    const float phaseR = atmosphereRayleighPhase(mu);
    const float phaseM = atmosphereMiePhase(mu);
    static const uint maxStepCount = 96u;
    // Keep half the budget at the regular spacing for the visible part of the cloud.
    static const uint regularStepCount = maxStepCount / 2u;
    static const uint farStepCount = maxStepCount - regularStepCount;
    static const float stepsPerShapePeriod = 64.0f;
    // Two samples per cell of the highest-frequency Worley octave.
    static const float stepsPerDetailPeriod = 2.0f * DK_CLOUD_HIGH_FREQUENCY_CELL_COUNT;
    const float regularStepLength = _cloudShapePeriod / stepsPerShapePeriod;
    const float nearStepLength = min(_cloudDetailPeriod / stepsPerDetailPeriod, regularStepLength);
    const float shapeCellLength = _cloudShapePeriod / DK_CLOUD_LOW_FREQUENCY_CELL_COUNT;
    const float originDistance = length(rayOrigin);

    float stepStart = start;
    float farStepLengthIncrement = 0.0f;
    [loop]
    for (uint stepIndex = 0; stepIndex < maxStepCount && stepStart < end; ++stepIndex)
    {
        if (stepIndex == regularStepCount)
        {
            // Spread only the excess distance across the far steps as an arithmetic series.
            const float excessLength = max(0.0f, end - stepStart - farStepCount * regularStepLength);
            farStepLengthIncrement = (2.0f * excessLength) / float(farStepCount * (farStepCount - 1u));
        }

        const float regularWeight = saturate(stepStart / _cloudDetailPeriod);
        const float farStepLength = regularStepLength + (float(stepIndex) - float(regularStepCount)) * farStepLengthIncrement;
        const float stepLength = stepIndex < regularStepCount ? lerp(nearStepLength, regularStepLength, regularWeight) : farStepLength;
        // Clip the last step and cover any residual distance from float rounding at the budget limit.
        const float stepEnd = stepIndex + 1u == maxStepCount ? end : min(stepStart + stepLength, end);
        // Offset distant samples to break up the shared sampling planes of neighboring rays.
        const float cameraDistance = originDistance + 0.5f * (stepStart + stepEnd);
        resolveWeight = smoothstep(0.5f * shapeCellLength, shapeCellLength, cameraDistance);
        const float sampleOffset = pixelSampleOffset * resolveWeight;
        result = composeAtmosphereIntervals(result, evaluateAtmosphereInterval(rayOrigin, rayDir, stepStart, stepEnd, sunDir, phaseR, phaseM, false, true, sampleOffset));
        if (all(result.transmittance < 1e-6f))
            break;

        stepStart = stepEnd;
    }
    return result;
}

AtmosphereSegment calculateAtmosphereWithClouds(float3 rayOrigin, float3 rayDir, float rayLength, float3 sunDir, bool referencePixel, float pixelSampleOffset, out float resolveWeight)
{
    resolveWeight = 0.0f;
    // The gas reference and diagnostics remain comparable with the clear-atmosphere solver.
    float4 cloudIntervals = 0.0f;
    if (_cloudCoverage > 0.0f && _atmosphereSamplingMode == ATEMOSPHERE_PRODUCTION && rayLength > 0.0f)
    {
        const float2 planetHit = raySphere(_planetCentre, _densityReferenceRadius, rayOrigin, rayDir);
        const bool entersPlanet = planetHit.y > 0.0f && (planetHit.x > 0.0f || dot(rayOrigin - _planetCentre, rayDir) < 0.0f);
        const float cloudRayEnd = entersPlanet ? min(planetHit.x, rayLength) : rayLength;
        cloudIntervals = cloudRayIntervals(rayOrigin, rayDir, cloudRayEnd);
    }

    AtmosphereSegment result = atmosphereIdentity();
    float start = 0.0f;
    [loop]
    for (uint index = 0; index <= 2; ++index)
    {
        const bool hasCloudInterval = index < 2u;
        const float2 interval = index == 0u ? cloudIntervals.xy : cloudIntervals.zw;
        if (hasCloudInterval && interval.y <= interval.x)
            continue;

        const float clearEnd = hasCloudInterval ? interval.x : rayLength;
        result = composeAtmosphereIntervals(result, calculateAtmosphere(rayOrigin + rayDir * start, rayDir, clearEnd - start, sunDir, referencePixel));
        if (hasCloudInterval == false || all(result.transmittance < 1e-6f))
            break;

        float intervalResolveWeight;
        const AtmosphereSegment clouds = calculateCloudInterval(rayOrigin, rayDir, interval.x, interval.y, sunDir, pixelSampleOffset, intervalResolveWeight);
        result = composeAtmosphereIntervals(result, clouds);
        resolveWeight = max(resolveWeight, intervalResolveWeight);
        if (all(result.transmittance < 1e-6f))
            break;
        start = interval.y;
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
    const float3 rayDirToNearPlaneWS = getViewForwardDirection() * _nearDistance +  getViewRightDirection() * nearPlaneHalfWidthWS * nearPlaneUV.x +  getViewUpDirection() * nearPlaneHalfHeightWS * nearPlaneUV.y;

    // Camera-relative origin.
    const float3 rayOrigin = float3(0, 0, 0);
    const float3 rayDirWS = normalize(rayDirToNearPlaneWS);

    const Texture2D<float4> renderTexture = getTexture(_lightingIndex);
    const Texture2D<float4> depthTexture = getTexture(_sceneDepthIndex);
    const float4 originalCol = renderTexture.Load(uint3(input.uv * _resolution, 0));
    // Debug primitives use the unlit channel and must survive sky/ground composition.
    if (getTexture(_materialIndex).Load(uint3(input.uv * _resolution, 0)).z > 0.5f)
        return float4(originalCol.rgb, 0);

    float3 sunDir = float3(0, -1, 0);
    const float sunCoverage = renderSun(rayDirWS, sunDir);
    // Uniform disk: irradiance on a facing plane is L * pi * sin(radius)^2.
    const float sinRadius = sin(radians(_sunAngularRadius));
    const float3 sunRadiance = atmosphereSunIrradiance() * _sunRadianceScale / max(PI * sinRadius * sinRadius, 1e-8f);
    
    const float depth = depthTexture.Load(uint3(input.uv * _resolution, 0)).x;
    // Infinite reversed Z stores near/viewZ; zero is the cleared sky.
    // Skip reconstruction for sky so no division by zero enters integration.
    const bool hasGeometry = depth > 0.0f;
    const float viewZ = hasGeometry ? _cameraProjectionMatrix._43 / (depth - _cameraProjectionMatrix._33) : 0.0f;
    // 카메라 위치로부터의 z위치를 카메라 위치로부터 길이로 변환
    const float linearDepthDistance = viewZ / max(dot(rayDirWS, getViewForwardDirection()), 1e-6f);

    const float2 hitInfo = raySphere(_planetCentre, _atmosphereRadius, rayOrigin, rayDirWS);
    const float destToAtmosphere = hitInfo.x;   // 대기 near 거리 (만약 카메라가 대기 안에 있다면 0)
    const float lengthAtmosphere = hitInfo.y;   // 대기 관통 거리 (만약 카메라가 대기 안에 있다면 far - camerapos)

    float3 backgroundRadiance = hasGeometry ? originalCol.rgb : /*float3(1, 1, 1) * */sunRadiance * sunCoverage;

    // 우주공간
    if(lengthAtmosphere == 0)
        return float4(backgroundRadiance, 0);

    const float atmosphereEnd = destToAtmosphere + lengthAtmosphere;
    const float rayEnd = hasGeometry ? min(linearDepthDistance, atmosphereEnd) : atmosphereEnd;
    const float integrationLength = max(0.0f, rayEnd - destToAtmosphere);
    const float3 atmosphereStart = rayOrigin + rayDirWS * destToAtmosphere;
    
    // Restrict nested direct integration to a 16x16 centre patch. Surrounding pixels use the LUT.
    const bool referencePixel = all(abs(input.position.xy - 0.5f * float2(_resolution)) < 8.0f);
    const bool resolveClouds = hasGeometry == false && sunCoverage <= 0.0f;
    const int2 pixel = int2(input.position.xy);
    // Each 2x2 block covers four sample strata. Hash the starting phase per block without changing it across frames.
    const uint sampleStratum = 2u * uint((pixel.x ^ pixel.y) & 1) + uint(pixel.y & 1);
    const float blockPhase = float(HashGridCell(pixel >> 1) >> 8u) * (1.0f / 16777216.0f);
    const float pixelSampleOffset = resolveClouds ? 0.25f * (sampleStratum + blockPhase) - 0.5f : 0.0f;
    float resolveWeight;
    const AtmosphereSegment atmosphere = calculateAtmosphereWithClouds(atmosphereStart, rayDirWS, integrationLength, sunDir, referencePixel, pixelSampleOffset, resolveWeight);

    if (_atmosphereSamplingMode == ATEMOSPHERE_LIMIT_DEBUG && referencePixel)
        return float4(atmosphere.limitReached != 0u ? float3(1, 0, 0) : float3(0, 0.25f, 0), 0);
    if (_atmosphereSamplingMode == ATMOSPHERE_WORK_DEBUG)
    {
        const float work = float(atmosphere.workNodes) / clamp(float(_viewNodeBudget), 128.0f, 256.0f);
        return float4(work, atmosphere.limitReached != 0u ? 0.0f : 0.25f, 1.0f - work, 0);
    }
    if (_atmosphereSamplingMode == ATMOSPHERE_TRANSMITTANCE_DEBUG)
        return float4(atmosphere.transmittance, 0);
        
    const float3 composedHDR = backgroundRadiance * atmosphere.transmittance + atmosphere.inScattering;
    // Alpha carries the sky-cloud resolve weight through bloom to the final HDR resolve.
    return float4(composedHDR, resolveClouds ? resolveWeight : 0.0f);
}
#endif
