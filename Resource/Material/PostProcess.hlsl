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

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    Texture2D renderTexture = getRenderTargetTexture(_frameIndex.x);
    Texture2D depthTexture = getDepthStencilTexture();
    
    //Atmosphere PostProcessing
    // 행성 원점 (0, 0, 0) 가져오기
    // 대기 반지름 (1unit = 1m >> 30000  = 30km) [10km 대류권, 50km 성층권, 80km 중간권, 600km 열권]

    return renderTexture.Sample(normalSampler, input.uv);
}

#endif