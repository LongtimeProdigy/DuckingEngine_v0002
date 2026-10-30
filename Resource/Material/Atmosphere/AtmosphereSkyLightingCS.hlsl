#define ATMOSPHERE_FORCE_LUT
#include "Atmosphere/AtmosphereTransmittance.hlsl"
#include "Atmosphere/AtmosphereSkyLighting.hlsl"
#include "Atmosphere/AtmosphereSurfaceLighting.hlsl"
#include "Atmosphere/AtmosphereMultipleScattering.hlsl"

cbuffer SkyLightingOutputConstants : register(b2)
{
    uint _skyOutputUav;
};

float3 skyDirection(float2 uv)
{
    const float phi = uv.x * 2.0f * PI;
    const float theta = uv.y * PI;
    return float3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
}

[numthreads(8, 8, 1)]
void CSRadiance(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= SKY_WIDTH || id.y >= SKY_HEIGHT) 
        return;

    const float3 dir = skyDirection((float2(id.xy) + 0.5f) / float2(SKY_WIDTH, SKY_HEIGHT));
    const float2 hit = raySphere(_planetCentre, _atmosphereRadius, 0.0f, dir);
    const float3 betaR = atmosphereRayleighScattering();
    const float3 betaM = atmosphereMieScattering();
    const float3 extinctionM = atmosphereMieExtinction();
    const float angle = radians(_sunDegree);
    const float3 sunDir = float3(0, sin(angle), cos(angle));
    const float mu = dot(dir, sunDir);
    const float phaseR = atmosphereRayleighPhase(mu), phaseM = atmosphereMiePhase(mu);
    float3 T = 1.0f, radiance = 0.0f;
    // Fixed work: 64 intervals per probe direction, no adaptive view integration.
    if (hit.y > 0.0f && _scatteringScale > 0.0f && _sunIrradiance > 0.0f)
    {
        [loop] for (uint i = 0u; i < 64u; ++i)
        {
            const float u0 = float(i) / 64.0f, u1 = float(i + 1u) / 64.0f;
            const float a = hit.y * u0 * u0 * u0, b = hit.y * u1 * u1 * u1;
            const float width = b - a;
            const float3 p = dir * (hit.x + 0.5f * (a + b));
            const float4 density = atmosphereDensity(atmosphereHeight(p));
            const float3 sigma = atmosphereOpticalExtinction(density);
            const float3 tau = sigma * width;
            const float3 segmentT = exp(-tau);
            // Stable integral of constant source and extinction within this interval.
            const float3 weight = float3(
                tau.x < 1e-3f ? width * (1.0f - tau.x * 0.5f) : (1.0f - segmentT.x) / max(sigma.x, 1e-30f),
                tau.y < 1e-3f ? width * (1.0f - tau.y * 0.5f) : (1.0f - segmentT.y) / max(sigma.y, 1e-30f),
                tau.z < 1e-3f ? width * (1.0f - tau.z * 0.5f) : (1.0f - segmentT.z) / max(sigma.z, 1e-30f));

            const float radius = max(_densityReferenceRadius + atmosphereHeight(p), 0.0f);
            GPUASSERT(abs(dot(p - _planetCentre, sunDir)) <= radius, float4(dot(p - _planetCentre, sunDir), length(p - _planetCentre), atmosphereHeight(p), length(p)));

            const float3 source = atmosphereSunIrradiance() * getSunTransmittance(p, sunDir) * atmosphereSurfaceSunVisibility(p)
                * (betaR * density.x * phaseR + betaM * density.y * phaseM)
                + atmosphereSurfaceBounce(p) * (betaR * density.x + betaM * density.y) / (4.0f * PI)
                + atmosphereMultipleIncident(atmosphereHeight(p), dot(p - _planetCentre, sunDir)) * (betaR * density.x + betaM * density.y);
            radiance += T * source * weight;
            T *= segmentT;
            if (all(T < 1e-6f)) break;
        }
    }
    // Solar disk excluded: direct sunlight is already evaluated by the surface BRDF.
    const RWTexture2D<float4> output = getTextureRW(_skyOutputUav);
    output[id.xy] = float4(radiance, 1);
}

float radicalInverse(uint bits) 
{ 
    return float(reversebits(bits)) * 2.3283064365386963e-10f; 
}

[numthreads(8, 8, 1)]
void CSFilter(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= SKY_WIDTH || id.y >= SKY_HEIGHT * SKY_BANDS) return;
    const uint band = id.y / SKY_HEIGHT;
    const float3 N = skyDirection((float2(id.x, id.y % SKY_HEIGHT) + 0.5f) / float2(SKY_WIDTH, SKY_HEIGHT));
    const float3 up = abs(N.y) < 0.99f ? float3(0, 1, 0) : float3(1, 0, 0);
    const float3 tangent = normalize(cross(up, N)), bitangent = cross(N, tangent);
    float3 sum = 0.0f;
    float weights = 0.0f;
    if (band == 0u) { sum = sampleSkyBand(_skyRadianceIndex, N, 0u); weights = 1.0f; }
    else
    {
        [loop] for (uint i = 0u; i < 64u; ++i)
        {
            const float2 xi = float2((float(i) + 0.5f) / 64.0f, radicalInverse(i));
            const float phi = 2.0f * PI * xi.y;
            float3 L;
            if (band == 5u)
            {
                const float sinTheta = sqrt(xi.x), cosTheta = sqrt(1.0f - xi.x);
                L = tangent * (cos(phi) * sinTheta) + bitangent * (sin(phi) * sinTheta) + N * cosTheta;
            }
            else
            {
                const float roughness = float(band) / 4.0f;
                const float a2 = pow(roughness, 4.0f);
                const float cosTheta = sqrt((1.0f - xi.x) / (1.0f + (a2 - 1.0f) * xi.x));
                const float sinTheta = sqrt(max(1.0f - cosTheta * cosTheta, 0.0f));
                const float3 H = tangent * (cos(phi) * sinTheta) + bitangent * (sin(phi) * sinTheta) + N * cosTheta;
                L = reflect(-N, H);
            }
            const float weight = band == 5u ? 1.0f : saturate(dot(N, L));
            sum += sampleSkyBand(_skyRadianceIndex, L, 0u) * weight;
            weights += weight;
        }
    }
    const RWTexture2D<float4> output = getTextureRW(_skyOutputUav);
    output[id.xy] = float4(sum / max(weights, 1e-6f) * (band == 5u ? PI : 1.0f), 1);
}
