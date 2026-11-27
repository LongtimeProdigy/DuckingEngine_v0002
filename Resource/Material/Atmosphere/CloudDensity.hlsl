#ifndef DK_CLOUD_DENSITY_HLSL
#define DK_CLOUD_DENSITY_HLSL

#include "CommonRendering.hlsl"
#include "CommonTexture.hlsl"
#include "Atmosphere/AtmosphereGeometry.hlsl"
#include "Atmosphere/CloudLimits.h"

#define DK_SHADER_CLOUD
#include "ShaderStructures.h"
#undef DK_SHADER_CLOUD

// 광선의 [0, rayEnd] 범위에서 바깥 구 내부 구간을 구한 뒤, 안쪽 구 내부 구간을 빼서 구름층 통과 구간을 얻는다.
// .xy는 앞쪽 구간, .zw는 뒤쪽 구간의 시작·끝 거리(m)이다. 거리는 origin 기준이다.
// 시작과 끝이 같으면 빈 구간이다. 구름층 아래에서 위를 보면 .xy는 비고 .zw만 유효하다.
// 우주에서 본다고 가정하고 "광선 → [가까운 쪽 구름층 .xy] → 구름층 아래 공간 → [반대편 구름층 .zw]"같이 통과하면 zw가 필요해짐 (구름층이 2개가됨)
float4 cloudRayIntervals(float3 origin, float3 direction, float rayEnd)
{
    const float2 outer = raySphere(_planetCentre, _densityReferenceRadius + _cloudTopHeight, origin, direction);
    if (outer.y <= 0.0f || outer.x >= rayEnd)
        return 0.0f;

    const float end = min(outer.x + outer.y, rayEnd);
    const float2 inner = raySphere(_planetCentre, _densityReferenceRadius + _cloudBaseHeight, origin, direction);
    if (inner.y <= 0.0f || inner.x >= end)
        return float4(outer.x, end, end, end);

    GPUASSERT(outer.x <= inner.x);
    return float4(outer.x, inner.x, min(inner.x + inner.y, end), end);
}

float cloudResolvedNoiseWeight(float footprint, float period, uint cellCount)
{
    // Start attenuating at half a cell and fade the octave out across the next cell.
    return 1.0f - smoothstep(0.5f, 1.5f, footprint * cellCount / period);
}

float cloudDensity(float3 positionRelative, float height, float volumeFootprint, float weatherFootprint)
{
    const float heightRatio = (height - _cloudBaseHeight) / (_cloudTopHeight - _cloudBaseHeight);
    if (heightRatio <= 0.0f || heightRatio >= 1.0f)
        return 0.0f;

    const float2 worldXZ = getViewPosition().xz + positionRelative.xz - _cloudWindSpeedXZ * _time;
    const float3 weather = getTexture(_cloudWeatherIndex).SampleLevel(bilinearRepeatSampler, worldXZ / _cloudWeatherPeriod, 0.0f).rgb;
    const float weatherWeight = cloudResolvedNoiseWeight(weatherFootprint, _cloudWeatherPeriod, DK_CLOUD_HIGH_FREQUENCY_CELL_COUNT);
    const float weatherCoverage = lerp(weather.b, weather.r, weatherWeight);
    // A signed coverage offset makes 0 and 1 the clear and fully covered endpoints.
    const float coverage = saturate(weatherCoverage + (2.0f * _cloudCoverage - 1.0f));
    if (coverage <= 0.0f)
        return 0.0f;

    // Stratocumulus stays low and broad. Cumulus fades more gradually toward the top.
    const float profile = smoothstep(0.0f, 0.12f, heightRatio) * (1.0f - smoothstep(lerp(0.36f, 0.64f, weather.y), lerp(0.64f, 1.0f, weather.y), heightRatio));
    const float3 cloudPosition = float3(worldXZ.x, height - _cloudBaseHeight, worldXZ.y);
    const Texture3D<float4> noise = getTexture3D(_cloudNoiseIndex);
    const float shapeMean = lerp(DK_CLOUD_WORLEY_MEAN, 1.0f, DK_CLOUD_PERLIN_MEAN);
    const float shapeWeight = cloudResolvedNoiseWeight(volumeFootprint, _cloudShapePeriod, DK_CLOUD_LOW_FREQUENCY_CELL_COUNT);
    const float shapeDetailWeight = cloudResolvedNoiseWeight(volumeFootprint, _cloudShapePeriod, DK_CLOUD_HIGH_FREQUENCY_CELL_COUNT);
    float shape = shapeMean;
    if (shapeWeight > 0.0f)
    {
        const float2 shapeBands = noise.SampleLevel(bilinearRepeatSampler, cloudPosition / _cloudShapePeriod, 0.0f).rb;
        shape = lerp(shapeMean, lerp(shapeBands.y, shapeBands.x, shapeDetailWeight), shapeWeight);
    }
    const float baseDensity = saturate((shape * profile - (1.0f - coverage)) / coverage);
    if (baseDensity <= 0.0f)
        return 0.0f;

    // Erode the silhouette while leaving the densest cores intact.
    const float erosionWeight = cloudResolvedNoiseWeight(volumeFootprint, _cloudDetailPeriod, DK_CLOUD_MID_FREQUENCY_CELL_COUNT);
    float erosionNoise = DK_CLOUD_WORLEY_MEAN;
    if (erosionWeight > 0.0f)
    {
        const float detailWeight = cloudResolvedNoiseWeight(volumeFootprint, _cloudDetailPeriod, DK_CLOUD_HIGH_FREQUENCY_CELL_COUNT);
        const float2 erosionBands = noise.SampleLevel(bilinearRepeatSampler, cloudPosition / _cloudDetailPeriod, 0.0f).ga;
        erosionNoise = lerp(erosionNoise, lerp(erosionBands.y, erosionBands.x, detailWeight), erosionWeight);
    }
    const float erosion = 0.25f * erosionNoise;
    return saturate((baseDensity - erosion) / (1.0f - erosion));
}

float cloudSunOpticalDepth(float3 positionRelative, float3 sunDirection, float sunRayEnd, float volumeFootprint, float weatherFootprint)
{
    const float4 intervals = cloudRayIntervals(positionRelative, sunDirection, sunRayEnd);
    // Keep the directional trace bounded per view sample. Quadratic steps resolve nearby occluders.
    static const uint sunStepCount = 8u;
    const float horizontalDirectionLength = length(sunDirection.xz);
    float columnDensity = 0.0f;
    [unroll]
    for (uint intervalIndex = 0; intervalIndex < 2; ++intervalIndex)
    {
        const float start = intervals[intervalIndex * 2];
        const float lengthMetres = intervals[intervalIndex * 2 + 1] - start;
        if (lengthMetres <= 0.0f)
            continue;

        [loop]
        for (uint step = 0; step < sunStepCount; ++step)
        {
            const float u0 = float(step) / sunStepCount;
            const float u1 = float(step + 1u) / sunStepCount;
            const float t0 = lengthMetres * u0 * u0;
            const float t1 = lengthMetres * u1 * u1;
            const float3 p = positionRelative + sunDirection * (start + 0.5f * (t0 + t1));
            const float sunStepLength = t1 - t0;
            columnDensity += cloudDensity(p, atmosphereHeight(p), max(volumeFootprint, sunStepLength), max(weatherFootprint, sunStepLength * horizontalDirectionLength)) * sunStepLength;
        }
    }
    return _cloudExtinction * columnDensity;
}

#endif
