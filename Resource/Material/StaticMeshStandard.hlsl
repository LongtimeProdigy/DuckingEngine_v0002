#ifndef __DEFINE_STATICMESHSTANDARD_HLSL__
#define __DEFINE_STATICMESHSTANDARD_HLSL__

#include "CommonTexture.hlsl"
#include "CommonRendering.hlsl"
#include "GBufferCommon.hlsl"

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
    float _metallic;
    float _roughness;
}

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = float4(input.position, 1.0f);
    output.normal = float4(input.normal, 0.0f);
    output.uv0 = input.uv0;

    output.position = mul(output.position, _worldMatrix);
    output.position = mul(output.position, _cameraWorldMatrixInv);
    output.position = mul(output.position, _cameraProjectionMatrix);

    output.normal = float4(transformNormal(input.normal, (float3x3)_worldMatrix), 0);

    return output;
}

GBufferOutput PSMain(VS_OUTPUT input)
{
    Texture2D<float4> diffuseTexture = getTexture(_diffuseTexture);
    const float4 albedo = diffuseTexture.Sample(bilinearRepeatSampler, input.uv0);

    if(albedo.w * _opacity <= 0.0f)
        clip(-1);

    return writeGBuffer(albedo.rgb, input.normal.xyz, _metallic, _roughness);
}

#endif