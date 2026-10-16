#ifndef ATMOSPHERE_PARAMETERS_HLSL
#define ATMOSPHERE_PARAMETERS_HLSL

#define ATEMOSPHERE_PRODUCTION 0
#define ATEMOSPHERE_REFERENCE 1
#define ATEMOSPHERE_LIMIT_DEBUG 2

cbuffer AtmosphereConstantBuffer : register(b1)
{
    uint _numInScatteringPoints;
    uint _opticalDepthPointCount;
    float _rayleighHeight;
    float _mieHeight;

    float _sunDegree;
    float _sunIntensity;
    float _scatteringScale;
    float _cameraAltitude;

    float _densityReferenceRadius;
    float3 _planetCentre;

    float _atmosphereRadius;
    float _sunAngularRadius;
    float _exposureEV;
    float _bloomStrength;

    float _bloomRadius;
    float _bloomThreshold;
    float _sunRadianceScale;
    float _densityTransitionRadius; // Inner approximation boundary, in metres from centre.

    float _integrationRelativeTolerance;
    uint _integrationMaxDepth;
    uint _atmosphereSamplingMode; // 0: production, 1: direct reference, 2: reference limit debug
    uint _transmittanceLutIndex;
};

#endif
