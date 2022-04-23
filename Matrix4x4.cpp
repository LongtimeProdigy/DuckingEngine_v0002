#include "stdafx.h"
#include "Matrix4x4.h"

#include "Matrix3x3.h"

const Matrix4x4 Matrix4x4::Identity = Matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

Matrix4x4::Matrix4x4(const Matrix3x3& matrix)
	: _11(matrix._11), _12(matrix._12), _13(matrix._13), _14(0)
	, _21(matrix._21), _22(matrix._22), _23(matrix._23), _24(0)
	, _31(matrix._31), _32(matrix._32), _33(matrix._33), _34(0)
	, _41(0), _42(0), _43(0), _44(1)
{}

void Matrix4x4::Transpose() noexcept
{
	Swap(_12, _21);
	Swap(_13, _31);
	Swap(_14, _41);
	Swap(_23, _32);
	Swap(_24, _42);
	Swap(_34, _43);
}