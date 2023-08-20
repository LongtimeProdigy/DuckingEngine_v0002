#ifndef __DEFINE_STATICMESHSTANDARD_HLSL__
#define __DEFINE_STATICMESHSTANDARD_HLSL__

#include "CommonTexture.hlsl"
#include "CommonRendering.hlsl"

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv0 : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    float2 uv0 : TEXCOORD0;
};

cbuffer SceneObjectConstantBuffer : register(b1)
{
    float4x4 _worldMatrix;
}

cbuffer StaticMeshStandard : register(b2)
{
    TextureParameter _diffuseTexture;
    float _opacity;
}

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = float4(input.position, 1.0f);
    output.normal = float4(input.normal, 1.0f);
    output.uv0 = input.uv0;

    output.position = mul(output.position, _worldMatrix);
    output.position = mul(output.position, _cameraWorldMatrixInv);
    output.position = mul(output.position, _cameraProjectionMatrix);

    output.normal = mul(output.normal, _cameraWorldMatrixInv);

    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    Texture2D diffuseTexture = gBindlessTextureArray[_diffuseTexture];
    return diffuseTexture.Sample(normalSampler, input.uv0);
}

#endif