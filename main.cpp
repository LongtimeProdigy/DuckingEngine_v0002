#include "stdafx.h"
#include "Application.h"

#ifdef DK_WINDOW
#include <Windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	try
	{
		// #todo- 아마도.. Facotry Pattern을 쓰는 게 좋을듯?
		Application* application = dk_new WindowApplication;
		ApplicationInitializeData data(hInstance, true, 1920, 1080, false);
		if (application->Initialize(data) == false)
		{
			DK_ASSERT_LOG(false, "Application Initialization - Failed");
		}

		application->Run();
	}
	catch (const std::exception& exo)
	{
		DK_ASSERT_LOG(false, exo.what());
		return -1;
	}

	return 0;
}
#elif DK_XBOX
int main()
{
}
#elif DK_PS
int main()
{
}
#endif