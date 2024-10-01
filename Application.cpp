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
	// �ð��� ���ϱ� ���� ������
	LARGE_INTEGER g_tSecond;   // �ʴ� Ŭ�ϼ�    ex) 360  (������)
	LARGE_INTEGER g_tTime;      // ���� Ŭ�ϼ�    
	float		  g_fDeltaTime;   // (����Ŭ�ϼ� - ����Ŭ�ϼ�) / �ʴ� Ŭ�ϼ� --> ex 36 / 360

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
			//	DK_ASSERT_LOG(false, "���� resize ����� �������� �ʽ��ϴ�.");
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
				case VK_ESCAPE:	// ESC ���� �� ���α׷� ����
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
		QueryPerformanceFrequency(&g_tSecond);    // �ʴ� Ŭ�ϼ� ��������
		QueryPerformanceCounter(&g_tTime);  // ���� Ŭ�ϼ� ��������

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
		if (_hwnd == nullptr)
		{
			MessageBox(NULL, L"Error Creating window", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		// ��üȭ���̸� â ��Ÿ���� �����ؾ��Ѵ�!
		if (fullScreen == true)
		{
			SetWindowLong(_hwnd, GWL_STYLE, 0);
		}

		// â�� ǥ���մϴ�.
		ShowWindow(_hwnd, showWnd);
		// â�� ������Ʈ�մϴ�.
		UpdateWindow(_hwnd);

		return true;
	}

	void Application::run()
	{
		MSG msg;
		ZeroMemory(&msg, sizeof(MSG));

		// #todo- running�� ��ĥ �� ������?
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
		// DeltaTime�� �����ش�.
		LARGE_INTEGER tTime;
		QueryPerformanceCounter(&tTime);    // ���� Ŭ�ϼ� ��������

		// ȣ��1���� �̵��ð� =	����� ���� Ŭ�ϼ��� ����  /  1�ʴ� Ŭ�ϼ�  ex)   36/360  
		g_fDeltaTime = (tTime.QuadPart - g_tTime.QuadPart) / (float)g_tSecond.QuadPart;

		DuckingEngine::getInstance().Update(g_fDeltaTime);

		// �����ð��� ���ݽð����� �ʱ�ȭ���ش�.
		g_tTime = tTime;
	}

	void Application::renderFrame()
	{
		DuckingEngine::getInstance().Render(g_fDeltaTime);
	}
}