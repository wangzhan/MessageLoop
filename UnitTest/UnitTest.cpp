// UnitTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
//#include "vld.h"
#include "UnitTest.h"
#include "gtest/gtest.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(NULL);

	if (hModule != NULL)
	{
		// initialize MFC and print and error on failure
		if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
		{
			// TODO: change error code to suit your needs
			_tprintf(_T("Fatal Error: MFC initialization failed\n"));
			nRetCode = 1;
		}
		else
		{
			// start gtest
            testing::InitGoogleTest(&argc, argv);
            nRetCode = RUN_ALL_TESTS();
		}
	}
	else
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
		nRetCode = 1;
	}

    std::cout << "Press any key to continue ..." << std::endl;
    getchar();

	return nRetCode;
}
