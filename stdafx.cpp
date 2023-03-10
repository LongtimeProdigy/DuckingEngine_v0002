#include "stdafx.h"

namespace DK
{
	DKString GlobalPath::kResourcePath;

	const float2 float2::Identity = float2(1, 1);
	const float2 float2::Zero = float2(0, 0);

	const float3 float3::Identity = float3(1, 1, 1);
	const float3 float3::Zero = float3(0, 0, 0);

	const Quaternion Quaternion::Identity = Quaternion(0, 0, 0, 1);

	const Transform Transform::Identity = Transform(float3::Zero, Quaternion::Identity, float3::Identity);

	const float3x3 float3x3::Identity = float3x3(1, 0, 0, 0, 1, 0, 0, 0, 1);

	const float4x4 float4x4::Identity = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

	void float4x4::toTransform(Transform& outTransform) const
	{
#ifdef USE_DIRECTX_MATH
		DirectX::XMMATRIX matrix;
		matrix.r[0].m128_f32[0] = _rows[0].x; matrix.r[0].m128_f32[1] = _rows[0].y; matrix.r[0].m128_f32[2] = _rows[0].z; matrix.r[0].m128_f32[3] = _rows[0].w;
		matrix.r[1].m128_f32[0] = _rows[1].x; matrix.r[1].m128_f32[1] = _rows[1].y; matrix.r[1].m128_f32[2] = _rows[1].z; matrix.r[1].m128_f32[3] = _rows[1].w;
		matrix.r[2].m128_f32[0] = _rows[2].x; matrix.r[2].m128_f32[1] = _rows[2].y; matrix.r[2].m128_f32[2] = _rows[2].z; matrix.r[2].m128_f32[3] = _rows[2].w;
		matrix.r[3].m128_f32[0] = _rows[3].x; matrix.r[3].m128_f32[1] = _rows[3].y; matrix.r[3].m128_f32[2] = _rows[3].z; matrix.r[3].m128_f32[3] = _rows[3].w;

		DirectX::XMVECTOR scale, quaternion, translation;
		DirectX::XMMatrixDecompose(&scale, &quaternion, &translation, matrix);

		outTransform = Transform(
			float3(translation.m128_f32[0], translation.m128_f32[1], translation.m128_f32[2]),
			Quaternion(quaternion.m128_f32[0], quaternion.m128_f32[1], quaternion.m128_f32[2], quaternion.m128_f32[3]), 
			float3(scale.m128_f32[0], scale.m128_f32[1], scale.m128_f32[2])
			);
#else
		static_assert(false, "Matrix Inverse 구현 필요합니다");
#endif
	}
}