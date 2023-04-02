#ifndef __DEFINE_COMMONRENDERING_HLSL__
#define __DEFINE_COMMONRENDERING_HLSL__

cbuffer SceneConstantBuffer : register(b0)
{
    uint4 _frameIndex;
    float4x4 _cameraWorldMatrix;
	float4x4 _cameraProjectionMatrix;
}

float3 getViewDirection()
{
    return _cameraWorldMatrix._13_23_33 * -1;
}

#endif