
struct VS_INPUT
{
    float2 position : POSITION;
    float2 uv0 : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv0 : TEXCOORD0;
    float4 color : TEXCOORD1;
};

#define TextureParameter uint
#define BINDLESSTEXTUREARRAY_SPACE space10
Texture2D gBindlessTextureArray[] : register(t0, BINDLESSTEXTUREARRAY_SPACE);
SamplerState normalSampler : register(s0);

cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 _cameraWorldMatrixInv;
	float4x4 _cameraProjectionMatrix;
}

cbuffer TerrainStandard : register(b1)
{
    TextureParameter _heightTexture;
}

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    Texture2D heightTexture = gBindlessTextureArray[_heightTexture];
#if 1
    uint t_width = 0, t_height = 0;
    heightTexture.GetDimensions(t_width, t_height);
    float4 height = heightTexture.Load(uint3(input.uv0 * uint2(t_width, t_height), 0));
#else
    float4 height = heightTexture.Load(uint3(input.uv0 * 4096, 0));
#endif

    output.position = float4(input.position.x, height.x * 10, input.position.y, 1.0f);
    output.position = mul(output.position, _cameraWorldMatrixInv);
    output.position = mul(output.position, _cameraProjectionMatrix);

    output.uv0 = input.uv0;
    output.color = float4(t_width, t_height, 0, 1);
    
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    return input.color;
}