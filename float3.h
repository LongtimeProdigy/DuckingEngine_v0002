#pragma once

struct float3
{
	static const float3 Identity;
	static const float3 Zero;

	float3()
		: m{ 0, 0, 0 }
	{}

	float3(float _x, float _y, float _z)
		: m{ _x, _y, _z }
	{}

	dk_inline void operator+=(const float3& rhs) noexcept
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
	}

	dk_inline const float3 operator+(const float3& rhs) const noexcept
	{
		return float3(x + rhs.x, y + rhs.y, z + rhs.z);
	}

	dk_inline float3 operator-(const float3& rhs) const noexcept
	{
		return float3(x - rhs.x, y - rhs.y, z - rhs.z);
	}

	dk_inline void operator*=(const int rhs) noexcept
	{
		x *= rhs;
		y *= rhs;
		z *= rhs;
	}

public:
	union
	{
		float m[3];
		struct
		{
			float x, y, z;
		};
	};
};

dk_inline float3 operator*(const float3& lhs, const float rhs)
{
	return float3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
}