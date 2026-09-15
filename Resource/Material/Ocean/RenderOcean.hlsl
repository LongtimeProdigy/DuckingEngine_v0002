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
    float3 worldPos : TEXCOORD1;
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

    output.worldPos = float3(localPosition.x, height, localPosition.y);

    output.position = float4(output.worldPos, 1.0);
    output.position = mul(output.position, _cameraWorldMatrixInv);
    output.position = mul(output.position, _cameraProjectionMatrix);

    output.uv = uv;

    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    static float _sunDegree = 70;
    static float _Glossiness = 500;
    static float _SpecularIntensity = 5.0;
    static float3 _LightColor0 = float3(1.0, 0.9, 0.8);

    // 1. 노멀맵 샘플링 (0~1 범위를 -1~1로 복원)
    Texture2D<float4> normalTexture = getTexture(_normalSRV);
    float3 normal = normalTexture.Sample(bilinearRepeatSampler, input.uv).xyz * 2.0 - 1.0;
    normal = normalize(normal);

    // 2. 조명 계산을 위한 벡터들
    float3 viewDir = normalize(getViewPosition() - input.worldPos);
    float rad = _sunDegree * 0.01745329;
    float3 lightDir = float3(0, sin(rad), cos(rad));
    float3 halfDir = normalize(lightDir + viewDir);

    // 3. Diffuse (심해의 어두운 푸른색 느낌)
    // 깊이에 따른 색상 (어두운 파랑 -> 밝은 파랑)
    float3 deepOcean = float3(0.01, 0.05, 0.1);
    float3 shallowOcean = float3(0.1, 0.4, 0.7);
    float diff = max(dot(normal, lightDir), 0.0);
    float3 oceanColor = lerp(deepOcean, shallowOcean, diff);

    // 4. Specular (태양광 반사 - 반짝이는 물결)
    // 노멀맵 덕분에 1m 격자 사이에서도 매끄러운 하이라이트가 생깁니다.
    float spec = pow(max(dot(normal, halfDir), 0.0), _Glossiness);
    float3 specularColor = _LightColor0.rgb * spec * _SpecularIntensity;

    // 5. Fresnel (바라보는 각도에 따른 반사광 조절)
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 5.0);
    float3 finalColor = oceanColor + specularColor + (fresnel * 0.2);

    return float4(finalColor, 1.0);
}

#endif