#pragma once

#include <Windows.h>

class DuckingEngine;
class WindowApplication;

struct ApplicationInitializeData
{
#if defined DK_WINDOW
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
#elif defined DK_XBOX
#define APPLICATION(application, hInstance, showWnd, width, height, fullScreen) \
...
#elif defined DK_PS
#define APPLICATION(application) \
...
#endif
};

class Application
{
public:
	Application() = default;
	virtual ~Application();

	virtual bool Initialize(const ApplicationInitializeData& data) = 0;
	virtual void Run() = 0;
	virtual HWND GetHWND() const noexcept = 0;

protected:
	void Update();

protected:
	bool _running = true;
	DuckingEngine* _duckingEngine;
};

class WindowApplication : public Application
{
public:
	WindowApplication() = default;
	~WindowApplication() = default;
	virtual bool Initialize(const ApplicationInitializeData& data) override final;

	virtual HWND GetHWND() const noexcept override final
	{
		return _hwnd;
	}

private:
	bool InitializeWindow(HINSTANCE hInstance, int showWnd, int width, int height, bool fullScreen);
	virtual void Run() override final;

private:
	HWND _hwnd;
};