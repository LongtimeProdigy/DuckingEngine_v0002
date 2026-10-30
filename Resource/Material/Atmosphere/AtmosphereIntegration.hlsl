#ifndef ATMOSPHERE_INTEGRATION_HLSL
#define ATMOSPHERE_INTEGRATION_HLSL

#include "Atmosphere/AtmosphereParameters.hlsl"
#include "CommonMathConstant.hlsl"

static const float3 gStandardBetaR = float3(3.8e-6f, 13.5e-6f, 33.1e-6f);
static const float3 gStandardBetaM = float3(21e-6f, 21e-6f, 21e-6f);

// Rayleigh의 산란계수 또는 소멸계수 (레일리는 흡수계수가 0이므로 사실상 소멸계수 == 산란계수)
float3 atmosphereRayleighScattering() 
{ 
    GPUASSERT(_scatteringScale >= 0);
    GPUASSERT(_rayleighScale >= 0);
    return gStandardBetaR * _scatteringScale * _rayleighScale; 
}
// Mie의 산란계수
float3 atmosphereMieScattering() 
{ 
    GPUASSERT(_scatteringScale >= 0);
    GPUASSERT(_mieScale >= 0);
    return gStandardBetaM * _scatteringScale * _mieScale; 
}
// Mie의 소멸계수 (산란계수 + 흡수계수)
float3 atmosphereMieExtinction()
{
    GPUASSERT(_scatteringScale >= 0);
    GPUASSERT(_mieScale >= 0);
    GPUASSERT(_mieAbsorptionScale >= 0);
    return gStandardBetaM * _scatteringScale * (_mieScale + _mieAbsorptionScale);
}
// 흡수 레이어의 소멸계수, 순수 흡수 성분 (ex. 오존층)
float3 atmosphereAbsorptionExtinction(uint layer)
{
    GPUASSERT(_scatteringScale >= 0);
    GPUASSERT(all(_absorptionLayers[layer].extinction >= 0));
    GPUASSERT(_absorptionLayers[layer].strength >= 0);
    return _absorptionLayers[layer].extinction * _absorptionLayers[layer].strength * _scatteringScale;
}
// 흡수층에서 흡수밀도의 비율을 높이에 따라 반환 (흡수율이 아님!)
float atmosphereAbsorptionDensity(float heightAtReference, uint layer)
{
    GPUASSERT(_absorptionLayers[layer].lowerWidth > 0);
    GPUASSERT(_absorptionLayers[layer].upperWidth > 0);
    const float peakHeightAtReference = _absorptionLayers[layer].peakRadius - _densityReferenceRadius;
    const float delta = heightAtReference - peakHeightAtReference;
    const float width = delta < 0.0f ? _absorptionLayers[layer].lowerWidth : _absorptionLayers[layer].upperWidth;
    return saturate(1.0f - (abs(delta) / width));
}

// columns가 한 지점의 밀도이면 소멸계수가 반환
// comumns가 거리에 적분된 밀도이면 opticalDepth가 반환 (이유는 반환된 밀도를 거리로 적분하면 그게 opticalDepth이니까)
float3 atmosphereOpticalExtinction(float4 columns)
{
    return atmosphereRayleighScattering() * columns.x + atmosphereMieExtinction() * columns.y + atmosphereAbsorptionExtinction(0u) * columns.z + atmosphereAbsorptionExtinction(1u) * columns.w;
}

float atmosphereRayleighPhase(float mu) { return 3.0f / (16.0f * PI) * (1.0f + mu * mu); }
float atmosphereMiePhase(float mu)
{
    const float g = clamp(_mieAnisotropy, -0.95f, 0.95f);
    return (1.0f - g * g) / (4.0f * PI * pow(max(1.0f + g * g - 2.0f * g * mu, 1e-4f), 1.5f));
}

// Mean transmittance through a constant-extinction interval: integral_0^1 exp(-tau*u) du.
// The series avoids cancellation at small optical depths, including vacuum.
float3 atmosphereIntervalWeight(float3 tau)
{
    const float3 regular = (1.0f - exp(-tau)) / max(tau, 1e-30f);
    const float3 series = 1.0f - 0.5f * tau + tau * tau / 6.0f;
    return float3(tau.x < 1e-3f ? series.x : regular.x,
                  tau.y < 1e-3f ? series.y : regular.y,
                  tau.z < 1e-3f ? series.z : regular.z);
}

// _densityReferenceRadius을 기준으로 _densityTransitionRadius의 높이를 반환
float atmosphereTransitionHeight()
{
    GPUASSERT(_densityTransitionRadius < _densityReferenceRadius);
    return _densityTransitionRadius - _densityReferenceRadius;
}

// rayleigh, mie 밀도 반환
float2 atmosphereExponentialDensity(float height)
{
    GPUASSERT(_rayleighHeight > 0);
    GPUASSERT(_mieHeight > 0);
    return exp(min(-height / float2(_rayleighHeight, _mieHeight), 50.0f));
}

