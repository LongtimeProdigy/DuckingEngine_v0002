#pragma once

class ComputerController;
enum class KeyboardState : uint8;
#ifdef DK_XBOX
enum class XboxState : uint;
class XBOXController;
#endif // DK_XBOX

struct float2;

class InputModule
{
public:
	InputModule() = delete;
	~InputModule();

	static void Update();

	// PC는 기본 지원입니다.
#pragma region PC
	static const bool InitializePCController();
	static const bool GetKeyDown(KeyboardState keyCode);
#pragma endregion

#if defined(DK_XBOX) || defined(DK_PS)
#if defined DK_XBOX
	static const bool InitializeXBOXController(int playerNumber);
#elif defined DK_PS
	static const bool InitializePlayStationController(int playerNumber);
#endif // DK_XBOX
	// 아래 함수들은 PS, XBOX PAD 모두를 지원하는 함수입니다.
	static const bool GetXJoypadDown(const XboxState code) noexcept;
	static const float2& GetJoystickL() noexcept;
	static const float2& GetJoystickR() noexcept;
#endif // DK_XBOX

private:
	static ComputerController* _computerController;
#ifdef DK_XBOX
	static XBOXController* _xboxController;
#endif // DK_XBOX
};