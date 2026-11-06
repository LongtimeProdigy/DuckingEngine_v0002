#ifndef ATMOSPHERE_SKY_LIGHTING_HLSL
#define ATMOSPHERE_SKY_LIGHTING_HLSL
#include "CommonTexture.hlsl"
#include "CommonMathConstant.hlsl"
#include "Atmosphere/AtmosphereParameters.hlsl"
static const uint SKY_WIDTH = 64u;
static const uint SKY_HEIGHT = 32u;

float2 skyCoordinates(float3 direction)
{
    return float2(frac(atan2(direction.z, direction.x) / (2.0f * PI)), acos(clamp(direction.y, -1.0f, 1.0f)) / PI);
}
// Wrap longitude, clamp latitude, and never interpolate between atlas bands.
float3 sampleSkyBand(uint textureIndex, float3 direction, uint band)
{
    const float2 grid = skyCoordinates(normalize(direction)) * float2(SKY_WIDTH, SKY_HEIGHT) - 0.5f;
    const int2 p = int2(floor(grid));
    const float2 f = frac(grid);
    const int x0 = (p.x + int(SKY_WIDTH)) % int(SKY_WIDTH);
    const int x1 = (x0 + 1) % int(SKY_WIDTH);
    const int y0 = clamp(p.y, 0, int(SKY_HEIGHT) - 1) + int(band * SKY_HEIGHT);
    const int y1 = clamp(p.y + 1, 0, int(SKY_HEIGHT) - 1) + int(band * SKY_HEIGHT);
    const Texture2D<float4> source = getTexture(textureIndex);
    return lerp(lerp(source.Load(int3(x0, y0, 0)).rgb, source.Load(int3(x1, y0, 0)).rgb, f.x),
                lerp(source.Load(int3(x0, y1, 0)).rgb, source.Load(int3(x1, y1, 0)).rgb, f.x), f.y);
}

#endif
