/*
 Code for mouse and keyboard hook library.
*/

#include <windows.h>

#pragma comment(linker,"/NODEFAULTLIB")
#pragma comment(linker,"/ENTRY:DllMain")

#pragma section("shared" , read, write, shared)
#define DLL_SHARED __declspec(allocate("shared"))

struct MouseMove {
    HWND    hWnd;
    POINT   mouseStartPos;
    POINT   wndStartPos;
    SIZE    wndStartSize;
};

DLL_SHARED struct Dll {
    DWORD   hostProcessPid;
    HMODULE dllHandle;
    HHOOK   hKbHook;
    HHOOK   hMouseHook;
    bool    altPressed;
    bool    leftMousePressed;
    bool    rightMousePressed;
    MouseMove mouse;
} dll;

#undef DLL_SHARED

// Note: will overrun if result > 1024 bytes
/*
void DbgPrint(const char *fmt, ...)
{
    char buf[1024];

    va_list arglist;
    va_start(arglist, fmt);

    wvsprintfA(buf, fmt, arglist);

    va_end(arglist);

    OutputDebugStringA(buf);
}
*/

LRESULT CALLBACK MouseHookLLFunction(int code, WPARAM wParam, LPARAM lParam)
{
    if ((code < 0) || (code != HC_ACTION) || !dll.altPressed) {
        return CallNextHookEx(dll.hMouseHook, code, wParam, lParam);
    }

    MSLLHOOKSTRUCT *m = (MSLLHOOKSTRUCT *)(lParam);

    //DbgPrint("m.x=%d\tm.y=%d\n", m->pt.x, m->pt.y);

    if ((wParam == WM_LBUTTONDOWN) || (wParam == WM_RBUTTONDOWN)) {

        dll.mouse.mouseStartPos = m->pt;

        HWND hPointWnd = WindowFromPoint(dll.mouse.mouseStartPos);
        dll.mouse.hWnd = GetAncestor(hPointWnd, GA_ROOT);

        RECT r;
        GetWindowRect(dll.mouse.hWnd, &r);
        dll.mouse.wndStartPos.x = r.left;
        dll.mouse.wndStartPos.y = r.top;
        dll.mouse.wndStartSize.cx = r.right - r.left;
        dll.mouse.wndStartSize.cy = r.bottom - r.top;

        if (wParam == WM_LBUTTONDOWN)

            dll.leftMousePressed = true;

        else if (wParam == WM_RBUTTONDOWN)

            dll.rightMousePressed = true;

        return 1;

    } else if (wParam == WM_LBUTTONUP) {

        dll.leftMousePressed = false;

        return 1;

    } else if (wParam == WM_RBUTTONUP) {

        dll.rightMousePressed = false;

        return 1;

    } else if (wParam == WM_MOUSEMOVE) {

        int dx = m->pt.x - dll.mouse.mouseStartPos.x; 
        int dy = m->pt.y - dll.mouse.mouseStartPos.y;

        if (dll.leftMousePressed) {

            MoveWindow(
                dll.mouse.hWnd,
                dll.mouse.wndStartPos.x + dx,
                dll.mouse.wndStartPos.y + dy,
                dll.mouse.wndStartSize.cx,
                dll.mouse.wndStartSize.cy,
                TRUE);

            return CallNextHookEx(dll.hMouseHook, code, wParam, lParam);

        } else if (dll.rightMousePressed) {

            MoveWindow(
                dll.mouse.hWnd,
                dll.mouse.wndStartPos.x,
                dll.mouse.wndStartPos.y,
                dll.mouse.wndStartSize.cx + dx,
                dll.mouse.wndStartSize.cy + dy,
                TRUE);

            return CallNextHookEx(dll.hMouseHook, code, wParam, lParam);
        }
    }

    return CallNextHookEx(dll.hMouseHook, code, wParam, lParam);
}

LRESULT CALLBACK KeyboardHookLLFunction(int code, WPARAM wParam, LPARAM lParam)
{
    if ((code < 0) || (code != HC_ACTION))
        return CallNextHookEx(dll.hKbHook, code, wParam, lParam);

    KBDLLHOOKSTRUCT *key = (KBDLLHOOKSTRUCT *)lParam;

    //DbgPrint("key scan=%d\tvk=%d\tflags=%d\n", key->scanCode, key->vkCode, key->flags);

    if ((key->flags & LLKHF_ALTDOWN) && !dll.altPressed) {

        dll.leftMousePressed = false;
        dll.rightMousePressed = false;
        dll.hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookLLFunction, dll.dllHandle, 0);
        dll.altPressed = true;
        //DbgPrint("mouseHook=%x\n", dll.hMouseHook);

    } else if (!(key->flags & LLKHF_ALTDOWN) && dll.altPressed) {

        UnhookWindowsHookEx(dll.hMouseHook);
        //DbgPrint("unhook mouse\n");
        dll.altPressed = false;
    }

    return CallNextHookEx(dll.hKbHook, code, wParam, lParam);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH) {

        if (dll.hostProcessPid == 0) {

            dll.hostProcessPid = GetCurrentProcessId();
            //DbgPrint("Host process attached with pid=%d\n", dll.hostProcessPid);
            dll.dllHandle = hModule;
            dll.hKbHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookLLFunction, dll.dllHandle, 0);
            //DbgPrint("kbHook=%x\n", dll.hKbHook);
        }

    } else if (dwReason == DLL_PROCESS_DETACH) {

        //DbgPrint("Process detached with pid=%d\n", GetCurrentProcessId());

        if (GetCurrentProcessId() == dll.hostProcessPid) {

            //DbgPrint("Host process detached\n");

            if (dll.hKbHook)
                UnhookWindowsHookEx(dll.hKbHook);
            if (dll.hMouseHook)
                UnhookWindowsHookEx(dll.hMouseHook);
        }
    }

    return TRUE;
}
