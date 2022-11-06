#pragma once

#pragma comment(lib, "XInput.lib") 
#include <Xinput.h>

#include "float2.h"
#include "XboxState.h"

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
		//Sleep(30);  // CPU 점유율 100% 방지
		DWORD Result = XInputGetState(_playerNumber, &_xInputState);

		_joyStickL = float2((float)_xInputState.Gamepad.sThumbLX / 32767, (float)_xInputState.Gamepad.sThumbLY / 32767);
		_joyStickR = float2((float)_xInputState.Gamepad.sThumbRX / 32767, (float)_xInputState.Gamepad.sThumbRY / 32767);

		return Result == ERROR_SUCCESS;
	}

	dk_inline const bool GetXJoypadDown(const XboxState code) const noexcept
	{
		return _xInputState.Gamepad.wButtons & static_cast<uint8>(code);
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
		Vibration.wLeftMotorSpeed = leftVal;
		Vibration.wRightMotorSpeed = rightVal;

		// Vibrate the controller
		XInputSetState(_playerNumber, &Vibration);
	}

private:
	int _playerNumber;
	XINPUT_STATE _xInputState;

	float2 _joyStickL;
	float2 _joyStickR;
};