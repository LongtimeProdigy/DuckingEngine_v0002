#pragma once

#pragma comment(lib, "XInput.lib") 
#include <Xinput.h>
#include <limits>

#include "XboxState.h"

namespace DK
{
	class XBOXController
	{
	public:
		XBOXController(int playerNumber)
			: _playerNumber(playerNumber)
			, _xInputState()
		{}

	public:
		dk_inline const bool IsConnected() noexcept
		{
			ZeroMemory(&_xInputState, sizeof(XINPUT_STATE));
			DWORD Result = XInputGetState(_playerNumber, &_xInputState);

			_joyStickL = float2(normalizeThumb(_xInputState.Gamepad.sThumbLX), normalizeThumb(_xInputState.Gamepad.sThumbLY));
			_joyStickR = float2(normalizeThumb(_xInputState.Gamepad.sThumbRX), normalizeThumb(_xInputState.Gamepad.sThumbRY));

			return Result == ERROR_SUCCESS;
		}

		dk_inline const bool GetXJoypadDown(const XboxState code) const noexcept
		{
			return (_xInputState.Gamepad.wButtons & static_cast<WORD>(code)) != 0;
		}

		dk_inline const float2& GetJoystickL() const noexcept
		{
			return _joyStickL;
		}

		dk_inline const float2& GetJoystickR() const noexcept
		{
			return _joyStickR;
		}

		dk_inline void Vibrate(int leftVal, int rightVal) const noexcept
		{
			// Create a Vibraton State
			XINPUT_VIBRATION Vibration;

			// Zeroise the Vibration
			ZeroMemory(&Vibration, sizeof(XINPUT_VIBRATION));

			// Set the Vibration Values
			Vibration.wLeftMotorSpeed = static_cast<WORD>(leftVal);
			Vibration.wRightMotorSpeed = static_cast<WORD>(rightVal);

			// Vibrate the controller
			XInputSetState(_playerNumber, &Vibration);
		}

	private:
		static dk_inline float normalizeThumb(SHORT value) noexcept
		{
			// XInput의 SHORT 범위는 음수와 양수의 절댓값이 달라 부호별 끝값으로 정규화한다.
			const float range = value < 0 ? -static_cast<float>(std::numeric_limits<SHORT>::lowest()) : static_cast<float>(std::numeric_limits<SHORT>::max());
			return static_cast<float>(value) / range;
		}

		int _playerNumber;
		XINPUT_STATE _xInputState;

		float2 _joyStickL;
		float2 _joyStickR;
	};
}
