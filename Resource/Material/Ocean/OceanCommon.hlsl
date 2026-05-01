#ifndef __DEFINE_OCEANCOMMON_HLSL__
#define __DEFINE_OCEANCOMMON_HLSL__

cbuffer OceanParams : register(b1)
{
    float _timeForOcean;
    float _g;
    uint _stages;
    float2 _windDir;
    float _A;
    float _L;
    uint _N;
    TextureParameter _h0SRV;
    TextureParameter _h0UAV;
    TextureParameter _htSRV;
    TextureParameter _htUAV;
}

#endif