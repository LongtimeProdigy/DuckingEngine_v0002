#ifndef __DEFINE_TERRAINCLIPMAP__HLSL__
#define __DEFINE_TERRAINCLIPMAP__HLSL__

#include "CommonTexture.hlsl"
#include "CommonRendering.hlsl"
#include "ClipmapCommon.hlsl"
#include "GBufferCommon.hlsl"

struct VS_INPUT
{
    float2 position : POSITION;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv0 : TEXCOORD0;
    float4 color : COLOR;
    float3 worldPosition : TEXCOORD1;
};

// 터레인에 하나만 있는 material 조합
cbuffer TerrainClipMap : register(b1)
{
    TextureParameter _diffuseTexture;
    TextureParameter _heightTexture;
    float _metallic;
    float _roughness;
}

float2 GetTerrainUV(float2 worldXZ, uint2 textureSize)
{
    const float meterPerTexel = 0.125f;
    return worldXZ / (float2(textureSize) * meterPerTexel) + 0.5f;
}

float3 EvaluateSurfacePosition(float2 worldXZ, uint level)
{
    Texture2D<float4> heightTexture = getTexture(_heightTexture);

    uint2 textureSize;
    heightTexture.GetDimensions(textureSize.x, textureSize.y);

    // texel 중심을 기준으로 샘플링한다.
    const float2 heightUV = GetTerrainUV(worldXZ, textureSize) + 0.5f / float2(textureSize);

    // 현재 데이터 경로에 맞춰 mip 0을 사용한다.
    // 레벨별 필터링 데이터를 추가하면 여기서 level에 따라 선택한다.
    const float height = heightTexture.SampleLevel(bilinearRepeatSampler, heightUV, 0.0f).x * 50.0f - 50.0f;
    return float3(worldXZ.x, height, worldXZ.y);
}

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    const float2 baseXZ = GetClipmapBaseXZ(input.position);
    const float3 worldPosition = EvaluateClipmapPosition(baseXZ);

    output.worldPosition = worldPosition;
    output.position = mul(float4(worldPosition, 1.0f), _cameraWorldMatrixInv);

    output.position = mul(output.position, _cameraProjectionMatrix);

    Texture2D<float4> heightTexture = getTexture(_heightTexture);

    uint2 textureSize;
    heightTexture.GetDimensions(textureSize.x, textureSize.y);

    // UV를 정점 단계에서 감싸지 않는다.
    // 픽셀에서 Repeat sampler가 처리한다.
    output.uv0 = GetTerrainUV(baseXZ, textureSize);

    static const float4 colors[5] =
    {
        float4(0, 0, 0, 1), // Cross
        float4(1, 0, 0, 1), // Tile
        float4(0, 1, 0, 1), // Filler
        float4(0, 0, 1, 1), // Seam
        float4(1, 1, 0, 1)  // Trim
    };

    output.color = colors[_type];

    return output;
}

GBufferOutput PSMain(VS_OUTPUT input)
{
    float3 normal = safeNormal(cross(ddy(input.worldPosition), ddx(input.worldPosition)));
    if (normal.y < 0) normal = -normal;
    const float3 albedo = getTexture(_diffuseTexture).Sample(bilinearRepeatSampler, input.uv0).rgb;
    return writeGBuffer(albedo, normal, _metallic, _roughness);
}
#endif