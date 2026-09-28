#include "stdafx.h"
#include "Camera.h"

#include "InputModule.h"
#include "XboxState.h"
#include "ComputerController.h"

namespace DK
{
	Camera* Camera::gMainCamera = nullptr;

	void Camera::update(float deltaTime)
	{
		static constexpr float mouseRotationFriction = 0.05f;
		float2 mouseDelta = (InputModule::getMouseDelta() * InputModule::GetKeyDown(KeyboardState::MOUSE_RIGHT)) * mouseRotationFriction;

		// 노트북으로 작업 시에 패드로 카메라 회전이 힘들어서 임시로 추가
		// Angular speeds in radians/second, preserving the previous feel at 60 FPS.
		static constexpr float keyboardRotationSpeed = 0.07f * 0.2f * 60.0f;
		static constexpr float joystickRotationSpeed = 0.2f * 60.0f;
		const float keyboardYaw = static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_L))
			- static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_J));
		const float keyboardPitch = static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_K))
			- static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_I));

		const float2& lJoystick = InputModule::GetJoystickL();
		const float2& rJoystick = InputModule::GetJoystickR();
		float moveForward = 
			//lJoystick.y + 
			static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_W)) - 
			static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_S));
		float moveRight =
			//lJoystick.x - 
			static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_D)) - 
			static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_A));
		float moveUp = static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_E)) - 
			static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_Q));

		// Mouse delta already represents displacement since the previous input update.
		_yaw += mouseDelta.x * 0.2f
			+ (keyboardYaw * keyboardRotationSpeed + rJoystick.x * joystickRotationSpeed) * deltaTime;
		_pitch += mouseDelta.y * 0.2f
			+ (keyboardPitch * keyboardRotationSpeed + rJoystick.y * joystickRotationSpeed) * deltaTime;
		_pitch = Math::clamp(_pitch, -Math::Half_PI + 0.0001f, Math::Half_PI - 0.0001f);
		
		// Rotate
		Quaternion finalQuaternion(0.0f, _pitch, _yaw);

		// Translation
		float3 moveOffset(moveRight, moveUp, moveForward);
		moveOffset.normalize();
		moveOffset *= deltaTime;

		if (InputModule::GetKeyDown(KeyboardState::KEYBOARD_CAPSLOCK) == true)
			moveOffset *= 10;
		if (InputModule::GetKeyDown(KeyboardState::KEYBOARD_SHIFT) == true)
			moveOffset *= 30;

		float3 rotatedMoveOffset = moveOffset * finalQuaternion;
		float3 finalMoveOffset = rotatedMoveOffset + get_worldTransform().get_translation();

		Transform setTransform(finalMoveOffset, finalQuaternion, float3::Identity);

		set_worldTransform(setTransform);
	}
}
