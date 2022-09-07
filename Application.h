#pragma once

#include <Windows.h>

class DuckingEngine;
class WindowApplication;

struct ApplicationInitializeData
{
#if defined(DK_WINDOW)
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
	bool Initialize(const ApplicationInitializeData& data);
	bool InitializeWindow(HINSTANCE hInstance, int showWnd, int width, int height, bool fullScreen);
	void Run();

private:
	void updateFrame();
	void renderFrame();

private:
	HWND _hwnd;
};