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
			static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_W)) - 
			static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_S));
		float moveRight = lJoystick.x - 
			static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_A)) + 
			static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_D));
		float moveUp = static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_E)) - 
			static_cast<float>(InputModule::GetKeyDown(KeyboardState::KEYBOARD_Q));

		float yaw = rJoystick.x + mouseDelta.x;
		float pitch = rJoystick.y + mouseDelta.y;

		yaw *= 0.2f;
		pitch *= 0.2f;

		// Translation
		float3 moveOffset(moveRight, moveUp, moveForward);
		moveOffset.normalize();
		moveOffset *= deltaTime;

		float3 rotatedMoveOffset = moveOffset * get_worldTransform().get_rotation();
		float3 finalMoveOffset = rotatedMoveOffset + get_worldTransform().get_translation();
		
		// Rotate
		const Quaternion& originRotation = get_worldTransform().get_rotation();
		float3 originEuler;
		originRotation.toEuler(originEuler);
		float3 rotateOffset(pitch, yaw, 0);
		float3 finalRotate = rotateOffset + originEuler;
		finalRotate.x = Math::clamp(finalRotate.x, -Math::Half_PI/2, Math::Half_PI/2);
		Quaternion finalQuaternion(finalRotate.z, finalRotate.x, finalRotate.y);

		Transform setTransform(finalMoveOffset, finalQuaternion, float3::Identity);

		set_worldTransform(setTransform);
	}
}