#ifndef ATMOSPHERE_PARAMETERS_HLSL
#define ATMOSPHERE_PARAMETERS_HLSL

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

    float _planetRadius;
    float3 _planetCentre;

    float _atmosphereRadius;
    float _sunAngularRadius;
    float _exposureEV;
    float _bloomStrength;

    float _bloomRadius;
    float _bloomThreshold;
    float _sunRadianceScale;
    float _padding;
};

#endif
