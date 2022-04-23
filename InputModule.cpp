#include "stdafx.h"
#include "InputModule.h"

#include "ComputerController.h"
#ifdef DK_XBOX
#include "XBOXController.h"
#endif // DK_XBOX
#ifdef DK_PS
#include "PlayStationController.h"
#endif

ComputerController* InputModule::_computerController = nullptr;
#ifdef DK_XBOX
XBOXController* InputModule::_xboxController = nullptr;
#endif // DK_XBOX

InputModule::~InputModule()
{
	dk_delete _xboxController;
	dk_delete _computerController;
}

const bool InputModule::InitializePCController()
{
	_computerController = dk_new ComputerController;
	CHECK_BOOL_AND_RETURN(_computerController != nullptr);

	return true;
}

#ifdef DK_XBOX
const bool InputModule::InitializeXBOXController(int playerNumber)
{
	_xboxController = dk_new XBOXController(1);
	CHECK_BOOL_AND_RETURN(_xboxController != nullptr);

	return true;
}
#endif // DK_XBOX

#ifdef DK_PS
static const bool InputModule::InitializePlayStationController(int playerNumber)
{

}
#endif // DK_PS

void InputModule::Update()
{
#ifdef DK_XBOX
	_xboxController->IsConnected();	// isConnect가 Update를 대신합니다.
#endif // DK_XBOX

	_computerController->Update();
}

const bool InputModule::GetKeyDown(KeyboardState keyCode)
{
	return _computerController->GetKeyDown(keyCode);
}

#if defined(DK_XBOX) || defined(DK_PS)
const bool InputModule::GetXJoypadDown(const XboxState code) noexcept
{
	return _xboxController->GetXJoypadDown(code);
}

const float2& InputModule::GetJoystickL() noexcept
{
	return _xboxController->GetJoystickL();
}

const float2& InputModule::GetJoystickR() noexcept
{
	return _xboxController->GetJoystickR();
}
#endif // DK_XBOX | DK_PS