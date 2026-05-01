#ifndef __DEFINE_INITIALSPECTRUM_HLSL__
#define __DEFINE_INITIALSPECTRUM_HLSL__

#include "CommonMath.hlsl"
#include "CommonTexture.hlsl"
#include "Ocean/OceanCommon.hlsl"

float Phillips(float2 k)
{
    const float kLen = length(k);
    if (kLen < FLOAT_EPSILON) 
        return 0.0;

    const float k2 = kLen * kLen;
    const float k4 = k2 * k2;

    const float2 kNorm = k / kLen;
    const float kw = dot(kNorm, normalize(_windDir));

    const float damping = exp(-1.0 / (k2 * _L * _L));
    return _A * damping * kw * kw / k4;
}

[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint x = id.x;
    const uint y = id.y;
    const float2 k = float2((x - _N/2) * 2.0 * 3.14159 / _L, (y - _N/2) * 2.0 * 3.14159 / _L);
    const float P = Phillips(k);

    const float2 gaussian = randGaussian(uint2(x, y));
    const float2 value = gaussian * sqrt(P * 0.5);
    RWTexture2D<float4> H0 = getTextureRW(_h0UAV);

    H0[id.xy] = float4(value.x, value.y, 0, 0);
}

#endif