// 반드시 XBOXPAD와 매칭시킬 것!!!!!!!!!
//XINPUT_GAMEPAD_DPAD_UP		0x0001
//XINPUT_GAMEPAD_DPAD_DOWN		0x0002
//XINPUT_GAMEPAD_DPAD_LEFT		0x0004
//XINPUT_GAMEPAD_DPAD_RIGHT		0x0008
//XINPUT_GAMEPAD_START			0x0010
//XINPUT_GAMEPAD_BACK			0x0020
//XINPUT_GAMEPAD_LEFT_THUMB		0x0040
//XINPUT_GAMEPAD_RIGHT_THUMB	0x0080
//XINPUT_GAMEPAD_LEFT_SHOULDER	0x0100
//XINPUT_GAMEPAD_RIGHT_SHOULDER	0x0200
//XINPUT_GAMEPAD_A				0x1000
//XINPUT_GAMEPAD_B				0x2000
//XINPUT_GAMEPAD_X				0x4000
//XINPUT_GAMEPAD_Y				0x8000
enum class XboxState : uint32
{
	XBOX_DPAD_UP = 0x0001, 
	XBOX_DPAD_DOWN = 0x0002, 
	XBOX_DPAD_LEFT = 0x0004, 
	XBOX_DPAD_RIGHT = 0x0008, 
	XBOX_START = 0x0010, 
	XBOX_BACK = 0x0020, 
	XBOX_LEFT_THUMB = 0x0040, 
	XBOX_RIGHT_THUMB = 0x0080, 
	XBOX_LEFT_SHOULDER = 0x0100, 
	XBOX_RIGHT_SHOULDER = 0x0200, 
	XBOX_A = 0x1000, 
	XBOX_B = 0x2000, 
	XBOX_X = 0x4000, 
	XBOX_Y = 0x8000, 
	COUNT = 0x0000
};