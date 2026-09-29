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
    uint depth;
    uint rngState;
    float4 color;
};

float RandomValue(inout uint state) 
{
    state *= (state + 195439) * (state + 124395) * (state + 845921);
    return state / 4294967295.0;
}

#define USE_JITTER

// ============================================================
// Ray Generation
// ============================================================
[shader("raygeneration")]
void RayGen()
{
    uint2 pixel = DispatchRaysIndex().xy;
    const uint2 size = DispatchRaysDimensions().xy;

#if defined(USE_JITTER)
    uint rngState = (pixel.y * size.x + pixel.x) + _rngState;
    const float2 jitter = float2(RandomValue(rngState), RandomValue(rngState));
    pixel = pixel + jitter;
#endif

    const float2 uv = (float2(pixel) + 0.5) / float2(size);
    float2 ndc = uv * 2.0 - 1.0;
    ndc.y = -ndc.y;
    const float nearPlaneHalfHeightWS = _nearDistance * getFOVTangent();
    const float nearPlaneHalfWidthWS = nearPlaneHalfHeightWS * ((float)_resolution.x / (float)_resolution.y);
    const float3 rayDirToNearPlaneWS = 
                                getViewForwardDirection() * _nearDistance 
                                + getViewRightDirection() * nearPlaneHalfWidthWS * ndc.x 
                                + getViewUpDirection() * nearPlaneHalfHeightWS * ndc.y;

    RayDesc ray;
    ray.Origin = getViewPosition();
    ray.Direction = normalize(rayDirToNearPlaneWS);
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    uint rayCount = 1;
    float4 tempColor = float4(0, 0, 0, 0);
    for (uint i = 0; i < rayCount; ++i)
    {
        RayPayload payload;
        payload.depth = 1;
        payload.rngState = rngState;
        payload.color = float4(0, 0, 0, 1);
        TraceRay(gTLAS, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

        tempColor += payload.color;
    }

    tempColor /= rayCount;

    RWTexture2D<float4> output = getTextureRW(_targetUAV);
    output[pixel] = tempColor;
}

// ============================================================
// Miss
// ============================================================
[shader("miss")]
void Miss(inout RayPayload payload)
{
    //payload.color = float4(1, 1, 1, 1.0);
    payload.color = float4(1, 1, 1, 1);
}

// ============================================================
// Closest Hit
// ============================================================
float3 CosineSampleHemisphere(inout uint state) 
{
    float u1 = RandomValue(state);
    float u2 = RandomValue(state);

    float r = sqrt(u1);
    float theta = 2.0 * 3.1415926 * u2;

    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(1.0 - u1);

    return float3(x, y, z);
}
float RandomValueNormalDistribution(inout uint state) {
    float theta = 2 * 3.1415926 * RandomValue(state);
	float rho = sqrt(-2 * log(max(RandomValue(state), 1e-9)));
    return rho * cos(theta);
}
float3 RandomDirection(inout uint state) {
    float x = RandomValueNormalDistribution(state);
    float y = RandomValueNormalDistribution(state);
    float z = RandomValueNormalDistribution(state);
    return normalize(float3(x, y, z));
}
float3 RandomHemisphereDirection(bool useCos, float3 normal, inout uint state) {
    float3 dir = useCos ? CosineSampleHemisphere(state) : RandomDirection(state);
    return dir * sign(dot(normal, dir));
}

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    if(payload.depth >= 2)
	{
		payload.color = float4(0, 0, 0, 1);
		return;
	}

    uint subMeshIndex = InstanceID();
    uint primitiveIndex = PrimitiveIndex();

    uint index0 = gIndices[NonUniformResourceIndex(subMeshIndex)][primitiveIndex * 3 + 0];
    uint index1 = gIndices[NonUniformResourceIndex(subMeshIndex)][primitiveIndex * 3 + 1];
    uint index2 = gIndices[NonUniformResourceIndex(subMeshIndex)][primitiveIndex * 3 + 2];

    float3 p0 = gVertices[NonUniformResourceIndex(subMeshIndex)][index0].position;
    float3 p1 = gVertices[NonUniformResourceIndex(subMeshIndex)][index1].position;
    float3 p2 = gVertices[NonUniformResourceIndex(subMeshIndex)][index2].position;

    float3 n0 = gVertices[NonUniformResourceIndex(subMeshIndex)][index0].normal;
    float3 n1 = gVertices[NonUniformResourceIndex(subMeshIndex)][index1].normal;
    float3 n2 = gVertices[NonUniformResourceIndex(subMeshIndex)][index2].normal;

    float2 uv0 = gVertices[NonUniformResourceIndex(subMeshIndex)][index0].uv0;
    float2 uv1 = gVertices[NonUniformResourceIndex(subMeshIndex)][index1].uv0;
    float2 uv2 = gVertices[NonUniformResourceIndex(subMeshIndex)][index2].uv0;

    float b1 = attributes.barycentrics.x;
    float b2 = attributes.barycentrics.y;
    float b0 = 1.0 - b1 - b2;

    float3 hitPosition = p0 * b0 + p1 * b1 + p2 * b2; 
    float3 hitNormal = n0 * b0 + n1 * b1 + n2 * b2; 
    float2 uv = uv0 * b0 + uv1 * b1 + uv2 * b2;

    float3x4 objectToWorld = ObjectToWorld3x4();
    float3 hitPositionWS = mul(objectToWorld, float4(hitPosition, 1.0));
    float3 hitNormalWS = hitNormal;//mul((float3x3)objectToWorld, hitNormal);

    Material material = gMaterials[NonUniformResourceIndex(subMeshIndex)][0];
    Texture2D<float4> diffuseTexture = getTexture(NonUniformResourceIndex(material._diffuseTexture));
    float4 diffuse = diffuseTexture.SampleLevel(bilinearRepeatSampler, uv, 0.0);

    const uint sampleCount = 1;

    const uint currentDepth = payload.depth;
	float3 sumEmissive = float3(0, 0, 0);
	for(uint i = 0; i < sampleCount; ++i)
	{
		const float3 Ldir = RandomHemisphereDirection(false, hitNormalWS, payload.rngState);
		// if(roughness < 0.2)
		// {
		// 	Ldir = normalize(-V + 2 * dot(V, hitNormalWS) * hitNormalWS);
		// }

		//hitValue._prevMonteCarlo = Ldir;

        RayDesc ray;
        ray.Origin = hitPositionWS;
        ray.Direction = Ldir;
        ray.TMin = 0.001;
        ray.TMax = 10000.0;

        RayPayload payload2;
        payload2.depth = currentDepth + 1;
        payload2.rngState = payload.rngState;
        payload2.color = float4(0, 0, 0, 1);
        TraceRay(gTLAS, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload2);

		const float3 brdf = diffuse.xyz * (1 / 3.14159265358979); //EvaluateBRDF(hitNormalWS, V, Ldir, hitColor, roughness, metallic);
		const float3 emissiveFromQ = payload2.color.xyz;
		const float cos_p = dot(hitNormalWS, Ldir);
        const float spherePDF = 1 / (2 * 3.14159265358979);
		sumEmissive += (brdf * emissiveFromQ * cos_p) / spherePDF;

        payload.rngState = payload2.rngState;
	}

    // if (diffuse.a <= 0.0)
    // {
    //     IgnoreHit();
    //     return;
    // }

    payload.color = float4((sumEmissive / sampleCount), 1);
    //payload.color = float4(0, 1, 1, 1);
}

#endif