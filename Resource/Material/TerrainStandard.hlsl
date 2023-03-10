struct VS_INPUT
{
    float2 position : POSITION;
    float2 uv0 : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv0 : TEXCOORD0;
};

cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 _cameraWorldMatrix;
	float4x4 _cameraProjectionMatrix;
}

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = float4(input.position.x, 0.0f, input.position.y, 1.0f);
    output.position = mul(output.position, _cameraWorldMatrix);
    output.position = mul(output.position, _cameraProjectionMatrix);

    output.uv0 = input.uv0;
    
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    return float4(1, 1, 1, 1);
}