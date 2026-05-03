#ifndef __DEFINE_HEIGHTOUTPUT_HLSL__
#define __DEFINE_HEIGHTOUTPUT_HLSL__

#include "CommonTexture.hlsl"
#include "Ocean/OceanCommon.hlsl"
#include "CommonRendering.hlsl"

struct VS_INPUT
{
    float2 position : POSITION;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV0;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    const float2 localPosition = input.position;

    Texture2D<float4> heightTexture = getTexture(_heightSRV);
    uint2 texSize;
    heightTexture.GetDimensions(texSize.x, texSize.y);

    const float meterPerTexel = 1.0 / float(_length);
    const float2 uv =  (localPosition * meterPerTexel);
    //uv = abs(uv % 1.0f);
    const float height = heightTexture.Load(uint3(uv * texSize, 0)).x;

    output.position = float4(localPosition.x, height, localPosition.y, 1.0f);

    output.position = mul(output.position, _cameraWorldMatrixInv);
    output.position = mul(output.position, _cameraProjectionMatrix);

    output.color = float4(uv.x, uv.y, 0, 1);

    output.uv = uv;

    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    // 단순 시각화를 위해 높이값에 따른 색상 정의
    Texture2D<float4> heightTexture = getTexture(_heightSRV);
    float height = heightTexture.Sample(normalSampler, input.uv).x;

    // 깊이에 따른 색상 (어두운 파랑 -> 밝은 파랑)
    float3 deepOcean = float3(0.01, 0.05, 0.1);
    float3 shallowOcean = float3(0.1, 0.4, 0.7);

    // 높이 기반 보간 (0을 기준으로 색상 결정)
    float colorFactor = saturate(height * 0.1 + 0.5); 
    float3 baseColor = lerp(deepOcean, shallowOcean, colorFactor);

    // 간단한 퐁(Phong) 조명 느낌 추가 (위에서 아래로 내리는 빛)
    float3 lightDir = normalize(float3(0.5, 1.0, 0.2));
    // 실제 노멀맵이 없다면 대략적으로 위쪽 방향(0,1,0)과 연산
    float diff = max(dot(float3(0, 1, 0), lightDir), 0.5);

    return float4(baseColor * diff, 1.0f);
}

#endif