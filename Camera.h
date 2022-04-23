#pragma once

#include "Object.h"

struct Transform;
class InputModule;
class IResource;

class Camera : public Object
{
public:
	static Camera* gMainCamera;
	static IResource* gCameraConstantBuffer;

public:
	Camera(const float fov, const int width, const int height)
		: _fov(fov)
		, _width(width)
		, _height(height)
	{}

	virtual ~Camera() override final 
	{}

	virtual void Update() override final;

	dk_inline const float GetFOV() const noexcept
	{
		return _fov;
	}
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
	float _fov = 45.0f;
	int _width = 1920;
	int _height = 1080;

	// #todo- 일단은 설정없이 1~1000을 고정값으로 사용합니다. 나중에 받을 수 있도록 변경?
	float _nearPlaneDistance = 1.0f;
	float _farPlaneDistance = 1000.0f;
};