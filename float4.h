#pragma once

struct float4
{
	float4()
		: x(0), y(0), z(0), w(0)
	{}

	float4(float _x, float _y, float _z, float _w);
	float4(const float3& rhs, const float _w);
	float4(const float3&& rhs, const float _w);

	dk_inline const float4& operator*(const int rhs) noexcept
	{
		x *= rhs;
		y *= rhs;
		z *= rhs;
		w *= rhs;

		return *this;
	}

	union
	{
		float m[4];
		struct
		{
			float x, y, z, w;
		};
	};
};