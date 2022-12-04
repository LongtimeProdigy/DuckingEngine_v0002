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
		const float2 mouseDelta = (InputModule::getMouseDelta() * InputModule::GetKeyDown(KeyboardState::MOUSE_LEFT)) * mouseRotationFriction;

		const float2& lJoystick = InputModule::GetJoystickL();
		const float2& rJoystick = InputModule::GetJoystickR();
		float moveForward = lJoystick.y + 
			static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_UP)) - 
			static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_DOWN));
		float moveRight = lJoystick.x - 
			static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_LEFT)) + 
			static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_RIGHT));
		float moveUp = static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_PAGEUP)) - 
			static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_PAGEDOWN));

		float yaw = rJoystick.x + mouseDelta.x;
		float pitch = rJoystick.y + mouseDelta.y;

		float3 moveOffset(moveRight, moveUp, moveForward);
		moveOffset *= deltaTime;
		Quaternion rotateOffset(0, 0, yaw);
		Transform offsetTransform(moveOffset, rotateOffset, float3::Identity);
		set_worldTransform(offsetTransform * get_worldTransform());
	}
}