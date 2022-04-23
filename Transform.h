#pragma once

#include "float3.h"
#include "Matrix3x3.h"

struct Matrix4x4;

struct Transform
{
public: const static Transform Identity;
public:
	Transform()
		: _position(float3(0, 0, 0))
		, _rotation(float3(0, 0, 0))
		, _scale(float3(1, 1, 1))
	{}
	Transform(const float3& position, const float3& rotation, const float3& scale)
		: _position(position)
		, _rotation(rotation)
		, _scale(scale)
	{}

	dk_inline void operator+=(const Transform& rhs) noexcept
	{
		_position += rhs._position;
		_rotation += rhs._rotation;
		_scale += rhs._scale;
	}

	void ToMatrix4x4(Matrix4x4& outMatrix) const noexcept;

	dk_inline void GetRollRotationMatrix(Matrix3x3& outMatrix) const noexcept
	{
		float roll = _rotation.z;

		outMatrix._11 = cos(roll);
		outMatrix._12 = -sin(roll);
		outMatrix._13 = 0;
		outMatrix._21 = sin(roll);
		outMatrix._22 = cos(roll);
		outMatrix._23 = 0;
		outMatrix._31 = 0;
		outMatrix._32 = 0;
		outMatrix._33 = 1;
	}
	dk_inline void GetYawRotationMatrix(Matrix3x3& outMatrix) const noexcept
	{
		float yaw = _rotation.y;

		outMatrix._11 = cos(yaw);
		outMatrix._12 = 0;
		outMatrix._13 = sin(yaw);
		outMatrix._21 = 0;
		outMatrix._22 = 1;
		outMatrix._23 = 0;
		outMatrix._31 = -sin(yaw);
		outMatrix._32 = 0;
		outMatrix._33 = cos(yaw);
	}

	dk_inline void GetPitchRotationMatrix(Matrix3x3& outMatrix) const noexcept
	{
		float pitch = _rotation.x;

		outMatrix._11 = 1;
		outMatrix._12 = 0;
		outMatrix._13 = 0;
		outMatrix._21 = 0;
		outMatrix._22 = cos(pitch);
		outMatrix._23 = -sin(pitch);
		outMatrix._31 = 0;
		outMatrix._32 = sin(pitch);
		outMatrix._33 = cos(pitch);
	}

	dk_inline float3 GetForward() const noexcept
	{
		static const float3 worldForward = float3(0, 0, 1);

		Matrix3x3 rotationMatrix;
		//GetRoationMatrix(rotationMatrix);
		GetRoationMatrix(rotationMatrix);

		return worldForward * rotationMatrix;
	}

	dk_inline float3 GetRight() const noexcept
	{
		static const float3 worldRight = float3(1, 0, 0);

		Matrix3x3 rotationMatrix;
		GetRoationMatrix(rotationMatrix);

		return worldRight * rotationMatrix;
	}

	dk_inline void GetRoationMatrix(Matrix3x3& outMatrix) const noexcept
	{
#if 0
		float roll = _rotation.z;
		float yaw = _rotation.y;
		float pitch = _rotation.x;

		// √‚√≥ : https://en.wikipedia.org/wiki/Rotation_matrix
		outMatrix._11 = cos(yaw) * cos(pitch);
		outMatrix._12 = cos(yaw) * sin(pitch) * sin(roll) - sin(yaw) * cos(roll);
		outMatrix._13 = cos(yaw) * sin(pitch) * cos(roll) + sin(yaw) * sin(roll);
		outMatrix._21 = sin(yaw) * cos(pitch);
		outMatrix._22 = sin(yaw) * sin(pitch) * sin(roll) + cos(yaw) * cos(roll);
		outMatrix._23 = sin(yaw) * sin(pitch) * cos(roll) - cos(yaw) * sin(roll);
		outMatrix._31 = -sin(pitch);
		outMatrix._32 = cos(pitch) * sin(roll);
		outMatrix._33 = cos(pitch) * cos(roll);
#else
		Matrix3x3 yawMatrix;
		GetYawRotationMatrix(yawMatrix);
		Matrix3x3 rollMatrix;
		GetRollRotationMatrix(rollMatrix);
		Matrix3x3 pitchMatrix;
		GetPitchRotationMatrix(pitchMatrix);

		outMatrix = rollMatrix;
		outMatrix = outMatrix * pitchMatrix;
		outMatrix = outMatrix * yawMatrix;
#endif
	}

	dk_inline void MoveTransformNoScale(const Transform& offset) noexcept
	{
		_position += offset._position;
		_rotation += offset._rotation;
	}

	dk_inline void MovePosition(const float3& offset) noexcept
	{
		_position += offset;
	}

	dk_inline void Rotate(const float3& offset) noexcept
	{
		_rotation += offset;
	}

	dk_inline const float3& GetPosition() const noexcept
	{
		return _position;
	}

	dk_inline const float3& GetRotation() const noexcept
	{
		return _rotation;
	}

	dk_inline const float3& GetScale() const noexcept
	{
		return _scale;
	}

private:
	float3 _position;
	float3 _rotation;
	float3 _scale;
};