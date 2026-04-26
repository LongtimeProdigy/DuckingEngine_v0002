#pragma once

namespace DK
{
	namespace MeshUtil
	{
		const bool createSphere(const uint32 tessellationX, const uint32 tessellationY, const float radius, DKVector<float3>& positionArr, DKVector<float3>& normalArr, DKVector<float2>& uvArr, DKVector<uint32>& indexArr);
		const bool createPlane(const float width, const float depth, const uint32 m, const uint32 n, DKVector<float3>& positionArr, DKVector<float3>& normalArr, DKVector<float2>& uvArr, DKVector<uint32>& indexArr);
	}
}