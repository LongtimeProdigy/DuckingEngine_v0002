#include "CommonTexture.hlsl"
#include "Atmosphere/CloudLimits.h"

cbuffer CloudNoiseOutputConstants : register(b2)
{
    uint _cloudNoiseUav;
    uint _cloudWeatherUav;
};

uint3 cloudCellHash(int3 cell, uint count)
{
    // Power-of-two cell counts wrap integer lattice coordinates at the texture boundary.
    uint3 value = uint3(cell) & (count - 1u);
    // PCG3D spatial hash of the wrapped lattice coordinate.
    value = value * 1664525u + 1013904223u;
    value.x += value.y * value.z;
    value.y += value.z * value.x;
    value.z += value.x * value.y;
    value ^= value >> 16u;
    value.x += value.y * value.z;
    value.y += value.z * value.x;
    value.z += value.x * value.y;
    return value;
}

float3 cloudCellRandom(int3 cell, uint count)
{
    // Convert the upper 24 hash bits to exactly representable values in [0, 1).
    return float3(cloudCellHash(cell, count) >> 8u) * (1.0f / 16777216.0f);
}

float cloudPerlin(float3 uvw, uint count)
{
    const float3 position = uvw * count;
    const int3 cell = int3(floor(position));
    const float3 fraction = frac(position);
    const float3 weight = fraction * fraction * fraction * (fraction * (6.0f * fraction - 15.0f) + 10.0f);
    float value = 0.0f;
    [unroll]
    for (uint corner = 0; corner < 8; ++corner)
    {
        const int3 offset = int3(corner & 1u, (corner >> 1u) & 1u, corner >> 2u);
        const float3 gradient = normalize(cloudCellRandom(cell + offset, count) * 2.0f - 1.0f);
        const float3 cornerWeight = lerp(1.0f - weight, weight, float3(offset));
        value += cornerWeight.x * cornerWeight.y * cornerWeight.z * dot(gradient, fraction - offset);
    }
    return saturate(0.5f + value);
}

float cloudWorley(float3 uvw, uint count)
{
    const float3 position = uvw * count;
    const int3 cell = int3(floor(position));
    const float3 fraction = frac(position);
    // Distances outside one lattice cell are discarded by the final remap.
    float distanceSquared = 1.0f;
    [unroll]
    for (int z = -1; z <= 1; ++z)
    {
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            [unroll]
            for (int x = -1; x <= 1; ++x)
            {
                const int3 offset = int3(x, y, z);
                const float3 delta = float3(offset) + cloudCellRandom(cell + offset, count) - fraction;
                distanceSquared = min(distanceSquared, dot(delta, delta));
            }
        }
    }
    return 1.0f - sqrt(distanceSquared);
}

[numthreads(4, 4, 4)]
void CSVolume(uint3 id : SV_DispatchThreadID)
{
    const float3 uvw = (float3(id) + 0.5f) / DK_CLOUD_NOISE_SIZE;
    const float lowPerlin = cloudPerlin(uvw, DK_CLOUD_LOW_FREQUENCY_CELL_COUNT);
    const float lowWorley = cloudWorley(uvw, DK_CLOUD_LOW_FREQUENCY_CELL_COUNT);
    const float midWorley = cloudWorley(uvw, DK_CLOUD_MID_FREQUENCY_CELL_COUNT);
    // Low frequencies define the puffs. Keep their weights when the finer octaves are replaced by their means.
    const float perlin = 0.65f * lowPerlin + 0.25f * cloudPerlin(uvw, DK_CLOUD_MID_FREQUENCY_CELL_COUNT) + 0.10f * cloudPerlin(uvw, DK_CLOUD_HIGH_FREQUENCY_CELL_COUNT);
    const float worley = 0.7f * lowWorley + 0.3f * midWorley;
    const float shape = lerp(worley, 1.0f, perlin);
    const float detail = 0.7f * midWorley + 0.3f * cloudWorley(uvw, DK_CLOUD_HIGH_FREQUENCY_CELL_COUNT);
    const float broadShape = lerp(0.7f * lowWorley + 0.3f * DK_CLOUD_WORLEY_MEAN, 1.0f, 0.65f * lowPerlin + 0.35f * DK_CLOUD_PERLIN_MEAN);
    const float broadDetail = 0.7f * midWorley + 0.3f * DK_CLOUD_WORLEY_MEAN;
    getTexture3DRW(_cloudNoiseUav)[id] = float4(shape, detail, broadShape, broadDetail);
}

[numthreads(8, 8, 1)]
void CSWeather(uint2 id : SV_DispatchThreadID)
{
    const float2 uv = (float2(id) + 0.5f) / DK_CLOUD_WEATHER_SIZE;
    // Independent slices separate regional coverage from the cloud-type distribution.
    const float3 coverageUVW = float3(uv, 0.31f);
    const float3 typeUVW = float3(uv.yx, 0.71f);
    const float lowCoverage = 0.65f * cloudPerlin(coverageUVW, DK_CLOUD_LOW_FREQUENCY_CELL_COUNT);
    const float coverage = lowCoverage + 0.25f * cloudPerlin(coverageUVW, DK_CLOUD_MID_FREQUENCY_CELL_COUNT) + 0.10f * cloudPerlin(coverageUVW, DK_CLOUD_HIGH_FREQUENCY_CELL_COUNT);
    const float type = cloudPerlin(typeUVW, DK_CLOUD_LOW_FREQUENCY_CELL_COUNT);
    const float broadCoverage = lowCoverage + 0.35f * DK_CLOUD_PERLIN_MEAN;
    getTextureRW(_cloudWeatherUav)[id] = float4(smoothstep(0.30f, 0.68f, coverage), smoothstep(0.24f, 0.64f, type), smoothstep(0.30f, 0.68f, broadCoverage), 0.0f);
}
