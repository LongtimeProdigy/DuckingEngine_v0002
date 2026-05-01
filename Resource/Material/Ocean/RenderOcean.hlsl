#ifndef __DEFINE_HEIGHTOUTPUT_HLSL__
#define __DEFINE_HEIGHTOUTPUT_HLSL__

#include "CommonTexture.hlsl"
#include "Ocean/OceanCommon.hlsl"
#include "CommonRendering.hlsl"

struct VS_INPUT
{
    float2 position : POSITION;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    const float2 localPosition = input.position;

    Texture2D<float4> heightTexture = getTexture(_htSRV);
    uint2 texSize;
    heightTexture.GetDimensions(texSize.x, texSize.y);

    const float meterPerTexel = 1.0 / _L;
    float2 uv =  (localPosition * meterPerTexel);
    uv = abs(uv % 1.0f);
    float4 height = heightTexture.Load(uint3(uv * texSize, 0));

    output.position = float4(localPosition.x, height.x, localPosition.y, 1.0f);

    output.position = mul(output.position, _cameraWorldMatrixInv);
    output.position = mul(output.position, _cameraProjectionMatrix);

    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    // 에메랄드색
    return float4(37 / 255.0, 241 / 255.0, 198 / 255.0, 1);
}

#endif