#include "stdafx.h"
#include "float4.h"

float4::float4(float _x, float _y, float _z, float _w)
	: x(_x), y(_y), z(_z), w(_w)
{}

float4::float4(const float3& rhs, const float _w)
	: x(rhs.x), y(rhs.y), z(rhs.z), w(_w)
{}

float4::float4(const float3&& rhs, const float _w)
	: x(rhs.x), y(rhs.y), z(rhs.z), w(_w)
{}