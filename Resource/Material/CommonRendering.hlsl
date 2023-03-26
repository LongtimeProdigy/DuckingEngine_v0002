#ifndef __DEFINE_COMMONRENDERING_HLSL__
#define __DEFINE_COMMONRENDERING_HLSL__

cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 _cameraWorldMatrix;
	float4x4 _cameraProjectionMatrix;
}

#endif