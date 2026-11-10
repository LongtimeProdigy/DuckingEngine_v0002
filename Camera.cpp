#include "stdafx.h"
#include "Camera.h"

#include "InputModule.h"
#include "XboxState.h"
#include "ComputerController.h"
#include "DuckingEngine.h"
#include "RaytracingRenderer.h"
#include "FileUtil.h"

namespace DK
{
	Camera* Camera::gMainCamera = nullptr;

	static DKString cameraStatePath()
	{
		return FileUtil::fromWidePath(GlobalPath::makeResourceFullPathW(L"editor/data/camera-state.txt").c_str());
	}

	bool Camera::loadViewState()
	{
		const DKString path = cameraStatePath();
		if (FileUtil::exist(path) == false)
		{
			return false;
		}
		FileReader reader;
		if (reader.open(path) == false)
		{
			return false;
		}
		DKString text;
		if (reader.readText(text) == false)
		{
			return false;
		}
		const DKVector<DKString> tokens = StringUtil::splitWhitespace(text);
		if (tokens.size() != 7 || tokens[0] != "DuckingCamera")
		{
			DK_ASSERT_LOG(false, "Invalid camera state layout");
			return false;
		}
		int version = 0;
		float3 position;
		float yaw = 0.0f, pitch = 0.0f;
		if (StringUtil::parseInt(tokens[1], version) == false || StringUtil::parseFloat(tokens[2], position.x) == false || StringUtil::parseFloat(tokens[3], position.y) == false || StringUtil::parseFloat(tokens[4], position.z) == false || StringUtil::parseFloat(tokens[5], yaw) == false || StringUtil::parseFloat(tokens[6], pitch) == false)
		{
			return false;
		}
		if (version != 1 || Math::abs(pitch) > Math::Half_PI)
		{
			DK_ASSERT_LOG(false, "Invalid camera state");
			return false;
		}
		_yaw = yaw;
		_pitch = pitch;
		set_worldTransform(Transform(position, Quaternion(0.0f, _pitch, _yaw), float3::Identity));
		return true;
	}

	bool Camera::saveViewState()
	{
		const DKString path = cameraStatePath();
		if (FileUtil::createDirectory(FileUtil::parentPath(path)) == false)
		{
			return false;
		}
		const float3& position = get_worldTransform().get_translation();
		if (std::isfinite(position.x) == false || std::isfinite(position.y) == false || std::isfinite(position.z) == false || std::isfinite(_yaw) == false || std::isfinite(_pitch) == false || Math::abs(_pitch) > Math::Half_PI)
		{
			DK_ASSERT_LOG(false, "Invalid camera state");
			return false;
		}
		const DKString text = "DuckingCamera 1\n" + StringUtil::toString(position.x) + " " + StringUtil::toString(position.y) + " " + StringUtil::toString(position.z) + "\n" + StringUtil::toString(_yaw) + " " + StringUtil::toString(_pitch) + "\n";
		const DKString temporary = path + ".tmp";
		FileWriter writer;
		if (writer.open(temporary) == false)
		{
			return false;
		}
		if (writer.write(text.data(), text.size()) == false)
		{
			return false;
		}
		if (writer.close() == false)
		{
			return false;
		}
		return FileUtil::rename(temporary, path, true);
	}

	void Camera::update(float deltaTime)
	{
		// Detached tool windows must not keep moving the scene with stale input.
		if (InputModule::isBlocked())
			return;
#if defined(USE_IMGUI)
		if (ImGui::GetCurrentContext() && (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantCaptureMouse))
			return;
#endif
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

		if (mouseDelta == float2::Zero && keyboardYaw == 0.f && keyboardPitch == 0.f && lJoystick == float2::Zero && lJoystick == float2::Zero
			&& moveForward == 0.f && moveRight == 0.f && moveUp == 0.f)
			return;

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
		moveOffset *= deltaTime * _moveSpeed;

		if (InputModule::GetKeyDown(KeyboardState::KEYBOARD_CAPSLOCK) == true)
			moveOffset *= 10;
		if (InputModule::GetKeyDown(KeyboardState::KEYBOARD_SHIFT) == true)
			moveOffset *= 30;

		float3 rotatedMoveOffset = moveOffset * finalQuaternion;
		float3 finalMoveOffset = rotatedMoveOffset + get_worldTransform().get_translation();

		Transform setTransform(finalMoveOffset, finalQuaternion, float3::Identity);

		set_worldTransform(setTransform);
		DuckingEngine::getInstance().GetRaytracingRendererWritable().resetAccumulation();
	}
}
