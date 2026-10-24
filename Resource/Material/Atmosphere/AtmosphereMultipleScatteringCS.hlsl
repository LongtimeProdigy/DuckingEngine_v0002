#define ATMOSPHERE_FORCE_LUT
#include "Atmosphere/AtmosphereMultipleScattering.hlsl"
cbuffer MultipleScatteringOutputConstants : register(b2)
{
    uint _multipleOutputUav;
    uint _multipleInputSrv;
    uint _multipleIteration;
};
float3 msDirection(uint i)
{
    const float y = 1.0f - 2.0f * (float(i) + 0.5f) / 32.0f;
    const float angle = float(i) * 2.39996323f;
    const float s = sqrt(max(1.0f - y * y, 0.0f));
    return float3(s * cos(angle), y, s * sin(angle));
}
[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= ATMOSPHERE_MS_WIDTH || id.y >= ATMOSPHERE_MS_HEIGHT) return;
    const float height = atmosphereMsHeight(float(id.y) / (ATMOSPHERE_MS_HEIGHT - 1u));
    const float radius = max(_densityReferenceRadius + height, 0.0f);
    const float muSun = 2.0f * float(id.x) / (ATMOSPHERE_MS_WIDTH - 1u) - 1.0f;
    const float3 sun = float3(sqrt(max(1.0f - muSun * muSun, 0.0f)), muSun, 0);
    const float3 betaR = atmosphereRayleighScattering(), betaM = atmosphereMieScattering();
    float3 sum = 0.0f;
    // Normalize discrete phase weights so coarse angular quadrature cannot add energy.
    float2 phaseSum = 0.0f;
    [loop] for (uint phaseIndex = 0u; phaseIndex < 32u; ++phaseIndex)
    {
        const float mu = dot(msDirection(phaseIndex), sun);
        phaseSum += float2(atmosphereRayleighPhase(mu), atmosphereMiePhase(mu));
    }
    [loop] for (uint d = 0u; d < 32u; ++d)
    {
        const float3 dir = msDirection(d);
        const float radialDot = radius * dir.y;
        const float distance = atmosphereTopDistance(height, radialDot);
        const float mu = dot(dir, sun);
        const float2 phase = float2(atmosphereRayleighPhase(mu), atmosphereMiePhase(mu))
            * (32.0f / (4.0f * PI)) / max(phaseSum, 1e-6f);
        float3 T = 1.0f, L = 0.0f;
        // Fixed work, cubic intervals favor the visible near end of thick gas rays.
        [loop] for (uint step = 0u; step < 32u; ++step)
        {
            const float u0 = float(step) / 32.0f, u1 = float(step + 1u) / 32.0f;
            const float a = distance * u0 * u0 * u0, b = distance * u1 * u1 * u1;
            const float width = b - a, t = 0.5f * (a + b);
            const float h = atmosphereHeightOnRay(height, radialDot, t);
            const float4 density = atmosphereDensity(h);
            const float3 sigma = atmosphereOpticalExtinction(density);
            const float3 scattering = betaR * density.x + betaM * density.y;
            const float3 tau = sigma * width, segmentT = exp(-tau);
            const float3 weight = float3(
                tau.x < 1e-3f ? width * (1.0f - 0.5f * tau.x) : (1.0f - segmentT.x) / max(sigma.x, 1e-30f),
                tau.y < 1e-3f ? width * (1.0f - 0.5f * tau.y) : (1.0f - segmentT.y) / max(sigma.y, 1e-30f),
                tau.z < 1e-3f ? width * (1.0f - 0.5f * tau.z) : (1.0f - segmentT.z) / max(sigma.z, 1e-30f));
            const float radialSun = radius * sun.y + t * dot(dir, sun);
            const AtmosphereOpticalDepth optical = atmosphereSunOpticalDepth(h, radialSun, atmosphereTopDistance(h, radialSun));
            const float3 direct = exp(-atmosphereOpticalExtinction(optical.value))
                * (betaR * density.x * phase.x + betaM * density.y * phase.y);
            float3 indirect = 0.0f;
            if (_multipleIteration > 0u)
            {
                const float r = max(_densityReferenceRadius + h, 0.0f);
                indirect = scattering * sampleAtmosphereMs(_multipleInputSrv, h, r > 1e-3f ? radialSun / r : 0.0f);
            }
            L += T * (direct + indirect) * weight;
            T *= segmentT;
            if (all(T < 1e-6f)) break;
        }
        sum += L / 32.0f;
    }
    // Recurrence J[n] = average(transport(direct + sigma_s * J[n-1])).
    // MaxOrder - 1 iterations store incident orders 1..MaxOrder - 1;
    // the final camera scattering adds orders 2..MaxOrder.
    // Spherical atmosphere only: no opaque reference sphere or captured surface bounce.
    const RWTexture2D<float4> output = getTextureRW(_multipleOutputUav);
    output[id.xy] = float4(max(sum, 0.0f), 1.0f);
}
