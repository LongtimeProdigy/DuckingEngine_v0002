#include "AtmosphereTransmittance.hlsl"
cbuffer AtmosphereLutOutputConstants : register(b2)
{
    uint _atmosphereLutUav;
};

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= ATMOSPHERE_LUT_WIDTH || id.y >= ATMOSPHERE_LUT_HEIGHT) 
        return;
        
    const float2 uv = float2(id.xy) / float2(ATMOSPHERE_LUT_WIDTH - 1u, ATMOSPHERE_LUT_HEIGHT - 1u);
    const float2 parameters = atmosphereLutParameters(uv);
    const float height = parameters.x;
    const float radialDot = (_densityReferenceRadius + height) * parameters.y;
    const float pathLength = atmosphereTopDistance(height, radialDot);
    const AtmosphereOpticalDepth optical = integrateAtmosphereOpticalDepth(
        height, radialDot, pathLength, _opticalDepthPointCount);
    const RWTexture2D<float4> output = getTextureRW(_atmosphereLutUav);
    output[id.xy] = float4(optical.value, float(optical.limitReached), 0.0f);
}
