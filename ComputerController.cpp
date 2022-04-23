#include "stdafx.h"
#include "ComputerController.h"

#include "float2.h"

//DK_ASSERT_LOG(static_cast<uint>(KeyboardState::COUNT) < KEYBOARDSTATECOUNT, "KeyboardState������ �ùٸ��� �ʽ��ϴ�. %d / %d", static_cast<uint>(KeyboardState::COUNT), KEYBOARDSTATECOUNT);
static_assert(static_cast<uint>(KeyboardState::COUNT) < KEYBOARDSTATECOUNT, "KeyboardState������ �ùٸ��� �ʽ��ϴ�.");

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
	else
	{

	}

	ZeroMemory(_keyStates, sizeof(BYTE) * KEYBOARDSTATECOUNT);
	if (::GetKeyboardState(_keyStates) == TRUE)
	{
		// Down, Up�� �ҷ���.. _oldKeyState�� �ʿ���
	}
	else
	{
		DWORD error = GetLastError();
		DK_ASSERT_LOG(false, "KeyboardController_KeyCode�� �������µ� �����߽��ϴ�. ErrorCode: %d", error);
	}
}