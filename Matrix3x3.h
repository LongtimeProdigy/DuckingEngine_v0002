#pragma once

struct Matrix3x3
{
	static const Matrix3x3 Identity;

	Matrix3x3() : _rows{ float3(0,0,0), float3(0,0,0), float3(0,0,0) }
	{};
	Matrix3x3(float __11, float __12, float __13, float __21, float __22, float __23, float __31, float __32, float __33)
		: _11(__11), _12(__12), _13(__13),
		_21(__21), _22(__22), _23(__23),
		_31(__31), _32(__32), _33(__33)
	{}

	dk_inline void Transpose() noexcept
	{
		Swap(_12, _21);
		Swap(_13, _31);
		Swap(_23, _32);
	}

	dk_inline const Matrix3x3 operator*(const Matrix3x3& rhs) noexcept
	{
		Matrix3x3 temp;
		temp._11 = _11 * rhs._11 + _12 * rhs._21 + _13 * rhs._31;
		temp._12 = _11 * rhs._12 + _12 * rhs._22 + _13 * rhs._32;
		temp._13 = _11 * rhs._13 + _12 * rhs._23 + _13 * rhs._33;
		temp._21 = _21 * rhs._11 + _22 * rhs._21 + _23 * rhs._31;
		temp._22 = _21 * rhs._12 + _22 * rhs._22 + _23 * rhs._32;
		temp._23 = _21 * rhs._13 + _22 * rhs._23 + _23 * rhs._33;
		temp._31 = _31 * rhs._11 + _32 * rhs._21 + _33 * rhs._31;
		temp._32 = _31 * rhs._12 + _32 * rhs._22 + _33 * rhs._32;
		temp._33 = _31 * rhs._13 + _32 * rhs._23 + _33 * rhs._33;

		return temp;
	}

	union
	{
		float3 _rows[3];
		struct
		{
			float _11, _12, _13;
			float _21, _22, _23;
			float _31, _32, _33;
		};
	};
};

dk_inline float3 operator*(const float3& position, const Matrix3x3& rotationMatrix)
{
	return float3(
		position.x * rotationMatrix._11 + position.y * rotationMatrix._21 + position.z * rotationMatrix._31
		, position.x * rotationMatrix._12 + position.y * rotationMatrix._22 + position.z * rotationMatrix._32
		, position.x * rotationMatrix._13 + position.y * rotationMatrix._23 + position.z * rotationMatrix._33
	);
}