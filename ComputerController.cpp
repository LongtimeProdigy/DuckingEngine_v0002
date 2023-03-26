#include "stdafx.h"
#include "ComputerController.h"

namespace DK
{
	//DK_ASSERT_LOG(static_cast<uint32>(KeyboardState::COUNT) < KEYBOARDSTATECOUNT, "KeyboardState개수가 올바르지 않습니다. %d / %d", static_cast<uint32>(KeyboardState::COUNT), KEYBOARDSTATECOUNT);
	static_assert(static_cast<uint32>(KeyboardState::COUNT) < KEYBOARDSTATECOUNT, "KeyboardState개수가 올바르지 않습니다.");

	void ComputerController::initialize()
	{
		POINT mousePos = {};
		if (::GetCursorPos(&mousePos))
			_mousePosition = float2(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
	}

	void ComputerController::Update()
	{
		ZeroMemory(&_mouseDelta, sizeof(float2));
		POINT mousePos = {};
		if (::GetCursorPos(&mousePos))
		{
			if (mousePos.x != _mousePosition.x || mousePos.y != _mousePosition.y)
			{
				// onMouseMove
				float2 prevMousePos = _mousePosition;
				_mousePosition = float2(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
				_mouseDelta = _mousePosition - prevMousePos;
			}
		}

		ZeroMemory(_keyStates, sizeof(BYTE) * KEYBOARDSTATECOUNT);
		if (::GetKeyboardState(_keyStates) == TRUE)
		{
			// Down, Up을 할려면.. _oldKeyState가 필요함
		}
		else
		{
			DWORD error = GetLastError();
			DK_ASSERT_LOG(false, "KeyboardController_KeyCode를 가져오는데 실패했습니다. ErrorCode: %d", error);
		}
	}
}