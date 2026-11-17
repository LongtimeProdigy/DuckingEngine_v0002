#ifndef __DEFINE_SKINNEDMESHSTANDARD_HLSL__
#define __DEFINE_SKINNEDMESHSTANDARD_HLSL__

#include "SkinningLimits.h"
#include "CommonTexture.hlsl"
#include "CommonRendering.hlsl"
#include "GBufferCommon.hlsl"

#define DK_SHADER_SKINNED_VERTEX
#include "ShaderStructures.h"
#undef DK_SHADER_SKINNED_VERTEX

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 _normal : NORMAL;
    float2 _uv : TEXCOORD0;
};

#define DK_SHADER_OBJECT
#include "ShaderStructures.h"
#undef DK_SHADER_OBJECT

cbuffer SkinnedMeshStandard : register(b2)
{
    TextureParameter _diffuseTexture;
    float _opacity;
    float _metallic;
    float _roughness;
}

// #todo- StructuredBuffer로 교체 가능?
#define MAX_BONE_COUNT DK_MAX_BONE_COUNT
#define DK_SHADER_SKELETON
#include "ShaderStructures.h"
#undef DK_SHADER_SKELETON

VS_OUTPUT VSMain(SkinnedMeshVertex input)
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

        skinMatrix += _currentCharacterSpaceBoneAnimation[input._boneIndices[i]] * input._boneWeights[i];
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
