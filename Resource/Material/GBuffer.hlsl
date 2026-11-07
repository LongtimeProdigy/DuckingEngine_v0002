#ifndef GBUFFER_LIGHTING_HLSL
#define GBUFFER_LIGHTING_HLSL

#include "PostProcessCommon.hlsl"
#define ATMOSPHERE_FORCE_LUT
#include "Atmosphere/AtmosphereTransmittance.hlsl"
#include "GBufferCommon.hlsl"
#include "Atmosphere/AtmosphereSkyLighting.hlsl"
#include "Atmosphere/AtmosphereSurfaceLighting.hlsl"
#include "CommonMathConstant.hlsl"

cbuffer GBufferConstants : register(b2)
{
    uint _baseColorIndex;
    uint _normalRoughnessIndex;
    uint _materialIndex;
    uint _depthIndex;
};

// 태양 원반 이외의 하늘 환경광
float3 atmosphereSkyAmbient(float3 N, float3 V, float3 baseColor, float metallic, float roughness, float ao)
{
    if (_skyLightingStrength <= 0.0f) 
        return 0.0f;

    const float NoV = saturate(dot(N, V));
    const float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), baseColor, metallic);
    const float3 F = F0 + (max(1.0f - roughness, F0) - F0) * pow(1.0f - NoV, 5.0f);
    const float3 irradiance = sampleSkyBand(_skyFilteredIndex, N, 5u);                                                                      // 표면 법선 방향의 확산 irradiance를 조회
    const float level = saturate(roughness) * 4.0f;
    const uint band0 = uint(level), band1 = min(band0 + 1u, 4u);
    const float3 R = reflect(-V, N);
    const float3 reflection = lerp(sampleSkyBand(_skyFilteredIndex, R, band0), sampleSkyBand(_skyFilteredIndex, R, band1), frac(level));    // 정반사 환경광
    return ((1.0f - F) * (1.0f - metallic) * baseColor * irradiance / PI + F * reflection) * saturate(ao) * _skyLightingStrength;
}

float atmosphereSurfaceSunVisibilityFiltered(float3 p, float3 normal)
{
    if (_surfaceMapEnabled == 0u || _surfaceShadowStrength <= 0.0f) 
        return 1.0f;

    if (_shadowCascadeCount > 0u)
        return lerp(1.0f, atmosphereCascadedVisibility(p + normal * max(_surfaceBias, 0.0f), true), saturate(_surfaceShadowStrength));

    const float3 uvw = atmosphereSurfaceCoordinates(p + normal * max(_surfaceBias, 0.0f));
    const float delta = 0.5f / max(float(_surfaceMapSize), 1.0f);
    float visibility = 0.0f;
    [unroll]
    for (uint i = 0u; i < 4u; ++i)
    {
        const float2 offset = float2((i & 1u) != 0u ? delta : -delta, (i & 2u) != 0u ? delta : -delta);
        visibility += atmosphereSurfaceShadowTap(uvw + float3(offset, 0));
    }
    return lerp(1.0f, visibility * 0.25f, saturate(_surfaceShadowStrength));
}

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
    const float3 N = safeNormal(nr.xyz);                        // 표면 바깥쪽을 향하는 법선
    const float3 V = safeNormal(-cameraRelativePosition);       // 표면→카메라
    const float angle = radians(_sunDegree);
    const float3 L = float3(0, sin(angle), cos(angle));         // 표면→태양
    const float3 H = safeNormal(V + L);                         // V와 L 사이의 중간 방향
    const float NoL = saturate(dot(N, L));                      // 태양빛이 표면을 얼마나 정면으로 비추는지
    const float NoV = max(saturate(dot(N, V)), 0.001f);
    const float NoH = saturate(dot(N, H));
    const float VoH = saturate(dot(V, H));
    const float metallic = material.x;
    const float roughness = nr.w;
    const float a2 = pow(roughness, 4);
    const float d = NoH * NoH * (a2 - 1) + 1;
    const float D = a2 / max(PI * d * d, 1e-7f);                // 원하는 방향으로 기울어진 미세면이 얼마나 있는가
    const float k = (roughness + 1) * (roughness + 1) / 8;
    const float G = NoV / (NoV * (1 - k) + k) * NoL / (NoL * (1 - k) + k);  // 미세면들이 서로 가리는 정도
    const float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), base.rgb, metallic);    // 비금속에서는 정면 반사율을 약 4%로 설정, 금속에서는 baseColor를 반사색으로 사용
    const float3 F = F0 + (1 - F0) * pow(1 - VoH, 5);           // 미세면에서 반사되는 비율, 비스듬한 각도에서는 반사율이 커짐 (프레넬)
    const float3 specular = D * G * F / max(4 * NoV * NoL, 0.001f); // 입사광을 카메라 방향으로 얼마나 반사하는지 나타내는 BRDF 값 (빛의 양이 아님!)
    const float3 diffuse = (1 - F) * (1 - metallic) * base.rgb / PI;
    const float3 ambient = atmosphereSkyAmbient(N, V, base.rgb, metallic, roughness, material.y);
    float3 sunTransmittance = float3(1, 1, 1);
    if (NoL > 0.0f && _sunIrradiance > 0.0f)
        sunTransmittance = getSunTransmittance(cameraRelativePosition, L);
    return float4(ambient + (diffuse + specular) * NoL * atmosphereSunIrradiance() * sunTransmittance * atmosphereSurfaceSunVisibilityFiltered(cameraRelativePosition, N), 1);
}
#endif
