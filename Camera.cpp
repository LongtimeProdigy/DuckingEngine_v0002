#include "stdafx.h"
#include "Camera.h"

#include "InputModule.h"
#include "XboxState.h"
#include "ComputerController.h"
#include "DuckingEngine.h"
#include "RaytracingRenderer.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>

namespace DK
{
	Camera* Camera::gMainCamera = nullptr;

	namespace
	{
		const std::filesystem::path& cameraStatePath()
		{
			// Independent of Visual Studio's working directory.
			static const std::filesystem::path path = [] {
				wchar_t executable[32768];
				const DWORD length = GetModuleFileNameW(nullptr, executable, 32768);
				if (length == 0 || length >= 32768)
					return std::filesystem::path();

				return std::filesystem::path(executable).parent_path() / L"camera-state.txt";
			}();
			return path;
		}
	}

	bool Camera::loadViewState()
	{
		std::ifstream input(cameraStatePath());
		input.imbue(std::locale::classic());

		std::string signature;
		int version = 0;
		float3 position;
		float yaw = 0.0f, pitch = 0.0f;
		if (!(input >> signature >> version >> position.x >> position.y >> position.z >> yaw >> pitch))
			return false;
		if (signature != "DuckingCamera" || version != 1 || !std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z) || !std::isfinite(yaw) || !std::isfinite(pitch) || std::abs(pitch) > Math::Half_PI)
			return false;

		_yaw = yaw;
		_pitch = pitch;
		set_worldTransform(Transform(position, Quaternion(0.0f, _pitch, _yaw), float3::Identity));

		return true;
	}

	void Camera::saveViewState()
	{
		if (cameraStatePath().empty() /*|| !std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z) || !std::isfinite(_yaw) || !std::isfinite(_pitch)*/)
			return;

		const float3& position = get_worldTransform().get_translation();

		// Replace only after a complete write, keeping the previous snapshot on failure.
		const std::filesystem::path temporary = cameraStatePath().wstring() + L".tmp";
		std::ofstream output(temporary, std::ios::trunc);
		output.imbue(std::locale::classic());
		output << std::setprecision(std::numeric_limits<float>::max_digits10) << "DuckingCamera 1\n" << position.x << ' ' << position.y << ' ' << position.z << '\n' << _yaw << ' ' << _pitch << '\n';
		output.close();

		if (!output || MoveFileExW(temporary.c_str(), cameraStatePath().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == false)
		{
			DK_LOG("Camera state save failed.");
			return;
		}
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
