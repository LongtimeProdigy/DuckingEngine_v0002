#ifndef __DEFINE_COMMONRENDERING_HLSL__
#define __DEFINE_COMMONRENDERING_HLSL__

#include "CommonMath.hlsl"

#define DK_SHADER_SCENE
#include "ShaderStructures.h"
#undef DK_SHADER_SCENE

float getFOVTangent()
{
    return 1 / _cameraProjectionMatrix._22;
}

float3 getViewForwardDirection()
{
    return _cameraWorldMatrix._31_32_33;
}
float3 getViewRightDirection()
{
    return _cameraWorldMatrix._11_12_13;
}
float3 getViewUpDirection()
{
    return _cameraWorldMatrix._21_22_23;
}
float3 getViewPosition()
{
    return _cameraWorldMatrix._41_42_43;
}

#endif