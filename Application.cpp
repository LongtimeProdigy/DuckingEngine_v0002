#include "stdafx.h"
#include "Application.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "InputModule.h"

#if defined(USE_IMGUI)
#include "imgui_impl_win32.h"
// Lib
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

namespace DK
{
	// 시간을 구하기 위한 변수들
	LARGE_INTEGER g_tSecond;   // 초당 클록수    ex) 360  (고정값)
	LARGE_INTEGER g_tTime;      // 이전 클록수    
	float		  g_fDeltaTime;   // (현재클록수 - 이전클록수) / 초당 클록수 --> ex 36 / 360

	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
#if defined(USE_IMGUI)
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
			return true;
#endif

		switch (msg)
		{
			//case WM_SIZE:
			//{
			//	DK_ASSERT_LOG(false, "현재 resize 기능을 지원하지 않습니다.");
			//}
			//break;
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
						DestroyWindow(hwnd);
						exit(0);
					}
					return 0;
				}
			}
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
		QueryPerformanceFrequency(&g_tSecond);    // 초당 클록수 가져오기
		QueryPerformanceCounter(&g_tTime);  // 이전 클록수 가져오기

		if (initializeWindow(data._hInstance, data._showWnd, data._width, data._height, data._fullScreen) == false)
		{
			DK_ASSERT_LOG(false, "Window Initialize - Failed");
			return false;
		}
		DK_LOG("Window Initialize - Success");

		if (DuckingEngine::getInstance().Initialize(_hwnd, data._width, data._height) == false)
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

		// 앞으로 만들 window의 정보를 저장합니다.
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

		// wc를 등록합니다.
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

		// 등록한 wc를 이용하여 창을 만듭니다.
		_hwnd = CreateWindowEx(
			NULL, windowName, windowTitle, WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, width, height,
			NULL, NULL, hInstance, NULL
		);

		// 창이 제대로 만들어지지 않았는지 체크!
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

		// 창을 표시합니다.
		ShowWindow(_hwnd, showWnd);
		// 창을 업데이트합니다.
		UpdateWindow(_hwnd);

		return true;
	}

	void Application::run()
	{
		MSG msg;
		ZeroMemory(&msg, sizeof(MSG));

		// #todo- running을 합칠 수 있을까?
		while (true)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				//if (msg.message == WM_QUIT)
				//	break;

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else {
				updateFrame();
				renderFrame();
			}
		}
	}

	void Application::updateFrame()
	{
		// DeltaTime을 구해준다.
		LARGE_INTEGER tTime;
		QueryPerformanceCounter(&tTime);    // 현재 클록수 가져오기

		// 호출1번당 이동시간 =	현재와 이전 클록수의 차이  /  1초당 클록수  ex)   36/360  
		g_fDeltaTime = (tTime.QuadPart - g_tTime.QuadPart) / (float)g_tSecond.QuadPart;

		DuckingEngine::getInstance().Update(g_fDeltaTime);

		// 이전시간을 지금시간으로 초기화해준다.
		g_tTime = tTime;
	}

	void Application::renderFrame()
	{
		DuckingEngine::getInstance().Render(g_fDeltaTime);
	}
}