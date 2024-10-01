#ifndef __DEFINE_DEBUGDRAWELEMENT_HLSL__
#define __DEFINE_DEBUGDRAWELEMENT_HLSL__

#include "CommonRendering.hlsl"

struct VS_INPUT
{
	float3 position : POSITION;
};

struct VS_OUTPUT
{
	float4 position: SV_POSITION;
	float4 color : COLOR;
};

// per DrawElement (Cube, Sphere, Capsule...etc)
struct SpherePrimitiveInfo
{
	float3 _worldPosition;
	float _radius;
	float3 _color;
	float _padding;
};
StructuredBuffer<SpherePrimitiveInfo> SpherePrimitiveInfoBuffer : register(t0);

// per DrawElement (Cube, Sphere, Capsule...etc)
struct LinePrimitiveInfo
{
	float3 _worldPosition[2];	// 0 : start , 1 : end
	float3 _color;
};
StructuredBuffer<LinePrimitiveInfo> LinePrimitiveInfoBuffer : register(t0);

float4 convertToWorldSpace(in float3 localPosition)
{
	float4 position = float4(localPosition, 1);
	position = mul(position, _cameraWorldMatrixInv);
	position = mul(position, _cameraProjectionMatrix);

	return position;
}

VS_OUTPUT VSMainSphere(VS_INPUT input, uint instanceID : SV_InstanceID)
{
	VS_OUTPUT output;

	const SpherePrimitiveInfo instanceData = SpherePrimitiveInfoBuffer[instanceID];

	float3 scaledPosition = input.position * instanceData._radius + instanceData._worldPosition;
	output.position = convertToWorldSpace(scaledPosition);
	output.color = float4(instanceData._color, 1);

	return output;
}

float4 PSMainSphere(VS_OUTPUT input) : SV_TARGET
{
	return input.color;
}

VS_OUTPUT VSMainLine(VS_INPUT input, uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
	VS_OUTPUT output;

	const LinePrimitiveInfo instanceData = LinePrimitiveInfoBuffer[instanceID];

	const float3 worldPosition = input.position + instanceData._worldPosition[vertexID];
	output.position = convertToWorldSpace(worldPosition);
	output.color = float4(instanceData._color, 1);

	return output;
}
float4 PSMainLine(VS_OUTPUT input) : SV_TARGET
{
	return input.color;
}

#endif