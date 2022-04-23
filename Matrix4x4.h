#pragma once

#include "float4.h"

struct Matrix3x3;

struct Matrix4x4
{
	static const Matrix4x4 Identity;

	Matrix4x4()
		: _11(1), _12(0), _13(0), _14(0)
		, _21(0), _22(1), _23(0), _24(0)
		, _31(0), _32(0), _33(1), _34(0)
		, _41(0), _42(0), _43(0), _44(1)
	{}

	Matrix4x4(
		float __11, float __12, float __13, float __14
		, float __21, float __22, float __23, float __24
		, float __31, float __32, float __33, float __34
		, float __41, float __42, float __43, float __44
	)
		: _11(__11), _12(__12), _13(__13), _14(__14)
		, _21(__21), _22(__22), _23(__23), _24(__24)
		, _31(__31), _32(__32), _33(__33), _34(__34)
		, _41(__41), _42(__42), _43(__43), _44(__44)
	{}

	Matrix4x4(const Matrix3x3& matrix);

	dk_inline const Matrix4x4 operator*(const Matrix4x4& rhs) const noexcept
	{
		return Matrix4x4(
			_11 * rhs._11 + _12 * rhs._21 + _13 * rhs._31 + _14 * rhs._41,
			_11 * rhs._12 + _12 * rhs._22 + _13 * rhs._32 + _14 * rhs._42,
			_11 * rhs._13 + _12 * rhs._23 + _13 * rhs._33 + _14 * rhs._43,
			_11 * rhs._14 + _12 * rhs._24 + _13 * rhs._34 + _14 * rhs._44,

			_21 * rhs._11 + _22 * rhs._21 + _23 * rhs._31 + _24 * rhs._41,
			_21 * rhs._12 + _22 * rhs._22 + _23 * rhs._32 + _24 * rhs._42,
			_21 * rhs._13 + _22 * rhs._23 + _23 * rhs._33 + _24 * rhs._43,
			_21 * rhs._14 + _22 * rhs._24 + _23 * rhs._34 + _24 * rhs._44,

			_31 * rhs._11 + _32 * rhs._21 + _33 * rhs._31 + _34 * rhs._41,
			_31 * rhs._12 + _32 * rhs._22 + _33 * rhs._32 + _34 * rhs._42,
			_31 * rhs._13 + _32 * rhs._23 + _33 * rhs._33 + _34 * rhs._43,
			_31 * rhs._14 + _32 * rhs._24 + _33 * rhs._34 + _34 * rhs._44,

			_41 * rhs._11 + _42 * rhs._21 + _43 * rhs._31 + _44 * rhs._41,
			_41 * rhs._12 + _42 * rhs._22 + _43 * rhs._32 + _44 * rhs._42,
			_41 * rhs._13 + _42 * rhs._23 + _43 * rhs._33 + _44 * rhs._43,
			_41 * rhs._14 + _42 * rhs._24 + _43 * rhs._34 + _44 * rhs._44
		);
	}

	void Transpose() noexcept;
	void SetPosition(const float4& position) noexcept
	{
		_rows[3] = position;
	}

	union
	{
		float4 _rows[4];
		struct
		{
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
	};
};