#ifndef __DEFINE_SKINNEDMESHSTANDARD_HLSL__
#define __DEFINE_SKINNEDMESHSTANDARD_HLSL__

#include "CommonTexture.hlsl"
#include "CommonRendering.hlsl"

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv0 : TEXCOORD0;
    uint4 boneIndexes : BONEINDEXES;
    float4 boneWeights : BONEWEIGHTS;
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

cbuffer SkinnedMeshStandard : register(b2)
{
    TextureParameter _diffuseTexture;
    float _opacity;
}

// #todo- StructuredBuffer로 교체 가능?
#define MAX_BONE_COUNT 64
#define MAX_SKINNING_COUNT 4
cbuffer SkeletonConstantBuffer : register(b3)
{
    float4x4 _skeletonConstantBuffer[MAX_BONE_COUNT];
}

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = float4(input.position, 1.0f);
    output.normal = float4(input.normal, 1.0f);
    output.uv0 = input.uv0;

    // Skinning
    float4x4 skinMatrix = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    for(uint i = 0; i < MAX_SKINNING_COUNT; ++i)
    {
        if(input.boneIndexes[i] == 0xffffffff)
            continue;

        if(input.boneWeights[i] == 0)
            continue;

        skinMatrix += _skeletonConstantBuffer[input.boneIndexes[i]] * input.boneWeights[i];
    }

    output.position = mul(output.position, skinMatrix);
    output.position = mul(output.position, _worldMatrix);
    output.position = mul(output.position, _cameraWorldMatrix);
    output.position = mul(output.position, _cameraProjectionMatrix);

    output.normal = mul(output.normal, _cameraWorldMatrix);
    
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    Texture2D diffuseTexture = gBindlessTextureArray[_diffuseTexture];
    return diffuseTexture.Sample(normalSampler, input.uv0);
}

#endif