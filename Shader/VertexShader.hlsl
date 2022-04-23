#define MAX_SKINNING_COUNT 4

struct VS_INPUT
{
    float4 pos : POSITION;
    float4 normal : NORMAL;
    float2 texCoord: TEXCOORD;

    int4 boneIndices : BONEINDICES;
    float4 weights : WEIGHTS;
};

struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float4 normal : NORMAL;
    float2 texCoord: TEXCOORD;
    float4 color : COLOR;
};

cbuffer CameraConstantBufferStruct : register(b0)
{
    float4x4 cameraWorldMatrix;
    float4x4 cameraProjectionMatrix;
};

cbuffer SceneObjectConstantBufferStruct : register(b1)
{
    float4x4 worldMatrix;
};

cbuffer SkeletonConstantBufferStruct : register(b2)
{
    float4x4 skeletonMatrixBuffer[300];
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    int bonIndex1 = input.boneIndices.x, bonIndex2 = input.boneIndices.y, bonIndex3 = input.boneIndices.z, bonIndex4 = input.boneIndices.w;
    float weight1 = input.weights.x, weight2 = input.weights.y, weight3 = input.weights.z, weight4 = input.weights.z;
    output.pos += mul(input.pos, skeletonMatrixBuffer[bonIndex1]) * weight1;
    output.pos += mul(input.pos, skeletonMatrixBuffer[bonIndex2]) * weight2;
    output.pos += mul(input.pos, skeletonMatrixBuffer[bonIndex3]) * weight3;
    output.pos += mul(input.pos, skeletonMatrixBuffer[bonIndex4]) * weight4;
    output.pos = mul(input.pos, worldMatrix);

    output.pos = mul(output.pos, cameraWorldMatrix);
    output.pos = mul(output.pos, cameraProjectionMatrix);

    output.normal = mul(input.normal, worldMatrix);
    output.texCoord = input.texCoord;

    output.color = output.pos;

    return output;
}