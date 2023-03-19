
//#define NORMAL_TERRAIN
#ifdef NORMAL_TERRAIN
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
    float4x4 _cameraWorldMatrix;
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
    output.position = mul(output.position, _cameraWorldMatrix);
    output.position = mul(output.position, _cameraProjectionMatrix);

    output.uv0 = input.uv0;
    output.color = float4(t_width, t_height, 0, 1);
    
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    return input.color;
}
#endif

struct VS_INPUT
{
    float3 position : POSITION;
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

cbuffer TerrainMeshConstantBuffer : register(b2)
{
    float4 _base_scale;
    float4 _color;
    float4x4 _rotate;
}

#define TextureParameter uint
#define BINDLESSTEXTUREARRAY_SPACE space10
Texture2D gBindlessTextureArray[] : register(t0, BINDLESSTEXTUREARRAY_SPACE);
SamplerState normalSampler : register(s0);
cbuffer TerrainStandard : register(b1)
{
    TextureParameter _diffuseTexture;
    TextureParameter _heightTexture;
}

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    //output.position = float4(input.position.x, 0, input.position.z, 1.0f);
    float3 scaledPosition = input.position * _base_scale.z;
    output.position = float4(scaledPosition, 1.0f);
    output.position = mul(output.position, _rotate);
    output.position = float4(_base_scale.x, 0, _base_scale.y, 0) + output.position;

    Texture2D heightTexture = gBindlessTextureArray[_heightTexture];
    float meterPerTexel = 0.0625f;
    uint2 texSize;
    heightTexture.GetDimensions(texSize.x, texSize.y);
    float2 uv =  (float2(output.position.x, output.position.z) + texSize * meterPerTexel / 2) / (texSize * meterPerTexel);
    float4 height = heightTexture.Load(uint3(uv * texSize, 0));

    output.position.y = height.x * 20 - 15;

    output.position = mul(output.position, _cameraWorldMatrix);
    output.position = mul(output.position, _cameraProjectionMatrix);

    output.uv0 = uv;

    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
#if 0
    Texture2D diffuseTexture = gBindlessTextureArray[_diffuseTexture];
    return diffuseTexture.Sample(normalSampler, input.uv0);
#else
    return _color;
#endif
}