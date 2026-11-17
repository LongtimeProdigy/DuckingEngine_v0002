#ifndef __DEFINE_STATICMESHSTANDARD_HLSL__
#define __DEFINE_STATICMESHSTANDARD_HLSL__

#include "CommonTexture.hlsl"
#include "CommonRendering.hlsl"
#include "GBufferCommon.hlsl"

struct VS_INPUT
{
    float3 _position : POSITION;
    float3 _normal : NORMAL;
    float2 _uv : TEXCOORD0;
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

struct StaticMeshInstance
{
    float4x4 _worldMatrix;
};
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

VS_OUTPUT TransformStaticMeshVertex(VS_INPUT input, float4x4 worldMatrix)
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

VS_OUTPUT VSMain(VS_INPUT input)
{
    return TransformStaticMeshVertex(input, _worldMatrix);
}

VS_OUTPUT VSMainInstanced(VS_INPUT input, uint instanceID : SV_InstanceID)
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
