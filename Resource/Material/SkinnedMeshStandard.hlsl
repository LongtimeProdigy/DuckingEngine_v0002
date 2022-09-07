#define MAX_SKINNING_COUNT 4
#define MAX_BONE_COUNT 55
#define INVALID_BONE_INDEX 0xffffffff
#define INVALID_TEXTURE_INDEX 0xffffffff

struct VS_INPUT
{
    float4 pos : POSITION;
    float4 normal : NORMAL;
    float2 texCoord: TEXCOORD;

    uint4 boneIndices : BONEINDICES;
    float4 weights : WEIGHTS;
};

struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float4 normal : NORMAL;
    float2 texCoord: TEXCOORD;
    float4 color : COLOR;
};

Texture2D texture2DTable[] : register(t0, space0)

// per Scene
cbuffer CameraConstantBufferStruct : register(b0)
{
    float4x4 cameraWorldMatrix;
    float4x4 cameraProjectionMatrix;
};

// per SceneObject
cbuffer SceneObjectConstantBufferStruct : register(b1)
{
    float4x4 worldMatrix;
};

// per SkinnedMeshComponent
cbuffer SkeletonConstantBufferStruct : register(b2)
{
    float4x4 skeletonMatrixBuffer[MAX_BONE_COUNT];
};

// per SubMesh
cbuffer ModelPropertyBufferStruct : register(b3)
{
    uint diffuseTexture;
    float opacity;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;

    float4x4  identityMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    int boneIndex1 = input.boneIndices.x, boneIndex2 = input.boneIndices.y, boneIndex3 = input.boneIndices.z, boneIndex4 = input.boneIndices.w;
    float weight1 = input.weights.x, weight2 = input.weights.y, weight3 = input.weights.z, weight4 = input.weights.w;
    float4x4 skinningMatrix =
        (boneIndex1 != INVALID_BONE_INDEX ? skeletonMatrixBuffer[boneIndex1] : identityMatrix) * weight1 +
        (boneIndex2 != INVALID_BONE_INDEX ? skeletonMatrixBuffer[boneIndex2] : identityMatrix) * weight2 + 
        (boneIndex3 != INVALID_BONE_INDEX ? skeletonMatrixBuffer[boneIndex3] : identityMatrix) * weight3 + 
        (boneIndex4 != INVALID_BONE_INDEX ? skeletonMatrixBuffer[boneIndex4] : identityMatrix) * weight4;
    output.pos = mul(input.pos, skinningMatrix);

    output.pos = mul(input.pos, worldMatrix);
    output.pos = mul(output.pos, cameraWorldMatrix);
    output.pos = mul(output.pos, cameraProjectionMatrix);

    output.normal = mul(input.normal, worldMatrix);
    output.texCoord = input.texCoord;

    //output.color = float4(
    //    boneIndex1 != errorIndex ? 1.0f : 0.0f, 
    //    boneIndex2 != errorIndex ? 1.0f : 0.0f, 
    //    boneIndex3 != errorIndex ? 1.0f : 0.0f, 
    //    1.0f
    //    );

    //output.color = float4(1, 0, 1, 1);

    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    if(diffuseTexture == INVALID_TEXTURE_INDEX)
    {
        return float4(0, 1, 1, 1);
    }

    return texture2DTable[diffuseTexture].Sample(s0, input.texCoord);
}