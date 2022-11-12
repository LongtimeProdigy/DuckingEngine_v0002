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

#define BINDLESSTEXTUREARRAY_SPACE space10
Texture2D gBindlessTextureArray[] : register(t0, BINDLESSTEXTUREARRAY_SPACE);
SamplerState normalSampler : register(s0);

cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 _cameraWorldMatrix;
	float4x4 _cameraProjectionMatrix;
}
cbuffer SceneObjectConstantBuffer : register(b1)
{
    float4x4 _worldMatrix;
}
#define MAX_SKINNING_COUNT 4
#define MAX_BONE_COUNT 100
cbuffer SkeletonConstantBuffer : register(b2)
{
    float4x4 _skeletonMatrixBuffer[MAX_BONE_COUNT];
}
#define TextureParameter uint
cbuffer SkinnedMeshStandard : register(b3)
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
#if 0
    float4x4 temp = 
    {
        1, 0, 0, 0, 
        0, 1, 0, -1, 
        0, 0, 1, 1, 
        0, 0, 0, 1
    };
    float4x4 temp2 =
    {
        0.562500179, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1.00100100, -1.00100100,
        0, 0, 1, 0
    };
    output.position = mul(temp, output.position);
    output.position = mul(temp2, output.position);
#else
    // Skinning
    float4x4 skinMatrix = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    for(uint i = 0; i < MAX_SKINNING_COUNT; ++i)
    {
        skinMatrix += ((input.boneIndexes[i] == 0xffffffff || input.boneWeights[i] == 0.0) ? 0.0 : _skeletonMatrixBuffer[input.boneIndexes[i]]) * input.boneWeights[i];
    }

    //output.position = mul(skinMatrix, output.position);
    output.position = mul(_worldMatrix, output.position);
    output.position = mul(_cameraWorldMatrix, output.position);
    output.position = mul(_cameraProjectionMatrix, output.position);

    output.normal = mul(_cameraWorldMatrix, output.normal);
#endif
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    Texture2D diffuseTexture = gBindlessTextureArray[_diffuseTexture];
    return diffuseTexture.Sample(normalSampler, input.uv0);
}