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
		ApplicationInitializeData(HINSTANCE hInstance, int showWnd, int width, int height, bool fullScreen)
			: _hInstance(hInstance)
			, _showWnd(showWnd)
			, _width(width)
			, _height(height)
			, _fullScreen(fullScreen)
		{}

		HINSTANCE _hInstance;
		int _showWnd;
		int _width;
		int _height;
		bool _fullScreen;
#endif
	};

	class Application
	{
	public:
		bool initialize(const ApplicationInitializeData& data);
		bool initializeWindow(HINSTANCE hInstance, int showWnd, int width, int height, bool fullScreen);
		void run();

	private:
		void updateFrame();
		void renderFrame();

	private:
		HWND _hwnd;
	};
}