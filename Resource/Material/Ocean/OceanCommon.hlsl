#ifndef __DEFINE_OCEANCOMMON_HLSL__
#define __DEFINE_OCEANCOMMON_HLSL__

cbuffer OceanParams : register(b1)
{
    float _timeForOcean;
    float _g;
    uint _stages;
    float _heightScale;

    float2 _windDir;
    uint _length;
    float _A;

    float _L;
    uint _N;
    TextureParameter _h0SRV;
    TextureParameter _htSRV;

    TextureParameter _heightSRV;
    TextureParameter _normalSRV;
    TextureParameter _h0UAV;
    TextureParameter _htUAV;

    TextureParameter _heightUAV;
    TextureParameter _normalUAV;
}

#endif