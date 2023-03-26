#ifndef __DEFINE_TERRAINCLIPMAP__HLSL__
#define __DEFINE_TERRAINCLIPMAP__HLSL__

#include "CommonTexture.hlsl"
#include "CommonRendering.hlsl"

struct VS_INPUT
{
    float2 position : POSITION;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv0 : TEXCOORD0;
    float4 color : COLOR;
};

// 터레인에 하나만 있는 material 조합
cbuffer TerrainStandard : register(b1)
{
    TextureParameter _diffuseTexture;
    TextureParameter _heightTexture;
}

// Terrain Mesh별로 하나씩 있음
cbuffer TerrainMeshConstantBuffer : register(b2)
{
    float4 _baseXY_scale_rotate;
    uint _type;
}

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    float scale = _baseXY_scale_rotate.z;
    float2 offset = _baseXY_scale_rotate.xy;
    float type = _baseXY_scale_rotate.w;

    static float2x2 temp[] = 
    {
        float2x2(1, 0, 0, 1), // 0
        float2x2(0, -1, 1, 0), // 270
        float2x2(0, 1, -1, 0), // 90
        float2x2(-1, 0, 0, -1), // 180
    };
    float2 localPosition = input.position * scale;
    localPosition = mul(localPosition, temp[type]);
    localPosition = offset + localPosition;

    Texture2D heightTexture = gBindlessTextureArray[_heightTexture];
    float meterPerTexel = 0.125f;
    uint2 texSize;
    heightTexture.GetDimensions(texSize.x, texSize.y);
    float2 uv =  (localPosition + texSize * meterPerTexel * 0.5) / (texSize * meterPerTexel);
    uv = abs(uv % 1.0f);
    float4 height = heightTexture.Load(uint3(uv * texSize, 0)) * 50 - 50;

    output.position = float4(localPosition.x, height.x, localPosition.y, 1.0f);

    output.position = mul(output.position, _cameraWorldMatrix);
    output.position = mul(output.position, _cameraProjectionMatrix);

    output.uv0 = uv;

    static float4 ColorArr[] = 
    {
        float4(0, 0, 0, 1), // cross
        float4(1, 0, 0, 1), // tile
        float4(0, 1, 0, 1), // filter
        float4(0, 0, 1, 1), // seam
        float4(1, 1, 0, 1), // trim
    };
    output.color = ColorArr[_type];

    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
#if 1
    Texture2D diffuseTexture = gBindlessTextureArray[_diffuseTexture];
    return diffuseTexture.Sample(normalSampler, input.uv0);
#else
    return input.color;
#endif
}

#endif