struct VS_INPUT
{
	float3 position : POSITION;
};

struct VS_OUTPUT
{
	float4 position: SV_POSITION;
	float4 color : COLOR;
};

// per Scene
cbuffer SceneConstantBuffer : register(b0)
{
	float4x4 _cameraWorldMatrix;
	float4x4 _cameraProjectionMatrix;
};

// per DrawElement (Cube, Sphere, Capsule...etc)
#define USE_STRUCTURED_BUFFER		// 사용안할시 RenderPass도 수정해야함
#ifdef USE_STRUCTURED_BUFFER
struct SpherePrimitiveInfo
{
	float3 _worldPosition;
	float _radius;
	float3 _color;
	float _padding;
};
StructuredBuffer<SpherePrimitiveInfo> SpherePrimitiveInfoBuffer : register(t0);
#else
cbuffer SpherePrimitiveInfoBuffer : register(b1)
{
	float3 _worldPosition;
	float _radius;
	float3 _color;
	float _padding;
};
#endif

// struct exp0
// {
//     int Vertex_ID;
//     int Primitive_ID;
//     int Instance_ID;
//     float4 SV_POSITION;
//     float4 COLOR;
// }
VS_OUTPUT VSMainSphere(VS_INPUT input, uint instance : SV_InstanceID)
{
	VS_OUTPUT output;

#ifdef USE_STRUCTURED_BUFFER
	SpherePrimitiveInfo instanceData = SpherePrimitiveInfoBuffer[instance];
	float _radius = instanceData._radius;
	float3 _worldPosition = instanceData._worldPosition;
	float3 _color = instanceData._color;
#endif

	float3 scaledPosition = input.position * _radius + _worldPosition;
	output.position = float4(scaledPosition, 1);
#if 0
    float4x4 temp = 
    {
        1, 0, 0, 0, 
        0, 1, 0, -0.5, 
        0, 0, 1, 1, 
        0, 0, 0, 1
    };
    float4x4 temp2 =
    {
        0.562500179, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1.00100100, -1.00100100,
        0, 0, 1, 0
    };
    output.position = mul(temp, output.position);
    output.position = mul(temp2, output.position);
#else
	output.position = mul(_cameraWorldMatrix, output.position);
	output.position = mul(_cameraProjectionMatrix, output.position);
#endif
	output.color = float4(_color, 1);

	return output;
}

float4 PSMainSphere(VS_OUTPUT input) : SV_TARGET
{
	return input.color;
}