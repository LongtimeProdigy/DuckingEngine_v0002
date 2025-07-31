#pragma once

namespace DK
{
	class ComputerController;
	enum class KeyboardState : uint8;
	enum class XboxState : uint32;
	class XBOXController;
	struct float2;
}

namespace DK
{
	class InputModule
	{
	public:
		dk_inline static void setBlock(const bool value) { kBlock = value; }
		static void Update();

		// PC는 기본 지원입니다.
		static const bool InitializePCController();
		static const bool GetKeyDown(KeyboardState keyCode);
		static const float2& getMouseDelta();

		// XBOX 패드 지원
		static const bool InitializeXBOXController(int playerNumber);
		static const bool GetXJoypadDown(const XboxState code) noexcept;
		static const float2& GetJoystickL() noexcept;
		static const float2& GetJoystickR() noexcept;

	private:
		InputModule() {};

	private:
		static bool kBlock;
		static ComputerController* gComputerController;
		static XBOXController* gXBOXController;
	};
}
