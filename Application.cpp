#include "stdafx.h"
#include "Application.h"

#include "DuckingEngine.h"
#include "RenderModule.h"

Application::~Application()
{
	dk_delete _duckingEngine;
}

void Application::Update()
{
	_duckingEngine->Update();
	_duckingEngine->Render();
}

#pragma region WindowApplication
#if defined(USE_IMGUI)
#include "imgui_impl_win32.h"
// Lib
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#if defined(USE_IMGUI)
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return true;
#endif

	switch (msg)
	{
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_ESCAPE:
			if (MessageBox(0, L"Are you sure you want to exit?", L"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				DestroyWindow(hwnd);
			}
			return 0;
		}
	}
	case WM_DESTROY:
		//PostQuitMessage(0);
		return 0;
	default:
		return ::DefWindowProcW(hwnd, msg, wParam, lParam);
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

//bool WindowApplication::Initialize(HINSTANCE hInstance, int showWnd, int width, int height, bool fullScreen)
bool WindowApplication::Initialize(const ApplicationInitializeData& data)
{
	if (InitializeWindow(data._hInstance, data._showWnd, data._width, data._height, data._fullScreen) == false)
	{
		DK_ASSERT_LOG(false, "Window Initialize - Failed");
		return false;
	}
	DK_LOG("Window Initialize - Success");

	_duckingEngine = dk_new DuckingEngine;
	if (_duckingEngine->Initialize(_hwnd, data._width, data._height) == false)
	{
		DK_LOG("Engine Initialize - Failed");
		return false;
	}
	DK_LOG("Engine Initialize - Success");

	return true;
}

bool WindowApplication::InitializeWindow(HINSTANCE hInstance, int showWnd, int width, int height, bool fullScreen)
{
	LPCTSTR windowName = L"DuckingEngine_v001";
	LPCTSTR windowTitle = L"DuckingEngine_v001";

	// ������ ���� window�� ������ �����մϴ�.
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = windowName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	// wc�� ����մϴ�.
	if (false == RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Error registering class", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	if (fullScreen == true)
	{
		width = GetSystemMetrics(SM_CXSCREEN);
		height = GetSystemMetrics(SM_CYSCREEN);
	}

	// ����� wc�� �̿��Ͽ� â�� ����ϴ�.
	_hwnd = CreateWindowEx(
		NULL, windowName, windowTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		NULL, NULL, hInstance, NULL
	);

	// â�� ����� ��������� �ʾҴ��� üũ!
	if (!_hwnd)
	{
		MessageBox(NULL, L"Error Creating window", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	// ��üȭ���̸� â ��Ÿ���� �����ؾ��Ѵ�!
	if (fullScreen)
	{
		SetWindowLong(_hwnd, GWL_STYLE, 0);
	}

	// â�� ǥ���մϴ�.
	ShowWindow(_hwnd, showWnd);
	// â�� ������Ʈ�մϴ�.
	UpdateWindow(_hwnd);

	return true;
}

void WindowApplication::Run()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	// #todo- running�� ��ĥ �� ������?
	while (_running == true && _duckingEngine->GetRenderModule()->_isRunning == true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			//if (msg.message == WM_QUIT)
			//	break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			Update();
		}
	}
}
#pragma endregion