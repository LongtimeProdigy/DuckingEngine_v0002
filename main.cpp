#include "stdafx.h"
#include "Application.h"

#ifdef _DK_WINDOW_
#include <shlobj.h>
#include <strsafe.h>

#if defined(USE_PIX)
static std::wstring GetLatestWinPixGpuCapturerPath()
{
    LPWSTR programFilesPath = nullptr;
    SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

    std::wstring pixSearchPath = programFilesPath + std::wstring(L"\\Microsoft PIX\\*");

    WIN32_FIND_DATA findData;
    bool foundPixInstallation = false;
    wchar_t newestVersionFound[MAX_PATH];

    HANDLE hFind = FindFirstFile(pixSearchPath.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) &&
                (findData.cFileName[0] != '.'))
            {
                if (!foundPixInstallation || wcscmp(newestVersionFound, findData.cFileName) <= 0)
                {
                    foundPixInstallation = true;
                    StringCchCopy(newestVersionFound, _countof(newestVersionFound), findData.cFileName);
                }
            }
        } while (FindNextFile(hFind, &findData) != 0);
    }

    FindClose(hFind);

    if (!foundPixInstallation)
    {
        // TODO: Error, no PIX installation found
    }

    wchar_t output[MAX_PATH];
    StringCchCopy(output, pixSearchPath.length(), pixSearchPath.data());
    StringCchCat(output, MAX_PATH, &newestVersionFound[0]);
    StringCchCat(output, MAX_PATH, L"\\WinPixGpuCapturer.dll");

    return &output[0];
}
#endif

#include <Windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	try
	{
        DK::gMainThreadID = GetCurrentThreadId();

#if defined(USE_PIX)
        // PIX dll (https://devblogs.microsoft.com/pix/taking-a-capture/)
        // Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
        // This may happen if the application is launched through the PIX UI. 
        if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
            LoadLibrary(GetLatestWinPixGpuCapturerPath().c_str());
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
	}
	catch (const std::exception& exo)
	{
		DK_ASSERT_LOG(false, exo.what());
		return -1;
	}

	return 0;
}
#endif
