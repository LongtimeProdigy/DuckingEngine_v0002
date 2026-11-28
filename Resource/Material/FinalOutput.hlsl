#ifndef FINAL_OUTPUT_HLSL
#define FINAL_OUTPUT_HLSL

#include "PostProcessCommon.hlsl"
#include "Atmosphere/AtmosphereParameters.hlsl"

cbuffer FinalOutputConstants : register(b2)
{
    uint _sourceTextureIndex;
};

// 노출을 적용한 선형 HDR RGB에 채널별로 ACES 근사 곡선을 적용해 밝은 영역을 압축한다.
// 출력은 [0, 1] 범위의 디스플레이 선형 RGB이며, sRGB 인코딩은 linearToSrgb에서 적용한다.
float3 toneMapACES(float3 x)
{
    // Equivalent ACES fit, rearranged to remain finite at high solar radiance.
    x = max(x, 0.0f);
    return saturate((2.51f * x + 0.03f) / (2.43f * x + 0.59f + 0.14f / max(x, 1e-20f)));
}

// 디스플레이 선형 RGB의 한 채널에 sRGB 전달 함수를 적용한다.
// 출력은 [0, 1] 범위의 비선형 sRGB 코드값이다. SDR 백버퍼의 UNORM 뷰는 이 값을 8비트로 저장한다.
float linearToSrgb(float channel)
{
    return channel <= 0.0031308f ? 12.92f * channel : 1.055f * pow(channel, 1.0f / 2.4f) - 0.055f;
}

float3 resolveCloudSamples(Texture2D<float4> source, int2 pixel, float4 center)
{
    if (center.a <= 0.0f)
        return center.rgb;

    // The 3x3 tent gives equal weight to the four jitter strata. Zero-alpha samples include opaque geometry and the sun disk.
    float3 sum = 0.0f;
    float weightSum = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            const float4 neighbor = loadClampedRGBA(source, pixel + int2(x, y));
            const float weight = neighbor.a > 0.0f ? float((2 - abs(x)) * (2 - abs(y))) : 0.0f;
            sum += neighbor.rgb * weight;
            weightSum += weight;
        }
    }

    return lerp(center.rgb, sum / weightSum, center.a);
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    const Texture2D<float4> renderTexture = getTexture(_sourceTextureIndex);
    const int2 pixel = int2(input.position.xy);
    const float4 original = loadClampedRGBA(renderTexture, pixel);
    const float3 originalCol = resolveCloudSamples(renderTexture, pixel, original);
    const float3 exposedHDR = max(originalCol, 0.0f) * exp2(_exposureEV);
    const float3 displayLinear = toneMapACES(exposedHDR);
    return float4(linearToSrgb(displayLinear.r), linearToSrgb(displayLinear.g), linearToSrgb(displayLinear.b), 1);
}

#endif
