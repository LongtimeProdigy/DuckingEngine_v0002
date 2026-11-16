#pragma once

namespace DK
{
	class DuckingEngine;
	class WindowApplication;
}

namespace DK
{
	struct ApplicationInitializeData
	{
#if defined(_DK_WINDOW_)
		ApplicationInitializeData(HINSTANCE hInstance, int showWnd, int width, int height, bool fullScreen, const DKString& levelPath)
			: _hInstance(hInstance)
			, _showWnd(showWnd)
			, _width(width)
			, _height(height)
			, _fullScreen(fullScreen)
			, _levelPath(levelPath)
		{}

		HINSTANCE _hInstance;
		int _showWnd;
		int _width;
		int _height;
		bool _fullScreen;
		DKString _levelPath;
#endif
	};

	class Application
	{
	public:
		bool initialize(const ApplicationInitializeData& data);
		bool initializeWindow(HINSTANCE hInstance, int showWnd, int width, int height, bool fullScreen);
		void run();

	private:
		HWND _hwnd;
	};
}
