#ifndef GBUFFER_COMMON_HLSL
#define GBUFFER_COMMON_HLSL
// Linear base color + coverage; world normal + roughness; metallic/AO/unlit/reserved.
struct GBufferOutput
{
    float4 baseColor : SV_Target0;
    float4 normalRoughness : SV_Target1;
    float4 material : SV_Target2;
};
float3 safeNormal(float3 n)
{
    return dot(n, n) > 1e-12f ? normalize(n) : float3(0, 1, 0);
}
// Inverse transpose for row-vector transforms, including non-uniform scale.
float3 transformNormal(float3 normal, float3x3 transform)
{
    float3x3 cofactor = float3x3(cross(transform[1], transform[2]),
        cross(transform[2], transform[0]), cross(transform[0], transform[1]));
    float determinant = dot(transform[0], cofactor[0]);
    return safeNormal(mul(normal, cofactor) * (determinant < 0 ? -1.0f : 1.0f));
}
GBufferOutput writeGBuffer(float3 baseColor, float3 normal, float metallic, float roughness, float unlit = 0)
{
    GBufferOutput output;
    output.baseColor = float4(max(baseColor, 0), 1);
    output.normalRoughness = float4(safeNormal(normal), clamp(roughness, 0.04f, 1));
    output.material = float4(saturate(metallic), 1, unlit, 0);
    return output;
}
#endif
