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

	// PC�� �⺻ �����Դϴ�.
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
	// �Ʒ� �Լ����� PS, XBOX PAD ��θ� �����ϴ� �Լ��Դϴ�.
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