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

// Convert the upper 24 hash bits to exactly representable values in [0, 1). 16777216.0f == 1000000b -> 0은 포함하되 1은 포함하지않음
float3 cloudCellRandom(int3 cell, uint count)
{
    return float3(cloudCellHash(cell, count) >> 8u) * (1.0f / 16777216.0f);
}
// 3D 공간의 한 위치(uvw)에서 0~1 사이의 부드러운 노이즈 값 하나를 계산, count값이 클수록 패턴이 촘촘해짐
float cloudPerlin(float3 uvw, uint count)
{
    const float3 position = uvw * count;
    const int3 cell = int3(floor(position));
    const float3 fraction = frac(position);
    const float3 weight = fraction * fraction * fraction * (fraction * (6.0f * fraction - 15.0f) + 10.0f);  // 0과 1에서 기울이가 0인 함수
    float value = 0.0f;
    [unroll]
    for (uint corner = 0; corner < 8; ++corner)
    {
        const int3 offset = int3(corner & 1u, (corner >> 1u) & 1u, corner >> 2u);
        const float3 gradient = normalize(cloudCellRandom(cell + offset, count) * 2.0f - 1.0f);         // 그 좌표를 해시해서 만든 고정된 단위 방향 벡터
        const float3 distance = fraction - offset;                                                      // cell에서 샘플까지의 변위 벡터
        const float cornerContribution = dot(gradient, distance);                                       // 샘플이 꼭짓점의 화살표 방향으로 얼마나 일치하는지(이동하는지)
        const float3 cornerWeight = lerp(1.0f - weight, weight, float3(offset));                        // 1.0f - weight는 옆 cell끼리 이어질 수 있도록 하는 처리 (바로 주변 cell은 weight를 사용하므로 경계면 uvw는 같은 수치를 지닐 수 있음)
        value += cornerWeight.x * cornerWeight.y * cornerWeight.z * cornerContribution;
    }
    return saturate(0.5f + value);
}
// 보로노이 노이즈, position 근처 9개 꼭짓점에서 최소길이를 뽑는 알고리즘, count값이 클수록 패턴이 촘촘해짐
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
                const float3 delta = float3(offset) + cloudCellRandom(cell + offset, count) - fraction; // position이 원점이라 치고 계산된 각 꼭지점에서의 벡터를 fraction 공간으로 이동
                distanceSquared = min(distanceSquared, dot(delta, delta));
            }
        }
    }

    // 색반전
    return 1.0f - sqrt(distanceSquared);
}

[numthreads(4, 4, 4)]
void CSVolume(uint3 id : SV_DispatchThreadID)
{
    const float3 uvw = (float3(id) + 0.5f) / DK_CLOUD_NOISE_SIZE;

    // 큰 스케일의 Worley를 Perlin과 섞어 덩어리 형태를 만들고, 작은 스케일의 Worley로 밀도 경계를 깎아 작은 굴곡을 만듦
    const float lowPerlin = cloudPerlin(uvw, DK_CLOUD_LOW_FREQUENCY_CELL_COUNT);
    const float midPerlin = cloudPerlin(uvw, DK_CLOUD_MID_FREQUENCY_CELL_COUNT);
    const float highPerlin = cloudPerlin(uvw, DK_CLOUD_HIGH_FREQUENCY_CELL_COUNT);
    const float lowWorley = cloudWorley(uvw, DK_CLOUD_LOW_FREQUENCY_CELL_COUNT);
    const float midWorley = cloudWorley(uvw, DK_CLOUD_MID_FREQUENCY_CELL_COUNT);
    const float highWorley = cloudWorley(uvw, DK_CLOUD_HIGH_FREQUENCY_CELL_COUNT);

    const float perlin = 0.65f * lowPerlin + 0.25f * midPerlin + 0.10f * highPerlin;
    const float worley = 0.7f * lowWorley + 0.3f * midWorley;
    const float shape = lerp(worley, 1.0f, perlin);
    const float detail = 0.7f * midWorley + 0.3f * highWorley;
    // broadShape와 broadDetail은 세밀한 노이즈 일부를 평균값으로 바꿔 만든 버전, 먼 구름이나 샘플 간격이 넓은 구간에서 이쪽을 더 많이 사용하면 미세한 패턴이 깜빡이는 현상을 줄일 수 있음
    const float broadShape = lerp(0.7f * lowWorley + 0.3f * DK_CLOUD_WORLEY_MEAN, 1.0f, 0.65f * lowPerlin + 0.35f * DK_CLOUD_PERLIN_MEAN);
    const float broadDetail = 0.7f * midWorley + 0.3f * DK_CLOUD_WORLEY_MEAN;
    getTexture3DRW(_cloudNoiseUav)[id] = float4(shape, detail, broadShape, broadDetail);
}

[numthreads(8, 8, 1)]
void CSWeather(uint2 id : SV_DispatchThreadID)
{
    // 월드의 수평 위치마다 구름이 얼마나 많이 생길지, 위로 얼마나 높게 자랄지를 정하는 256×256 날씨 맵

    const float2 uv = (float2(id) + 0.5f) / DK_CLOUD_WEATHER_SIZE;
    // Independent slices separate regional coverage from the cloud-type distribution.
    const float3 coverageUVW = float3(uv, 0.31f);
    const float3 typeUVW = float3(uv.yx, 0.71f);
    const float lowCoverage = 0.65f * cloudPerlin(coverageUVW, DK_CLOUD_LOW_FREQUENCY_CELL_COUNT);
    // 값이 높을수록 3D 형태 패턴에서 더 많은 부분이 구름 밀도로 남습니다.
    const float coverage = lowCoverage + 0.25f * cloudPerlin(coverageUVW, DK_CLOUD_MID_FREQUENCY_CELL_COUNT) + 0.10f * cloudPerlin(coverageUVW, DK_CLOUD_HIGH_FREQUENCY_CELL_COUNT);
    // 값이 낮으면 위쪽 밀도가 일찍 줄고, 높으면 더 높은 곳까지 구름이 남습니다.
    const float type = cloudPerlin(typeUVW, DK_CLOUD_LOW_FREQUENCY_CELL_COUNT);
    // 작은 변화를 줄인 broadCoverage: 멀거나 샘플 간격이 넓을 때 R 대신 이 값을 더 많이 섞습니다.
    const float broadCoverage = lowCoverage + 0.35f * DK_CLOUD_PERLIN_MEAN;
    // R/B로 해당 지역에 구름이 얼마나 남을지 정하고, G로 높이 프로파일을 결정한 뒤, CSVolume()의 3D 패턴과 결합해 현재 샘플의 실제 구름 밀도를 구함
    getTextureRW(_cloudWeatherUav)[id] = float4(smoothstep(0.30f, 0.68f, coverage), smoothstep(0.24f, 0.64f, type), smoothstep(0.30f, 0.68f, broadCoverage), 0.0f);
}
