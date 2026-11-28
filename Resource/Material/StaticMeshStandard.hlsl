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
    float3 _worldOffset : TEXCOORD1;
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
    TextureParameter _normalTexture;
    TextureParameter _metallicRoughnessTexture;
    TextureParameter _occlusionTexture;
    float _normalScale;
    float _occlusionStrength;
}

VS_OUTPUT TransformStaticMeshVertex(StaticMeshVertex input, float4x4 worldMatrix)
{
    VS_OUTPUT output;
    output.position = float4(input._position, 1.0f);
    output._normal = float4(input._normal, 0.0f);
    output._uv = input._uv;
    output._worldOffset = mul(input._position, (float3x3)worldMatrix);

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

float3 sampleWorldNormal(VS_OUTPUT input)
{
    const float3 normal = safeNormal(input._normal.xyz);
    if (_normalTexture == DK_MISSING_MATERIAL_TEXTURE)
        return normal;

    const float3 dx = ddx(input._worldOffset);
    const float3 dy = ddy(input._worldOffset);
    const float2 uvDx = ddx(input._uv);
    const float2 uvDy = ddy(input._uv);
    const float determinant = uvDx.x * uvDy.y - uvDx.y * uvDy.x;
    const float3 dU = (dx * uvDy.y - dy * uvDx.y) * sign(determinant);
    const float3 dV = (dy * uvDx.x - dx * uvDy.x) * sign(determinant);
    const float3 tangent = dU - normal * dot(normal, dU);
    const float lengthSquared = dot(tangent, tangent);
    if (lengthSquared <= 1e-30f)
        return normal;

    const float3 T = tangent * rsqrt(lengthSquared);
    const float3 B = cross(normal, T) * sign(dot(cross(normal, T), dV));
    float3 mapped = getTexture(_normalTexture).SampleGrad(bilinearRepeatSampler, input._uv, uvDx, uvDy).xyz * 2.0f - 1.0f;
    mapped.xy *= _normalScale;
    return safeNormal(T * mapped.x + B * mapped.y + normal * mapped.z);
}

GBufferOutput PSMain(VS_OUTPUT input)
{
    Texture2D<float4> diffuseTexture = getTexture(_diffuseTexture);
    const float4 albedo = diffuseTexture.Sample(bilinearRepeatSampler, input._uv);
    const float3 normal = sampleWorldNormal(input);

    const float alpha = albedo.w * _opacity;
    if (alpha <= 0.0f || alpha < _alphaCutoff)
        clip(-1);

    float metallic = _metallic;
    float roughness = _roughness;
    if (_metallicRoughnessTexture != DK_MISSING_MATERIAL_TEXTURE)
    {
        const float4 packed = getTexture(_metallicRoughnessTexture).Sample(bilinearRepeatSampler, input._uv);
        roughness *= packed.g;
        metallic *= packed.b;
    }
    float occlusion = 1.0f;
    if (_occlusionTexture != DK_MISSING_MATERIAL_TEXTURE)
        occlusion = lerp(1.0f, getTexture(_occlusionTexture).Sample(bilinearRepeatSampler, input._uv).r, _occlusionStrength);

    return writeGBuffer(albedo.rgb, normal, metallic, roughness, 0, occlusion);
}

#endif
