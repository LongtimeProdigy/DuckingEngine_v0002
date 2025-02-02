#pragma once

namespace DK
{
#define KEYBOARDSTATECOUNT 256

	// https://m.blog.naver.com/PostView.naver?isHttpsRedirect=true&blogId=power2845&logNo=50151514583
	enum class KeyboardState : uint8
	{
		MOUSE_LEFT = 0x01,			// VK_LBUTTON
		MOUSE_RIGHT,				// VK_RBUTTON
		KEYBOARD_CNALE,				// VK_CANCLE	// Ctrl+Break
		MOUSE_CENTER,				// VK_MBUTTON
		MOUSE_X1,					// VK_MBUTTON1
		MOUSE_X2,					// VK_MBUTTON2
		UNDEFINED0, 
		KEYBOARD_BACKSPACE,			// VK_BACK		// Backspace
		KEYBOARD_TAB,				// VK_TAB
		RESERVED0, RESERVED1,		// 예약됨?
		KEYBOARD_CLEAR,				// VK_CLEAR		// Clear
		KEYBOARD_ENTER,				// VK_RETURN	// Enter
		UNDEFINED1, UNDEFINED2, 
		KEYBOARD_SHIFT,				// VK_SHIFT
		KEYBOARD_CONTROL,			// VK_CONTROL
		KEYBOARD_ACL,				// VK_MENU
		KEYBOARD_PAUSE,				// VK_PAUSE
		KEYBOARD_CAPSLOCK,			// VK_CAPITAL
		IME_HANGUEL, 				// VK_KANA, VK_HANGUEL	// IME가나모드 / IME한글모드
		UNDEFINED3, 
		IME_JUNJA,					// VK_JUNJA		// IME 전자 모드
		IME_FINAL,					// VK_FINAL		// IME 최종 모드
		IME_HANJA,					// VK_HANJA, VK_KANJI		// IME 한자 모드
		UNDEFINED4, 
		KEYBOARD_ESC,				// VK_ESCAPE
		IME_CONVERT, 				// VK_CONVERT	// IME 변환
		IME_NONCONVERT, 			// VK_NONCONVERT	// IME 변환 안함
		IME_ACCEPT,					// VK_ACCEPT	// IME 승인
		IME_MODECHANGE, 			// VK_MODECHANGE	// IME 모드 변경 요청
		KEYBOARD_SPACE, 			// VK_SPACE
		KEYBOARD_PAGEUP, 
		KEYBOARD_PAGEDOWN,
		KEYBOARD_END,
		KEYBOARD_HOME,
		KEYBOARD_LEFT,
		KEYBOARD_UP,
		KEYBOARD_RIGHT,
		KEYBOARD_DOWN,
		KEYBOARD_SELECT,
		KEYBOARD_PRINT,
		KEYBOARD_EXECUTE,
		KEYBOARD_PRINTSCREEN,
		KEYBOARD_INSERT,
		KEYBOARD_DELETE,
		KEYBOARD_HELP, 
		KEYBOARD_0, 
		KEYBOARD_1, 
		KEYBOARD_2, 
		KEYBOARD_3, 
		KEYBOARD_4, 
		KEYBOARD_5, 
		KEYBOARD_6, 
		KEYBOARD_7, 
		KEYBOARD_8, 
		KEYBOARD_9, 
		UNDEFINED5, UNDEFINED6, UNDEFINED7, UNDEFINED8, UNDEFINED9, UNDEFINED10, UNDEFINED11, 
		KEYBOARD_A, 
		KEYBOARD_B, 
		KEYBOARD_C, 
		KEYBOARD_D, 
		KEYBOARD_E, 
		KEYBOARD_F, 
		KEYBOARD_G, 
		KEYBOARD_H, 
		KEYBOARD_I, 
		KEYBOARD_J, 
		KEYBOARD_K, 
		KEYBOARD_L, 
		KEYBOARD_M, 
		KEYBOARD_N, 
		KEYBOARD_O, 
		KEYBOARD_P, 
		KEYBOARD_Q, 
		KEYBOARD_R, 
		KEYBOARD_S, 
		KEYBOARD_T, 
		KEYBOARD_U, 
		KEYBOARD_V, 
		KEYBOARD_W, 
		KEYBOARD_X, 
		KEYBOARD_Y, 
		KEYBOARD_Z, 
		KEYBOARD_LEFTWINDOW, 
		KEYBOARD_RIGHTWINDOW, 
		RESERVED2, 
		KEYBOARD_SLEEP, 
		KEYBOARD_NUM0, 
		KEYBOARD_NUM1, 
		KEYBOARD_NUM2, 
		KEYBOARD_NUM3, 
		KEYBOARD_NUM4, 
		KEYBOARD_NUM5, 
		KEYBOARD_NUM6, 
		KEYBOARD_NUM7, 
		KEYBOARD_NUM8, 
		KEYBOARD_NUM9, 
		KEYBOARD_NUMSTAR, 
		KEYBOARD_NUMPLUS, 
		KEYBOARD_SEPARATOR, 
		KEYBOARD_NUMMINUS, 
		KEYBOARD_NUMDOT, 
		KEYBOARD_NUMDIVIDE, 
		KEYBOARD_F1, 
		KEYBOARD_F2, 
		KEYBOARD_F3, 
		KEYBOARD_F4, 
		KEYBOARD_F5, 
		KEYBOARD_F6, 
		KEYBOARD_F7, 
		KEYBOARD_F8, 
		KEYBOARD_F9, 
		KEYBOARD_F10, 
		KEYBOARD_F11, 
		KEYBOARD_F12, 
		KEYBOARD_F13, 
		KEYBOARD_F14, 
		KEYBOARD_F15, 
		KEYBOARD_F16, 
		KEYBOARD_F17, 
		KEYBOARD_F18, 
		KEYBOARD_F19, 
		KEYBOARD_F20, 
		KEYBOARD_F21, 
		KEYBOARD_F22, 
		KEYBOARD_F23, 
		KEYBOARD_F24, 
		UNDEFINED12, UNDEFINED13, UNDEFINED14, UNDEFINED15, UNDEFINED16, UNDEFINED17, UNDEFINED18, UNDEFINED19, 
		KEYBOARD_NUMLOCK, 
		KEYBOARD_SCROLLLOCK, 
		OEM0, OEM1, OEM2, OEM3, OEM4, 
		UNDEFINED20, UNDEFINED21, UNDEFINED22, UNDEFINED23, UNDEFINED24, UNDEFINED25, UNDEFINED26, UNDEFINED27, UNDEFINED28, 
		KEYBOARD_LEFTSHIFT, 
		KEYBOARD_RIGHTSHIFT, 
		KEYBOARD_LEFTCONTROL, 
		KEYBOARD_RIGHTCONTROL, 
		KEYBOARD_LEFTALT, 
		KEYBOARD_RIGHTALT = 165,
		COUNT = 166
	};

	class ComputerController
	{
	public:
		void initialize();

		void Update();
		dk_inline bool GetKeyDown(KeyboardState keyCode)
		{
			DK_ASSERT_LOG(
				static_cast<uint32>(keyCode) < static_cast<uint32>(KeyboardState::COUNT), 
				"keyCode의 범위가 이상합니다. KeyCode: %d / %d", 
				static_cast<uint32>(keyCode), static_cast<uint32>(KeyboardState::COUNT)
			);

			// 0은 한번도 안눌린 상태 또는 1에서 한번 누른 상태
			// 1은 한번 눌려진 상태 (다시 누르면 0이됨)
			// 128은 0이었던 상태에서 눌리고 있는 상태(떼면 1이됨)
			// 129는 1이었던 상태에서 눌리고 있는 상태(떼면 0이됨)
			return _keyStates[static_cast<uint32>(keyCode)] == 128 || _keyStates[static_cast<uint32>(keyCode)] == 129;
		}

		dk_inline const float2& getMouseDelta() const
		{
			return _mouseDelta;
		}

	private:
		float2 _mousePosition;
		float2 _mouseDelta;
		BYTE _keyStates[KEYBOARDSTATECOUNT] = {};
	};
}