// 주어진 고도에서 Rayleigh·Mie·두 흡수층의 상대 밀도를 반환합니다.
// x : Rayleigh 밀도, y : Mie 밀도, z : 흡수층 0의 밀도, w : 흡수층 1의 밀도
float4 atmosphereDensity(float heightAtReference)
{
    const float transitionHeight = atmosphereTransitionHeight();
    // transitionRadius 이상이면 투과율 적분 계산
    if (heightAtReference >= transitionHeight)
        return float4(atmosphereExponentialDensity(heightAtReference), atmosphereAbsorptionDensity(heightAtReference, 0u), atmosphereAbsorptionDensity(heightAtReference, 1u));

    GPUASSERT(_densityTransitionRadius > 0);

    // transtionRadius 미만이면 근사식으로 계산. 근사식 : transitionRadius위치의 밀도 * (1 + heightAtTransition / H - heightAtTransition² / (2 * H * _densityTransitionRadius)); H는 각각 _rayleighHeight, _mieHeight이다.
    const float heightAtTransition = clamp(transitionHeight - heightAtReference, 0.0f, _densityTransitionRadius);
    const float2 compression = 1.0f + heightAtTransition * (1.0f - 0.5f * heightAtTransition / _densityTransitionRadius) / float2(_rayleighHeight, _mieHeight);
    return float4(atmosphereExponentialDensity(transitionHeight) * compression, atmosphereAbsorptionDensity(heightAtReference, 0u), atmosphereAbsorptionDensity(heightAtReference, 1u));
}

// 광선 원점에서 t만큼 이동한 위치의 고도를 계산합니다. 고도 기준은 _densityReferenceRadius입니다.
float atmosphereHeightOnRay(float heightAtReference, float radialDot, float t)
{
    // 반환값 : h(t), 광선 원점에서 t만큼 이동한 고도 : r(t), 기준 반지름 : R (여기선 _densityReferenceRadius)
    // h(t) = r(t)-R를 구하는 게 다지만 정밀도 문제로 (r(t)²-R²) / (r(t)+R) 형태로 변환해서 계산
    const float numerator = heightAtReference * (2.0f * _densityReferenceRadius + heightAtReference) + t * (2.0f * radialDot + t);
    const float radius = sqrt(max(_densityReferenceRadius * _densityReferenceRadius + numerator, 0.0f));
    return numerator / max(radius + _densityReferenceRadius, 1e-10f);
}

#include "Atmosphere/AtmosphereAbsorptionCore.hlsl"

// 적분 구간 안에서 밀도 변화의 중요한 지점을 찾아, 분할할 위치 t를 반환
// 1순위 : radialDot이 있으면 그곳을 반환 (radialDot은 rayOrigin과 행성 중심에서 ray에 수선의 발을 내려서 직교하는 점까지의 거리)
// 2순위 : transitionRadius와 접점 near -> far 순서로 반환
// 3순위 : absortionLayer 검사 후 접점 near -> far 순서로 반환
float atmosphereSplitPoint(float heightAtReference, float radialDot, float startPos, float endPos)
{
    // radialDot은 방사방향을 rayDir로 투영했을 때의 거리 == 행성 중심에서 rayDir에 수선의 발을 내렸을때 만나는 지점까지의 거리
    // 즉, rayDir 방향에서 행성 중심까지 가장 가까운 위치. 즉 밀도가 가장 높은 부분

    const float margin = max((endPos - startPos) * 1e-5f, 1e-10f);
    if (-radialDot > startPos + margin && -radialDot < endPos - margin) // 구간 내에 radialDot이 있으면 그게 밀도가 가장 높은 부분이므로 바로 반환
        return -radialDot;

    // _densityTransitionRadius 구와 접하는 점 구하기
    const float heightAtTransition = heightAtReference - atmosphereTransitionHeight();
    const float c = heightAtTransition * (2.0f * _densityTransitionRadius + heightAtTransition); // 정밀도 때문에 이렇게 계산
    const float det = radialDot * radialDot - c;
    if (det >= 0.0f)    // TransitionRadius 구와 rayDir이 겹치는 점이 1~2개가 있음
    {
        const float root = sqrt(det);
        const float q = -radialDot - (radialDot >= 0.0f ? root : -root);    // 정밀도 문제로 부호가 같은 것끼리 빼주기위함
        const float t0 = q;
        const float t1 = abs(q) > 1e-10f ? c / q : -radialDot;
        const float nearT = min(t0, t1);
        const float farT = max(t0, t1);

        // start부터 점진적으로 segment를 나누며 진행하기 때문에 near를 먼저 검사후 내보냄
        if (nearT > startPos + margin && nearT < endPos - margin)   // near가 segment내에 있으면 반환
            return nearT;
        if (farT > startPos + margin && farT < endPos - margin)     // far가 segment내에 있으면 반환
            return farT;
    }

    // peak - lowerWidth  // 흡수가 시작되는 아래 경계
    // peak               // 흡수 밀도가 최대인 위치
    // peak + upperWidth  // 흡수가 끝나는 위 경계
    [unroll] 
    for (uint layer = 0u; layer < 2u; ++layer)
    {
        if (all(atmosphereAbsorptionExtinction(layer) <= 0.0f))
            continue;

        GPUASSERT(_absorptionLayers[layer].peakRadius >= 0);
        const float peak = _absorptionLayers[layer].peakRadius;
        GPUASSERT(peak - _absorptionLayers[layer].lowerWidth >= 0);
        const float radii[3] = { peak - _absorptionLayers[layer].lowerWidth, peak, peak + _absorptionLayers[layer].upperWidth };
        [unroll]
        for (uint boundary = 0u; boundary < 3u; ++boundary)
        {
            const float2 hits = atmosphereAbsorptionSphereHits(heightAtReference, radialDot, radii[boundary]);
            if (hits.x > startPos + margin && hits.x < endPos - margin) 
                return hits.x;
            if (hits.y > startPos + margin && hits.y < endPos - margin) 
                return hits.y;
        }
    }

    return -1.0f;
}

