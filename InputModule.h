#pragma once

namespace DK
{
	class ComputerController;
	enum class KeyboardState : uint8;
	enum class XboxState : uint;
	class XBOXController;
	struct float2;
}

namespace DK
{
	class InputModule
	{
	public:
		static void Update();

		// PC�� �⺻ �����Դϴ�.
		static const bool InitializePCController();
		static const bool GetKeyDown(KeyboardState keyCode);
		static const float2& getMouseDelta();

		// XBOX �е� ����
		static const bool InitializeXBOXController(int playerNumber);
		static const bool GetXJoypadDown(const XboxState code) noexcept;
		static const float2& GetJoystickL() noexcept;
		static const float2& GetJoystickR() noexcept;

	private:
		static ComputerController* gComputerController;
		static XBOXController* gXBOXController;
	};
}