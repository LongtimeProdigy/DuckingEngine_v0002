#ifndef __DEFINE_GBUFFER_HLSL__
#define __DEFINE_GBUFFER_HLSL__

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

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    const Texture2D renderTexture = getRenderTargetTexture(_frameIndex.x, 1);
    const float4 originalCol = renderTexture.Load(uint3(input.uv * _resolution, 0));
    return originalCol;
}

#endif