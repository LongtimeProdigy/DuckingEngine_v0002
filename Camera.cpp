#include "stdafx.h"
#include "Camera.h"

#include "float2.h"
#include "Matrix4x4.h"

#include "InputModule.h"
#include "XboxState.h"
#include "ComputerController.h"

Camera* Camera::gMainCamera = nullptr;
IResource* Camera::gCameraConstantBuffer = nullptr;

void Camera::Update()
{
	float2 lJoystick = InputModule::GetJoystickL();
	float2 rJoystick = InputModule::GetJoystickR();

	static const float limit = 0.1f;
	if (abs(lJoystick.y) > limit)
		Camera::gMainCamera->MoveForward(lJoystick.y);

	if (abs(lJoystick.x) > limit)
		Camera::gMainCamera->MoveRight(lJoystick.x);

	if (abs(rJoystick.x) > limit || abs(rJoystick.y) > limit)
	{
		static const float friction = 0.01f;
		Camera::gMainCamera->GetWorldTransformWritable().Rotate(float3(rJoystick.y, -rJoystick.x, 0) * friction);
	}

	Transform offsetTransform = Transform::Identity;
	if (InputModule::GetXJoypadDown(XboxState::XBOX_DPAD_LEFT) || InputModule::GetKeyDown(KeyboardState::KEYBOARD_LEFT))
		offsetTransform.MovePosition(float3(-1, 0, 0));
	if (InputModule::GetXJoypadDown(XboxState::XBOX_DPAD_RIGHT) || InputModule::GetKeyDown(KeyboardState::KEYBOARD_RIGHT))
		offsetTransform.MovePosition(float3(1, 0, 0));
	if (InputModule::GetKeyDown(KeyboardState::KEYBOARD_UP))
		offsetTransform.MovePosition(float3(0, 0, 1));
	if (InputModule::GetKeyDown(KeyboardState::KEYBOARD_DOWN))
		offsetTransform.MovePosition(float3(0, 0, -1));
	if (InputModule::GetXJoypadDown(XboxState::XBOX_DPAD_UP) || InputModule::GetKeyDown(KeyboardState::KEYBOARD_PAGEUP))
		offsetTransform.MovePosition(float3(0, 1, 0));
	if (InputModule::GetXJoypadDown(XboxState::XBOX_DPAD_DOWN) || InputModule::GetKeyDown(KeyboardState::KEYBOARD_PAGEDOWN))
		offsetTransform.MovePosition(float3(0, -1, 0));
	Camera::gMainCamera->MoveOffset(offsetTransform);
}

void Camera::GetCameraWorldMatrix(Matrix4x4& outMatrix) noexcept
{
	Transform tempPositionTransform(
		_worldTransform.GetPosition() * -1,
		float3(0, 0, 0),
		float3(1, 1, 1)
	);

	Transform tempRotationTransform(
		float3(0, 0, 0),
		_worldTransform.GetRotation() * -1,
		float3(1, 1, 1)
	);

	Transform tempScaleTransform(
		float3(0, 0, 0),
		float3(0, 0, 0),
		_worldTransform.GetScale()
	);

	Matrix4x4 tempPositionMatrix;
	Matrix4x4 tempRotationMatrix;
	Matrix4x4 tempScaleMatrix;

	tempPositionTransform.ToMatrix4x4(tempPositionMatrix);
	tempRotationTransform.ToMatrix4x4(tempRotationMatrix);
	tempScaleTransform.ToMatrix4x4(tempScaleMatrix);

	outMatrix = tempScaleMatrix * tempPositionMatrix * tempRotationMatrix;
}

void Camera::GetCameraProjectionMaterix(Matrix4x4& outMaterix)
{
	float h = 1 / tanf(_fov);
	float w = h / (_width / _height);
	float a = _farPlaneDistance / (_farPlaneDistance - _nearPlaneDistance);
	float b = -_nearPlaneDistance * a;

	outMaterix._11 = w;
	outMaterix._22 = h;
	outMaterix._33 = a;
	outMaterix._43 = b;

	outMaterix._34 = 1.0f;
	outMaterix._44 = 0.0f;
}