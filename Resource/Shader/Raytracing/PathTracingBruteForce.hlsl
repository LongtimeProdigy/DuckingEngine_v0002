#ifndef __DEFINE_PATHTRACING_BRUTEFORCE__
#define __DEFINE_PATHTRACING_BRUTEFORCE__

#include "CommonTexture.hlsl"

cbuffer RaytracingConstants : register(b0)
{
    TextureParameter _targetUAV;
};
//RWTexture2D<float4> gOutput : register(u0);
RaytracingAccelerationStructure gTLAS : register(t0);

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
    // uint2 pixel = DispatchRaysIndex().xy;
    // uint2 size = DispatchRaysDimensions().xy;
    // float2 uv = (float2(pixel) + 0.5) / float2(size);

    // RayDesc ray;
    // ray.Origin = float3(0.0, 0.0, -5.0);
    // ray.Direction = normalize(float3(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 1.0));
    // ray.TMin = 0.001;
    // ray.TMax = 10000.0;

    // RayPayload payload;
    // payload.color = float4(0, 0, 0, 1);

    // TraceRay(gTLAS, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

    // RWTexture2D<float4> output = getTextureRW(_targetUAV);
    // output[pixel] = payload.color;

    uint2 id = DispatchRaysIndex().xy;
    RWTexture2D<float4> output = getTextureRW(_targetUAV);
    output[id] = float4(1.0, 1.0, 1.0, 1.0);
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
    payload.color = float4(1.0, 0.0, 0.0, 1.0);
}

#endif