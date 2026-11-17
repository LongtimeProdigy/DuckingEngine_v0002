#ifndef __DEFINE_SKINNEDMESHSTANDARD_HLSL__
#define __DEFINE_SKINNEDMESHSTANDARD_HLSL__

#include "SkinningLimits.h"
#include "CommonTexture.hlsl"
#include "CommonRendering.hlsl"
#include "GBufferCommon.hlsl"

struct VS_INPUT
{
    float3 _position : POSITION;
    float3 _normal : NORMAL;
    float2 _uv : TEXCOORD0;
    uint4 _boneIndices : BONEINDEXES;
    float4 _boneWeights : BONEWEIGHTS;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 _normal : NORMAL;
    float2 _uv : TEXCOORD0;
};

cbuffer SceneObjectConstantBuffer : register(b1)
{
    float4x4 _worldMatrix;
}

cbuffer SkinnedMeshStandard : register(b2)
{
    TextureParameter _diffuseTexture;
    float _opacity;
    float _metallic;
    float _roughness;
}

// #todo- StructuredBuffer로 교체 가능?
#define MAX_BONE_COUNT DK_MAX_BONE_COUNT
cbuffer SkeletonConstantBuffer : register(b3)
{
    float4x4 _skeletonConstantBuffer[MAX_BONE_COUNT];
}

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = float4(input._position, 1.0f);
    output._normal = float4(input._normal, 0.0f);
    output._uv = input._uv;

    // Skinning
    float4x4 skinMatrix = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    for(uint i = 0; i < DK_MAX_SKINNING_COUNT; ++i)
    {
        if(input._boneIndices[i] >= MAX_BONE_COUNT)
            continue;

        if(input._boneWeights[i] == 0)
            continue;

        skinMatrix += _skeletonConstantBuffer[input._boneIndices[i]] * input._boneWeights[i];
    }

    output.position = mul(output.position, skinMatrix);
    output.position = mul(output.position, _worldMatrix);
    output.position = mul(output.position, _cameraWorldMatrixInv);
    output.position = mul(output.position, _cameraProjectionMatrix);

    output._normal = float4(transformNormal(input._normal, (float3x3)mul(skinMatrix, _worldMatrix)), 0);

    return output;
}

GBufferOutput PSMain(VS_OUTPUT input)
{
    Texture2D<float4> diffuseTexture = getTexture(_diffuseTexture);
    const float4 albedo = diffuseTexture.Sample(bilinearRepeatSampler, input._uv);
    return writeGBuffer(albedo.rgb, input._normal.xyz, _metallic, _roughness);
}

#endif
