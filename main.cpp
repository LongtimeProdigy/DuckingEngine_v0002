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
#if defined(USE_PIX)
        // PIX dll 로딩 (출처: https://devblogs.microsoft.com/pix/taking-a-capture/)
        // Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
        // This may happen if the application is launched through the PIX UI. 
        if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
            LoadLibrary(GetLatestWinPixGpuCapturerPath().c_str());
#endif

#if defined(_DK_DEBUG_)
        int num = 0;
        LPWSTR lp = GetCommandLine();
        LPWSTR* argv = CommandLineToArgvW(lp, &num);
        if (argv == NULL)
        {
            DK_ASSERT_LOG(false, "Commandline Parsing Failed");
            return -1;
        }
        for (int i = 1; i < num; ++i)   // 0번째는 exe 경로
        {
            DK::StringSplitter splitter(DK::StringUtil::convertWCtoC(argv[i]), "=");
            if (splitter[0].find("--") == 0)    // 단축어X
            {
                if (DK::StringUtil::strcmp(splitter[0].c_str(), "--resourcePath") == true)
                    DK::GlobalPath::setResourcePath(splitter[1]);
            }
            else if (splitter[0].find('-') == 0)    // 단축어
            {
                if (DK::StringUtil::strcmp(splitter[0].c_str(), "-r") == true)
                    DK::GlobalPath::setResourcePath(splitter[1]);
            }
            else
            {
                DK_ASSERT_LOG(false, "Command는 -또는 --로 시작해야합니다.");
            }
        }
        LocalFree(argv);
#endif

		DK::Application* application = dk_new DK::Application;
		DK::ApplicationInitializeData data(hInstance, true, 1920, 1080, false);
		if (application->initialize(data) == false)
		{
			DK_ASSERT_LOG(false, "Application Initialization - Failed");
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