#ifndef POST_PROCESS_COMMON_HLSL
#define POST_PROCESS_COMMON_HLSL

#include "CommonRendering.hlsl"
#include "CommonTexture.hlsl"

struct VS_INPUT { float2 position : POSITION; };
struct VS_OUTPUT { float4 position : SV_POSITION; };

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = float4(input.position, 0, 1);
    return output;
}

float3 loadClamped(Texture2D<float4> source, int2 position)
{
    return source.Load(int3(clamp(position, int2(0, 0), int2(_resolution) - 1), 0)).rgb;
}

#endif
