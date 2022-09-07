struct VS_INPUT
{
	float4 pos : POSITION;
	float4 color : COLOR;
};

struct VS_OUTPUT
{
	float4 pos: SV_POSITION;
	float4 color : COLOR;
};

// per Scene
cbuffer SceneConstantBufferStruct : register(b0)
{
	float4x4 cameraWorldMatrix;
	float4x4 cameraProjectionMatrix;
};

// per DrawElement (Cube, Sphere, Capsule...etc)
cbuffer SceneObjectConstantBufferStruct : register(b1)
{
	float4x4 worldMatrix;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
	VS_OUTPUT output;

	output.pos = mul(input.pos, cameraWorldMatrix);
	output.pos = mul(output.pos, cameraProjectionMatrix);
	output.color = input.color;

	return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
	return input.color;
}