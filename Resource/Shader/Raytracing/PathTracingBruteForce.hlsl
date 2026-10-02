#ifndef __DEFINE_PATHTRACING_BRUTEFORCE__
#define __DEFINE_PATHTRACING_BRUTEFORCE__

#include "CommonTexture.hlsl"
#include "CommonRendering.hlsl"

#define BINDLESSVERTEXARRAY_SPACE space11
#define BINDLESSINDEXARRAY_SPACE space12
#define BINDLESSMATERIALARRAY_SPACE space13

cbuffer RaytracingConstants : register(b1)
{
    TextureParameter _accumulateTextureUAV;
    TextureParameter _targetUAV;
    uint _sampleCount;
    uint _samplingMode;
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

// ============================================================
// Ray Generation
// ============================================================
[shader("raygeneration")]
void RayGen()
{
    uint2 pixel = DispatchRaysIndex().xy;
    const uint2 size = DispatchRaysDimensions().xy;

#if 1
    uint rngState = (pixel.y * size.x + pixel.x) + _rngState + _sampleCount * 747796405u;
    const float2 jitter = float2(RandomValue(rngState), RandomValue(rngState));
    pixel = pixel + jitter;
    //일반적인 [0, 1) jitter는 정수 변환에서 사라집니다. 출력 좌표는 그대로 두고, 광선 생성용 좌표만 실수로 계산해야 합니다.
    const float2 uv = (float2(pixel) + jitter) / float2(size);
#else
    const float2 uv = (float2(pixel) + 0.5) / float2(size);
#endif

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

    const uint rayCount = 1;
    float4 tempColor = float4(0, 0, 0, 0);
    RayPayload payload;
    payload.rngState = rngState;
    for (uint i = 0; i < rayCount; ++i)
    {
        payload.depth = 1;
        payload.color = float4(0, 0, 0, 1);
        TraceRay(gTLAS, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 1, 0, ray, payload);

        tempColor += payload.color;
    }

    tempColor /= rayCount;

    RWTexture2D<float4> accumulation = getTextureRW(_accumulateTextureUAV);
    RWTexture2D<float4> output = getTextureRW(_targetUAV);

    float3 average;
    [branch]
    if (_sampleCount == 0)
    {
        // 최초 실행 또는 리셋: 초기화되지 않은 이전 텍스처 값을 읽지 않습니다.
        average = tempColor.rgb;
    }
    else
    {
        const float3 previousAverage = accumulation[pixel].rgb;
        const float weight = 1.0 / (float(_sampleCount) + 1.0);
        average = previousAverage + (tempColor.rgb - previousAverage) * weight;
    }

    accumulation[pixel] = float4(average, 1.0);
    output[pixel] = float4(pow(max(average, 0.0), 1.0 / 2.2), 1.0);
}

// ============================================================
// Miss
// ============================================================
[shader("miss")]
void Miss(inout RayPayload payload)
{
    const float height = normalize(WorldRayDirection()).y;
    const float3 horizonColor = float3(1.0, 0.38, 0.16);
    const float3 sunsetColor = float3(0.55, 0.24, 0.36);
    const float3 zenithColor = float3(0.08, 0.12, 0.28);
    const float3 groundColor = float3(0.08, 0.05, 0.06);

    float3 skyColor = lerp(horizonColor, sunsetColor, smoothstep(0.0, 0.3, height));
    skyColor = lerp(skyColor, zenithColor, smoothstep(0.2, 0.85, height));
    skyColor = lerp(skyColor, groundColor, smoothstep(0.0, 0.25, -height));
    payload.color = float4(skyColor, 1.0);
}

// ============================================================
// Closest Hit
// ============================================================
[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    if(payload.depth >= 3)
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
    float3 hitNormalWS = normalize(hitNormal);//mul((float3x3)objectToWorld, hitNormal);

    Material material = gMaterials[NonUniformResourceIndex(subMeshIndex)][0];
    Texture2D<float4> diffuseTexture = getTexture(NonUniformResourceIndex(material._diffuseTexture));
    const float4 diffuse = diffuseTexture.SampleLevel(bilinearRepeatSampler, uv, 0.0);

    const uint sampleCount = 1;
	float3 Lo = float3(0, 0, 0);
	for(uint i = 0; i < sampleCount; ++i)
	{
        float pdf;
        float3 wi;
        [branch]
        if(_samplingMode == 1)
        {
            wi = RandomHemisphereDirection(true, hitNormalWS, payload.rngState);
            float cosTheta = max(dot(hitNormalWS, wi), 0.0);
            pdf = cosTheta / PI;
        }
        else
        {
            wi = RandomHemisphereDirection(false, hitNormalWS, payload.rngState);
            pdf = 1.0 / PI2;
        }
		// if(roughness < 0.2)
		// {
		// 	    wi = normalize(-V + 2 * dot(V, hitNormalWS) * hitNormalWS);
		// }

		//hitValue._prevMonteCarlo = wi;

        RayDesc ray;
        ray.Origin = hitPositionWS;
        ray.Direction = wi;
        ray.TMin = 0.001;
        ray.TMax = 10000.0;

        RayPayload payload2;
        payload2.depth = payload.depth + 1;
        payload2.rngState = payload.rngState;
        payload2.color = float4(0, 0, 0, 1);
        TraceRay(gTLAS, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 1, 0, ray, payload2);

		const float3 brdf = diffuse.xyz / PI; //EvaluateBRDF(hitNormalWS, V, wi, hitColor, roughness, metallic);
		const float3 Li = payload2.color.xyz;
		const float cosTheta = dot(hitNormalWS, wi);
        [branch]
		if (_samplingMode == 1)
            Lo += diffuse.rgb * Li; // pdf가 0이 나올수 있어서 나눗셈을 하면안됨. 마침 cossampling시에는 pdf가 약분됨
        else
            Lo += (brdf * Li * max(dot(hitNormalWS, wi), 0.0)) / pdf;

        payload.rngState = payload2.rngState;
	}

    // if (diffuse.a <= 0.0)
    // {
    //     IgnoreHit();
    //     return;
    // }

    payload.color = float4((Lo / sampleCount), 1);
    //payload.color = float4(0, 1, 1, 1);
}

#endif
