#include "stdafx.h"
#include "Transform.h"

#include "Matrix3x3.h"
#include "Matrix4x4.h"

const Transform Transform::Identity = Transform(float3(0, 0, 0), float3(0, 0, 0), float3(0, 0, 0));

void Transform::ToMatrix4x4(Matrix4x4& outMatrix) const noexcept 
{
	float roll = _rotation.z;
	float pitch = _rotation.x;
	float yaw = _rotation.y;

	outMatrix = Matrix4x4(
		(cos(roll) * cos(yaw) + sin(roll) * -sin(pitch) * -sin(yaw)) * _scale.x, sin(roll) * cos(pitch), cos(roll) * sin(yaw) + sin(roll) * -sin(pitch) * cos(yaw), 0,
		-sin(roll) * cos(yaw) + cos(roll) * -sin(pitch) * -sin(yaw), (cos(roll) * cos(pitch)) * _scale.y, -sin(roll) * sin(yaw) + cos(roll) * -sin(pitch) * cos(yaw), 0,
		cos(pitch) * -sin(yaw), sin(roll), (cos(pitch) * cos(yaw)) * _scale.z, 0,
		_position.x, _position.y, _position.z, 1
	);
}