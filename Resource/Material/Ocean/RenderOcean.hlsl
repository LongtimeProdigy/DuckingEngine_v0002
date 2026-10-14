#ifndef __DEFINE_HEIGHTOUTPUT_HLSL__
#define __DEFINE_HEIGHTOUTPUT_HLSL__

#include "CommonTexture.hlsl"
#include "Ocean/OceanCommon.hlsl"
#include "CommonRendering.hlsl"
#include "GBufferCommon.hlsl"

struct VS_INPUT
{
    float2 position : POSITION;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD1;
    float2 uv : UV0;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    const float2 localPosition = input.position;

    Texture2D<float4> heightTexture = getTexture(_heightSRV);
    uint2 texSize;
    heightTexture.GetDimensions(texSize.x, texSize.y);

    const float meterPerTexel = 1.0 / float(_length);
    const float2 uv =  (localPosition * meterPerTexel);
    //uv = abs(uv % 1.0f);
    const float height = heightTexture.Load(uint3(uv * texSize, 0)).x;

    output.worldPos = float3(localPosition.x, height, localPosition.y);

    output.position = float4(output.worldPos, 1.0);
    output.position = mul(output.position, _cameraWorldMatrixInv);
    output.position = mul(output.position, _cameraProjectionMatrix);

    output.uv = uv;

    return output;
}

GBufferOutput PSMain(VS_OUTPUT input)
{
    const float3 normal = getTexture(_normalSRV).Sample(bilinearRepeatSampler, input.uv).xyz * 2 - 1;
    return writeGBuffer(float3(0.01f, 0.05f, 0.1f), normal, 0, 0.12f);
}
#endif