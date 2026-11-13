#include "stdafx.h"
#include "Application.h"

#ifdef _DK_WINDOW_
#include <shlobj.h>
#include "FileUtil.h"

#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")

#if defined(USE_PIX)
namespace DK
{
    static DKStringW GetLatestWinPixGpuCapturerPath()
    {
        LPWSTR programFilesPath = nullptr;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, nullptr, &programFilesPath)))
        {
            DK_ASSERT_LOG(false, "Cannot locate Program Files");
            return {};
        }
        const DKString pixDirectory = FileUtil::jointPath(FileUtil::fromWidePath(programFilesPath), "Microsoft PIX");
        CoTaskMemFree(programFilesPath);
        const DKStringW searchPath = FileUtil::toWidePath(FileUtil::jointPath(pixDirectory, "*"));
        WIN32_FIND_DATAW findData{};
        HANDLE search = FindFirstFileW(searchPath.c_str(), &findData);
        if (search == INVALID_HANDLE_VALUE)
        {
            const DWORD status = GetLastError();
            DK_ASSERT_LOG(status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND, "PIX installation search failed: %lu", status);
            return {};
        }
        DKStringW newestVersion;
        do
        {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && findData.cFileName[0] != L'.')
            {
                if (newestVersion.empty() || StringUtil::wcscmp(newestVersion.c_str(), findData.cFileName) <= 0)
                    newestVersion = findData.cFileName;
            }
        } while (FindNextFileW(search, &findData) != 0);
        const DWORD status = GetLastError();
        FindClose(search);
        DK_ASSERT_LOG(status == ERROR_NO_MORE_FILES, "PIX installation enumeration failed: %lu", status);
        if (newestVersion.empty())
            return {};
        return FileUtil::toWidePath(FileUtil::jointPath(FileUtil::jointPath(pixDirectory, FileUtil::fromWidePath(newestVersion)), "WinPixGpuCapturer.dll"));
    }
}
#endif

#include <Windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
    //SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#ifdef USE_IMGUI
    ImGui_ImplWin32_EnableDpiAwareness();
#endif

    DK::gMainThreadID = GetCurrentThreadId();

#if defined(USE_PIX)
    // PIX dll (https://devblogs.microsoft.com/pix/taking-a-capture/)
    // Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
    // This may happen if the application is launched through the PIX UI.
    if (GetModuleHandleW(L"WinPixGpuCapturer.dll") == nullptr)
    {
        const DK::DKStringW capturerPath = DK::GetLatestWinPixGpuCapturerPath();
        if (capturerPath.empty() == false)
        {
            const HMODULE capturer = LoadLibraryW(capturerPath.c_str());
            DK_ASSERT_LOG(capturer != nullptr, "PIX capturer load failed");
        }
    }
#endif

    int WIDTH = 1920;
    int HEIGHT = 1080;
#if defined(_DK_DEBUG_)
    int num = 0;
    LPWSTR lp = GetCommandLine();
    LPWSTR* argv = CommandLineToArgvW(lp, &num);
    if (argv == NULL)
    {
        DK_ASSERT_LOG(false, "Commandline Parsing Failed");
        return -1;
    }
    for (int i = 1; i < num; ++i)
    {
        DK::StringSplitter splitter(DK::StringUtil::convertWCtoC(argv[i]), "=");
        if (splitter[0].find("--") == 0)
        {
            if (DK::StringUtil::strcmp(splitter[0].c_str(), "--resourcePath") == 0)
                DK::GlobalPath::setResourcePath(splitter[1]);
            else if (DK::StringUtil::strcmp(splitter[0].c_str(), "--width") == 0)
                WIDTH = DK::StringUtil::atoi(splitter[1].c_str());
            else if (DK::StringUtil::strcmp(splitter[0].c_str(), "--height") == 0)
                HEIGHT = DK::StringUtil::atoi(splitter[1].c_str());
        }
        else if (splitter[0].find('-') == 0)
        {
            if (DK::StringUtil::strcmp(splitter[0].c_str(), "-r") == 0)
                DK::GlobalPath::setResourcePath(splitter[1]);
            else if (DK::StringUtil::strcmp(splitter[0].c_str(), "-w") == 0)
                WIDTH = DK::StringUtil::atoi(splitter[1].c_str());
            else if (DK::StringUtil::strcmp(splitter[0].c_str(), "-h") == 0)
                HEIGHT = DK::StringUtil::atoi(splitter[1].c_str());
        }
        else
        {
            DK_ASSERT_LOG(false, "CommandLine Argument Parsing Error.");
            return -1;
        }
    }
    LocalFree(argv);
#endif

	DK::Application* application = dk_new DK::Application;
	DK::ApplicationInitializeData data(hInstance, true, WIDTH, HEIGHT, false);
	if (application->initialize(data) == false)
	{
		DK_ASSERT_LOG(false, "Application Initialization - Failed");
        return -1;
	}

	application->run();

	return 0;
}
#endif
