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
		const float2 mouseDelta = (InputModule::getMouseDelta() * InputModule::GetKeyDown(KeyboardState::MOUSE_RIGHT)) * mouseRotationFriction;

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

		float yaw = mouseDelta.x; // + rJoystick.x
		float pitch = mouseDelta.y; //+ rJoystick.y 

		yaw *= 0.2f;
		pitch *= 0.2f;

		// Translation
		float3 moveOffset(moveRight, moveUp, moveForward);
		moveOffset.normalize();
		moveOffset *= deltaTime;

		if (InputModule::GetKeyDown(KeyboardState::KEYBOARD_CAPSLOCK) == true)
			moveOffset *= 10;
		if (InputModule::GetKeyDown(KeyboardState::KEYBOARD_SHIFT) == true)
			moveOffset *= 10;

		float3 rotatedMoveOffset = moveOffset * get_worldTransform().get_rotation();
		float3 finalMoveOffset = rotatedMoveOffset + get_worldTransform().get_translation();

#if 0
		static float test = 0.0f;
		test += deltaTime;
		float x = Math::sin(test*15) * 150;
		float xr = (Math::sin(test * 10)) * 15;
		finalMoveOffset = float3(0, 300, -300);// +float3(x, 0, 0);
		pitch = 45;//(45 + xr) * DK::Math::kToRadian;
		static float tttt = 0.0f;
		tttt += deltaTime * 4;
		yaw = tttt;
#endif
		
#if 1
		// Rotate
		const Quaternion& originRotation = get_worldTransform().get_rotation();
		float3 originEuler;
		originRotation.toEuler(originEuler);
		float3 rotateOffset(pitch, yaw, 0);
		float3 finalRotate = rotateOffset + originEuler;
		finalRotate.x = Math::clamp(finalRotate.x, -Math::Half_PI - 0.0001f, Math::Half_PI - 0.0001f);
		Quaternion finalQuaternion(finalRotate.z, finalRotate.x, finalRotate.y);
#else
		Quaternion finalQuaternion(0, pitch, yaw);
#endif

		Transform setTransform(finalMoveOffset, finalQuaternion, float3::Identity);

		set_worldTransform(setTransform);
	}
}