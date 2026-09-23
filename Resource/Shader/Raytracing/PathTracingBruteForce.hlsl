#ifndef __DEFINE_PATHTRACING_BRUTEFORCE__
#define __DEFINE_PATHTRACING_BRUTEFORCE__

#include "CommonTexture.hlsl"
#include "CommonRendering.hlsl"

#define BINDLESSVERTEXARRAY_SPACE space11
#define BINDLESSINDEXARRAY_SPACE space12
#define BINDLESSMATERIALARRAY_SPACE space13

cbuffer RaytracingConstants : register(b1)
{
    TextureParameter _targetUAV;
};

RaytracingAccelerationStructure gTLAS : register(t0);

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv0 : TEXCOORD0;
};
StructuredBuffer<VS_INPUT> gVertices[] : register(t0, BINDLESSVERTEXARRAY_SPACE);
StructuredBuffer<uint> gIndices[]  : register(t0, BINDLESSINDEXARRAY_SPACE);
struct Material
{
    TextureParameter _diffuseTexture;
    float _opacity;
};
StructuredBuffer<Material> gMaterials[] : register(t0, BINDLESSMATERIALARRAY_SPACE);

struct RayPayload
{
    float4 color;
};

// ============================================================
// Ray Generation
// ============================================================
[shader("raygeneration")]
void RayGen()
{
    const uint2 pixel = DispatchRaysIndex().xy;
    const uint2 size = DispatchRaysDimensions().xy;
    const float2 uv = (float2(pixel) + 0.5) / float2(size);

    float2 screen = uv * 2.0 - 1.0;
    screen.y = -screen.y;

    const float nearPlaneHalfHeightWS = _nearDistance * getFOVTangent();
    const float nearPlaneHalfWidthWS = nearPlaneHalfHeightWS * ((float)_resolution.x / (float)_resolution.y);
    const float3 rayDirToNearPlaneWS = getViewForwardDirection() * _nearDistance + getViewRightDirection() 
        * nearPlaneHalfWidthWS * screen.x + getViewUpDirection() * nearPlaneHalfHeightWS * screen.y;

    RayDesc ray;
    ray.Origin = getViewPosition();
    ray.Direction = normalize(rayDirToNearPlaneWS);
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    RayPayload payload;
    payload.color = float4(0, 0, 0, 1);

    TraceRay(gTLAS, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

    RWTexture2D<float4> output = getTextureRW(_targetUAV);
    output[pixel] = payload.color;
}

// ============================================================
// Miss
// ============================================================
[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.color = float4(0.1, 0.2, 0.4, 1.0);
}

// ============================================================
// Closest Hit
// ============================================================
[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    uint subMeshIndex = InstanceID();

    uint primitiveIndex = PrimitiveIndex();

    uint index0 = gIndices[subMeshIndex][primitiveIndex * 3 + 0];
    uint index1 = gIndices[subMeshIndex][primitiveIndex * 3 + 1];
    uint index2 = gIndices[subMeshIndex][primitiveIndex * 3 + 2];

    float2 uv0 = gVertices[subMeshIndex][index0].uv0;
    float2 uv1 = gVertices[subMeshIndex][index1].uv0;
    float2 uv2 = gVertices[subMeshIndex][index2].uv0;

    float b1 = attributes.barycentrics.x;
    float b2 = attributes.barycentrics.y;
    float b0 = 1.0 - b1 - b2;

    float2 uv = uv0 * b0 + uv1 * b1 + uv2 * b2;

    Material material = gMaterials[subMeshIndex][0];

    TextureParameter diffuseTextureSRV = material._diffuseTexture;
    Texture2D<float4> diffuseTexture = getTexture(diffuseTextureSRV);

    float4 diffuse = diffuseTexture.SampleLevel(bilinearRepeatSampler, uv, 0.0);

    // if (diffuse.a <= 0.0)
    // {
    //     IgnoreHit();
    //     return;
    // }

    payload.color = diffuse;
    //payload.color = float4(0, 1, 1, 1);
}

#endif