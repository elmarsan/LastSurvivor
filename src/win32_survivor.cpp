#include "survivor_types.h"
#include "survivor_platform.h"

#include <windows.h>

struct Win32State
{
    HWND window;
    b32  running;
};

global Win32State gWin32State;

PLATFORM_ERROR_MESSAGE(Win32ErrorMessage)
{
    const char* caption = "LastSurvivor Warning";

    UINT mboxType = MB_OK;
    if (errorType == PlatformErrorTypeFatal)
    {
        caption = "LastSurvivor Fatal Error";
        mboxType |= MB_ICONSTOP;
    }
    else
    {
        mboxType |= MB_ICONWARNING;
    }

    MessageBoxEx(gWin32State.window, message, caption, mboxType, 0);

    if (errorType == PlatformErrorTypeFatal)
    {
        ExitProcess(1);
    }
}

LRESULT WndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (msg)
    {
    case WM_SIZE:
    {
        break;
    }
    case WM_CLOSE:
    {
        gWin32State.running = false;
        break;
    }
    default:
    {
        result = DefWindowProc(window, msg, wParam, lParam);
        break;
    }
    }

    return result;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
#ifdef BUILD_TYPE_DEBUG
    AllocConsole();
    FILE* fconsole;
    freopen_s(&fconsole, "CONOUT$", "w", stdout);
#endif

    WNDCLASS windowClass      = { 0 };
    windowClass.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc   = WndProc;
    windowClass.hInstance     = hInstance;
    windowClass.lpszClassName = "SurvivorWindowClassname";

    if (!RegisterClass(&windowClass))
    {
        Win32ErrorMessage(PlatformErrorTypeFatal, "Unable to register game window handle");
    }

    DWORD styles = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    HWND  window = CreateWindow(windowClass.lpszClassName, "LastSurvivor", styles, CW_USEDEFAULT, CW_USEDEFAULT, 1280,
                                800, 0, 0, hInstance, 0);

    if (!window)
    {
        Win32ErrorMessage(PlatformErrorTypeFatal, "Unable to open game window");
    }

    gWin32State.window  = window;
    gWin32State.running = true;

    while (gWin32State.running)
    {
        MSG message;

        while (PeekMessage(&message, gWin32State.window, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }

    return 0;
}