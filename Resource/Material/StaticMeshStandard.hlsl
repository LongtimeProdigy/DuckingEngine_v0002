struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv0 : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    float2 uv0 : TEXCOORD0;
};

#define TextureParameter uint
#define BINDLESSTEXTUREARRAY_SPACE space10
Texture2D gBindlessTextureArray[] : register(t0, BINDLESSTEXTUREARRAY_SPACE);
SamplerState normalSampler : register(s0);

cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 _cameraWorldMatrix;
	float4x4 _cameraProjectionMatrix;
}
cbuffer SceneObjectConstantBuffer : register(b1)
{
    float4x4 _worldMatrix;
}

cbuffer StaticMeshStandard : register(b2)
{
    TextureParameter _diffuseTexture;
    float _opacity;
}

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = float4(input.position, 1.0f);
    output.normal = float4(input.normal, 1.0f);
    output.uv0 = input.uv0;

    output.position = mul(output.position, _worldMatrix);
    output.position = mul(output.position, _cameraWorldMatrix);
    output.position = mul(output.position, _cameraProjectionMatrix);

    output.normal = mul(output.normal, _cameraWorldMatrix);

    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
#if 1
    Texture2D diffuseTexture = gBindlessTextureArray[_diffuseTexture];
    return diffuseTexture.Sample(normalSampler, input.uv0);
#else
    return float4(_opacity, 0, 0, 1);
#endif
}