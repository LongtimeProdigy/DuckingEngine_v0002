#ifndef __DEFINE_SKYDOME_HLSL__
#define __DEFINE_SKYDOME_HLSL__

#include "CommonRendering.hlsl"
#include "GBufferCommon.hlsl"

struct VS_INPUT
{
    float3 position : POSITION;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    float4x4 tempPosition = _cameraWorldMatrixInv;
    tempPosition._41_42_43_44 = float4(0, 0, 0, 1); // SkyDome이 항상 카메라를 따라가도록

    output.position = float4(input.position, 1.0f);
    output.position = mul(output.position, tempPosition);
    output.position = mul(output.position, _cameraProjectionMatrix);

    return output;
}

GBufferOutput PSMain(VS_OUTPUT input)
{
    return writeGBuffer(float3(0, 1, 1), float3(0, 1, 0), 0, 1, 1);
}

#endif