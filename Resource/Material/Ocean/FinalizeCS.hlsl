#ifndef __DEFINE_FFTBUTTERFLY_HLSL__
#define __DEFINE_FFTBUTTERFLY_HLSL__

#include "CommonTexture.hlsl"
#include "Ocean/OceanCommon.hlsl"

cbuffer FinalizeCB : register(b0)
{
    TextureParameter _sourceSRV;
};

[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID)
{
    Texture2D<float4> FFTResult = getTexture(_sourceSRV);
    const float2 h = FFTResult[id.xy].xy;

    // FFT shift correction
    const float sign = ((id.x + id.y) & 1) ? -1.0 : 1.0;
    float height = h.x * sign;

    // normalization
    height /= (_N * _N);

    height *= _heightScale;

    RWTexture2D<float4> Height = getTextureRW(_heightUAV);
    Height[id.xy] = float4(height, 0, 0, 0);
}

#endif