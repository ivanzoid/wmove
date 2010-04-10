#include <windows.h>

#pragma comment(linker,"/NODEFAULTLIB")
#pragma comment(linker,"/ENTRY:WinMain")


int WINAPI WinMain(HINSTANCE /*hInst*/, HINSTANCE /*hPrevInst*/, PSTR /*cmdLine*/, int /*showCmd*/)
{
	LoadLibrary(L"wmhooks.dll");

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}
