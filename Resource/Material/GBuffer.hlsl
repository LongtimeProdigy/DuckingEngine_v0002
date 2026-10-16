#ifndef GBUFFER_LIGHTING_HLSL
#define GBUFFER_LIGHTING_HLSL

#include "PostProcessCommon.hlsl"
#include "AtmosphereTransmittance.hlsl"
#include "GBufferCommon.hlsl"
#include "CommonMathConstant.hlsl"

cbuffer GBufferConstants : register(b2)
{
    uint _baseColorIndex;
    uint _normalRoughnessIndex;
    uint _materialIndex;
    uint _depthIndex;
};

float4 PSMain(VS_OUTPUT input) : SV_Target
{
    const int3 pixel = int3(input.position.xy, 0);
    const float4 base = getTexture(_baseColorIndex).Load(pixel);
    if (base.a == 0) 
        return float4(0, 0, 0, 1);

    const float4 material = getTexture(_materialIndex).Load(pixel);
    if (material.z > 0.5f) 
        return float4(base.rgb, 1);
        
    const float4 nr = getTexture(_normalRoughnessIndex).Load(pixel);
    const float depth = getTexture(_depthIndex).Load(pixel).x;
    const float viewZ = _cameraProjectionMatrix._43 / (depth - _cameraProjectionMatrix._33);
    const float2 ndc = input.position.xy / float2(_resolution) * float2(2, -2) + float2(-1, 1);
    const float3 viewPosition = float3(ndc.x / _cameraProjectionMatrix._11, ndc.y / _cameraProjectionMatrix._22, 1) * viewZ;
    const float3 cameraRelativePosition = mul(float4(viewPosition, 0), _cameraWorldMatrix).xyz;
    const float3 N = safeNormal(nr.xyz);
    const float3 V = safeNormal(-cameraRelativePosition);
    const float angle = radians(_sunDegree);
    const float3 L = float3(0, sin(angle), cos(angle));
    const float3 H = safeNormal(V + L);
    const float NoL = saturate(dot(N, L));
    const float NoV = max(saturate(dot(N, V)), 0.001f);
    const float NoH = saturate(dot(N, H));
    const float VoH = saturate(dot(V, H));
    const float metallic = material.x;
    const float roughness = nr.w;
    const float a2 = pow(roughness, 4);
    const float d = NoH * NoH * (a2 - 1) + 1;
    const float D = a2 / max(PI * d * d, 1e-7f);
    const float k = (roughness + 1) * (roughness + 1) / 8;
    const float G = NoV / (NoV * (1 - k) + k) * NoL / (NoL * (1 - k) + k);
    const float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), base.rgb, metallic);
    const float3 F = F0 + (1 - F0) * pow(1 - VoH, 5);
    const float3 specular = D * G * F / max(4 * NoV * NoL, 0.001f);
    const float3 diffuse = (1 - F) * (1 - metallic) * base.rgb / PI;
    const float3 ambient = base.rgb * (1 - metallic) * 0.03f * material.y;
    float3 sunTransmittance = float3(1, 1, 1);
    if (NoL > 0.0f && _sunIntensity > 0.0f)
        sunTransmittance = getSunTransmittance(cameraRelativePosition, L);
    return float4(ambient + (diffuse + specular) * NoL * max(_sunIntensity, 0) * sunTransmittance, 1);
}
#endif
