#include "stdafx.h"
#include "InputModule.h"

#include "ComputerController.h"
#include "XBOXController.h"

ComputerController* InputModule::gComputerController = nullptr;
XBOXController* InputModule::gXBOXController = nullptr;

const bool InputModule::InitializePCController()
{
	gComputerController = dk_new ComputerController;
	if (gComputerController == nullptr)
	{
		return false;
	}

	return true;
}

const bool InputModule::InitializeXBOXController(int playerNumber)
{
	gXBOXController = dk_new XBOXController(0);
	if (gXBOXController == nullptr)
	{
		return false;
	}

	return true;
}

void InputModule::Update()
{
	gXBOXController->IsConnected();	// isConnect가 Update를 대신합니다.
	gComputerController->Update();
}

const bool InputModule::GetKeyDown(KeyboardState keyCode)
{
	return gComputerController->GetKeyDown(keyCode);
}

const bool InputModule::GetXJoypadDown(const XboxState code) noexcept
{
	return gXBOXController->GetXJoypadDown(code);
}

const float2& InputModule::GetJoystickL() noexcept
{
	return gXBOXController->GetJoystickL();
}

const float2& InputModule::GetJoystickR() noexcept
{
	return gXBOXController->GetJoystickR();
}