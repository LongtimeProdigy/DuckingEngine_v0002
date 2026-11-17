#ifndef __DEFINE_STATICMESHSTANDARD_HLSL__
#define __DEFINE_STATICMESHSTANDARD_HLSL__

#include "CommonTexture.hlsl"
#include "CommonRendering.hlsl"
#include "GBufferCommon.hlsl"

#define DK_SHADER_STATIC_VERTEX
#include "ShaderStructures.h"
#undef DK_SHADER_STATIC_VERTEX

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 _normal : NORMAL;
    float2 _uv : TEXCOORD0;
};

#define DK_SHADER_OBJECT
#include "ShaderStructures.h"
#undef DK_SHADER_OBJECT

#define DK_SHADER_INSTANCE
#include "ShaderStructures.h"
#undef DK_SHADER_INSTANCE

StructuredBuffer<StaticMeshInstance> _staticMeshInstances : register(t0);

cbuffer StaticMeshInstanceConstants : register(b3)
{
    uint _firstInstance;
}

cbuffer StaticMeshStandard : register(b2)
{
    TextureParameter _diffuseTexture;
    float _opacity;
    float _metallic;
    float _roughness;
    float _alphaCutoff;
}

VS_OUTPUT TransformStaticMeshVertex(StaticMeshVertex input, float4x4 worldMatrix)
{
    VS_OUTPUT output;
    output.position = float4(input._position, 1.0f);
    output._normal = float4(input._normal, 0.0f);
    output._uv = input._uv;

    output.position = mul(output.position, worldMatrix);
    output.position = mul(output.position, _cameraWorldMatrixInv);
    output.position = mul(output.position, _cameraProjectionMatrix);

    output._normal = float4(transformNormal(input._normal, (float3x3)worldMatrix), 0);

    return output;
}

VS_OUTPUT VSMain(StaticMeshVertex input)
{
    return TransformStaticMeshVertex(input, _worldMatrix);
}

VS_OUTPUT VSMainInstanced(StaticMeshVertex input, uint instanceID : SV_InstanceID)
{
    return TransformStaticMeshVertex(input, _staticMeshInstances[_firstInstance + instanceID]._worldMatrix);
}

GBufferOutput PSMain(VS_OUTPUT input)
{
    Texture2D<float4> diffuseTexture = getTexture(_diffuseTexture);
    const float4 albedo = diffuseTexture.Sample(bilinearRepeatSampler, input._uv);

    const float alpha = albedo.w * _opacity;
    if (alpha <= 0.0f || alpha < _alphaCutoff)
        clip(-1);

    return writeGBuffer(albedo.rgb, input._normal.xyz, _metallic, _roughness);
}

#endif