struct AtmosphereOpticalDepth
{
    float4 value;
    uint limitReached;
};

// pathLength를 진행하여 밀도의 차이에 따라 길이를 줄이고 늘려가며 opticalDepth의 적분값을 반환
AtmosphereOpticalDepth integrateAtmosphereOpticalDepth(float heightAtReference, float radialDot, float pathLength)
{
    AtmosphereOpticalDepth result;
    result.value = 0.0f;
    result.limitReached = 0u;

    if (pathLength <= 0.0f) 
        return result;

    const float3 betaR = atmosphereRayleighScattering();
    const float3 betaM = atmosphereMieExtinction();
    const float3 absorption0 = atmosphereAbsorptionExtinction(0u);
    const float3 absorption1 = atmosphereAbsorptionExtinction(1u);
    const float4 betaMax = float4(
        max(betaR.x, max(betaR.y, betaR.z)), max(betaM.x, max(betaM.y, betaM.z)), 
        max(absorption0.x, max(absorption0.y, absorption0.z)), max(absorption1.x, max(absorption1.y, absorption1.z))
        );
        
    uint visitedNodes = 0u;
    float3 stack[12];   // x는 startpoint, y는 nextpoint, z는 depth
    [loop]
    for (uint seed = 0u; seed < _opticalDepthPointCount; ++seed)
    {
        uint remainStackCount = 1u;
        stack[0] = float3(pathLength * (float(seed) / _opticalDepthPointCount), pathLength * (float(seed + 1u) / _opticalDepthPointCount), 0.0f);
        [loop]
        while (remainStackCount > 0u)
        {
            ++visitedNodes;
            const float3 node = stack[--remainStackCount];
            const float startPos = node.x;
            const float endPos = node.y;
            const float midPos = 0.5f * (startPos + endPos);
            const float length = endPos - startPos;
            const uint depth = uint(node.z);

            const float geometrySplit = atmosphereSplitPoint(heightAtReference, radialDot, startPos, endPos);
            const float4 densityAtMid = atmosphereDensity(atmosphereHeightOnRay(heightAtReference, radialDot, midPos)) * length;
            const float split = geometrySplit >= 0.0f ? geometrySplit : midPos;
            const float4 fine = atmosphereDensity(atmosphereHeightOnRay(heightAtReference, radialDot, 0.5f * (startPos + split)))  * (split - startPos) 
                + atmosphereDensity(atmosphereHeightOnRay(heightAtReference, radialDot, 0.5f * (split + endPos))) * (endPos - split);
            GPUASSERT(all(isfinite(fine)) && all(fine >= 0));

            const float4 opticalDepthError = abs(fine - densityAtMid) * betaMax;

            // 길이가 짧을수록 절대 허용 오차도 줄이고, opticalDepth의 값이 작을수록 절대 허용 오차도 줄인다. 즉, 값이 작을수록 오차도 엄격하게 적용한다.
            // 첫항: 구간 길이에 비례해 배분한 절대 허용 오차, 둘째 항: 적분값 크기에 비례하는 상대 허용 오차
            const float4 tolerance = 1e-5f * (length / pathLength) + _integrationRelativeTolerance * (fine * betaMax);
            const bool needsSplit = geometrySplit >= 0.0f || any(opticalDepthError > tolerance);
            // (_opticalDepthPointCount - seed - 1u) : 아직 시작하지 않은 sement 수
            // + 2u : 이번에 추가 될 left, right node 2개
            if (needsSplit && depth < _integrationMaxDepth && split > startPos && split < endPos && visitedNodes + remainStackCount + (_opticalDepthPointCount - seed - 1u) + 2u <= _opticalNodeBudget)
            {
                stack[remainStackCount++] = float3(split, endPos, depth + 1u);
                stack[remainStackCount++] = float3(startPos, split, depth + 1u);
            }
            else
            {
                result.value += fine;
                result.limitReached |= needsSplit ? 1u : 0u;
            }
        }
    }

    return result;
}
#endif
