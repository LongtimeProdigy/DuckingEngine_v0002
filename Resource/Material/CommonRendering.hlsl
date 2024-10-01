#ifndef __DEFINE_COMMONRENDERING_HLSL__
#define __DEFINE_COMMONRENDERING_HLSL__

//based on google's omni-directional stereo rendering thread
#define FLOAT_EPSILON 2.4414e-4
#define FLOAT_MAX 3.402823466e+38F

cbuffer SceneConstantBuffer : register(b0)
{
    uint _frameIndex;

	uint2 _resolution;
    float _time;

    float _nearDistance;
    float _farDistance;
    float2 _padding1;

    float4x4 _cameraWorldMatrix;
    float4x4 _cameraWorldMatrixInv;
	float4x4 _cameraProjectionMatrix;
}

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