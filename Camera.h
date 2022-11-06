#pragma once

#include "Object.h"

struct Transform;
class InputModule;

class Camera : public Object
{
public:
	static Camera* gMainCamera;

public:
	Camera(const float fov, const int width, const int height)
		: _halfFov((fov / 2.0f) * DK::Math::kToRadian)
		, _width(width)
		, _height(height)
	{}

	virtual void Update() override final;

	dk_inline const int GetWidth() const noexcept
	{
		return _width;
	}
	dk_inline const int GetHeight() const noexcept
	{
		return _height;
	}
	dk_inline const float GetNearPlaneDistance() const noexcept
	{
		return _nearPlaneDistance;
	}
	dk_inline const float GetFarPlaneDistance() const noexcept
	{
		return _farPlaneDistance;
	}
	dk_inline const Transform& GetWorldTransform() const noexcept
	{
		return _worldTransform;
	}
	dk_inline Transform& GetWorldTransformWritable() noexcept
	{
		return _worldTransform;
	}
	void GetCameraWorldMatrix(Matrix4x4& outMatrix) noexcept;

	dk_inline void MoveForward(const float distance)
	{
		float3 forward = _worldTransform.GetForward();
		_worldTransform.MovePosition(forward * distance);
	}
	dk_inline void MoveRight(const float distance)
	{
		float3 right = _worldTransform.GetRight();
		_worldTransform.MovePosition(right * distance);
	}

	dk_inline void MoveOffset(const Transform& offset)
	{
		_worldTransform.MoveTransformNoScale(offset);
	}

	void GetCameraProjectionMaterix(Matrix4x4& outMaterix);

private:
	float _halfFov = 45.0f * DK::Math::kToRadian;
	int _width = 1920;
	int _height = 1080;

	float _nearPlaneDistance = 0.01f;
	float _farPlaneDistance = 1000.0f;
};