#include "stdafx.h"

namespace DK
{
	const float2 float2::Identity = float2(1, 1);
	const float2 float2::Zero = float2(0, 0);

	const float3 float3::Identity = float3(1, 1, 1);
	const float3 float3::Zero = float3(0, 0, 0);

	const Quaternion Quaternion::Identity = Quaternion(0, 0, 0, 1);

	const Transform Transform::Identity = Transform(float3::Zero, Quaternion::Identity, float3::Identity);

	const float3x3 float3x3::Identity = float3x3(1, 0, 0, 0, 1, 0, 0, 0, 1);

	const float4x4 float4x4::Identity = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
}