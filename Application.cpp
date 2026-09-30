#include "stdafx.h"
#include "Application.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "InputModule.h"
#include "Camera.h"

#if defined(USE_IMGUI)
#include "imgui_impl_win32.h"
// Lib
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

namespace DK
{
	// 시간을 구하기 위한 변수들
	LARGE_INTEGER g_tSecond;		// 초당 클록수    ex) 360  (고정값)
	LARGE_INTEGER g_tTime;			// 이전 클록수    
	float		  g_fDeltaTime;		// (현재클록수 - 이전클록수) / 초당 클록수 --> ex 36 / 360

	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
#if defined(USE_IMGUI)
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
			return true;
#endif

		switch (msg)
		{
			case WM_SIZE:
			{
				if (wParam != SIZE_MINIMIZED && DuckingEngine::getInstance().isInitialized())
				{
					const UINT width = LOWORD(lParam);
					const UINT height = HIWORD(lParam);
					if (DuckingEngine::getInstance().GetRenderModuleWritable().resize(width, height))
					{
						if (Camera::gMainCamera)
							Camera::gMainCamera->setViewportSize(static_cast<int>(width), static_cast<int>(height));
					}
				}
			}
			break;
			case WM_KILLFOCUS:
			{
				InputModule::setBlock(true);
			}
			break;
			case WM_SETFOCUS:
			{
				InputModule::setBlock(false);
			}
			break;
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
			{
				switch (wParam)
				{
				case VK_ESCAPE:	// ESC 누를 시 프로그램 종료
					if (MessageBox(0, L"Are you sure you want to exit?", L"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES)
					{
						DuckingEngine::getInstance().destroy();

						DestroyWindow(hwnd);
						exit(0);
					}
					return 0;
				}
			}
			break;
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;
			default:
				return ::DefWindowProcW(hwnd, msg, wParam, lParam);
		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	bool Application::initialize(const ApplicationInitializeData& data)
	{
		QueryPerformanceFrequency(&g_tSecond);
		QueryPerformanceCounter(&g_tTime);

		if (initializeWindow(data._hInstance, data._showWnd, data._width, data._height, data._fullScreen) == false)
		{
			DK_ASSERT_LOG(false, "Window Initialize - Failed");
			return false;
		}
		DK_LOG("Window Initialize - Success");

		UINT dpi = GetDpiForWindow(_hwnd);
		DK_LOG("Window DPI = %u", dpi);

		RECT clientRect;
		GetClientRect(_hwnd, &clientRect);

		const int clientWidth = clientRect.right - clientRect.left;
		const int clientHeight = clientRect.bottom - clientRect.top;
		if (DuckingEngine::getInstance().Initialize(_hwnd, clientWidth, clientHeight) == false)
		{
			DK_ASSERT_LOG(false, "Engine Initialize - Failed");
			return false;
		}
		DK_LOG("Engine Initialize - Success");

		return true;
	}
	bool Application::initializeWindow(HINSTANCE hInstance, int showWnd, int width, int height, bool fullScreen)
	{
		LPCTSTR windowName = L"DuckingEngine_v001";
		LPCTSTR windowTitle = L"DuckingEngine_v001";

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

		_hwnd = CreateWindowEx(
			NULL, windowName, windowTitle, WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, width, height,
			NULL, NULL, hInstance, NULL
		);

		if (_hwnd == nullptr)
		{
			MessageBox(NULL, L"Error Creating window", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		// 전체화면이면 창 스타일을 제거해야한다!
		if (fullScreen == true)
		{
			SetWindowLong(_hwnd, GWL_STYLE, 0);
		}

		ShowWindow(_hwnd, showWnd);
		UpdateWindow(_hwnd);

		return true;
	}

	void Application::run()
	{
		MSG msg;
		ZeroMemory(&msg, sizeof(MSG));

		LARGE_INTEGER tTime;
		QueryPerformanceCounter(&tTime);
		g_tTime = tTime;

		// #todo- running을 합칠 수 있을까?
		while (true)
		{
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
					return;

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			LARGE_INTEGER tTime;
			QueryPerformanceCounter(&tTime);
			const double elapsedSeconds = static_cast<double>(tTime.QuadPart - g_tTime.QuadPart)
				/ static_cast<double>(g_tSecond.QuadPart);
			g_tTime = tTime;

			// Limit simulation advancement after a stall (window dragging, dialogs, debugger).
			// Excess wall-clock time is intentionally discarded, not carried into later frames.
			constexpr double maxDeltaTime = 0.1;
			g_fDeltaTime = static_cast<float>(elapsedSeconds < 0.0 ? 0.0
				: (elapsedSeconds > maxDeltaTime ? maxDeltaTime : elapsedSeconds));

			updateFrame();
			renderFrame();

		}
	}

	void Application::updateFrame()
	{
		DuckingEngine::getInstance().Update(g_fDeltaTime);
	}

	void Application::renderFrame()
	{
		DuckingEngine::getInstance().Render(g_fDeltaTime);
	}
}
