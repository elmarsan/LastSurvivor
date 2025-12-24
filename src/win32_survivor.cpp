#include <windows.h>
#include <xinput.h>
#include <gl/GL.h>
#include <gl/wglext.h>
#include <gl/glcorearb.h>

#ifdef BUILD_TYPE_DEBUG
#include <gl/glext.h>

#include "survivor_types.h"
#include "survivor_opengl.h"
#include "survivor_platform.h"
#elif defined(BUILD_TYPE_RELEASE)
#include "survivor.cpp"
#endif

#define GL_PROC_ADDRESS(name) name = (decltype(name))wglGetProcAddress(#name)

struct Win32State
{
    HWND      window;
    HDC       deviceContext;
    HGLRC     openglContext;
    b32       running;
    s64       performanceCounterFreq;
    f32       deltaTime;
    GameInput gameInput;
};

struct Win32GameCode
{
    HMODULE                  dllHandle;
    FILETIME                 dllLastWriteTime;
    GameUpdateAndRenderProc* UpdateAndRender;
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

PLATFORM_LOGF(Win32Log)
{
#if BUILD_TYPE_DEBUG
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
#endif
}

internal void APIENTRY Win32OpenGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                                const GLchar* message, const void* userParam)
{
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        Log("------------------------------------------------------------");
        Log("OpenGL debug message: %s", message);
        // clang-format off
        switch (source)
        {
            case GL_DEBUG_SOURCE_API:             Log("Source: API");             break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   Log("Source: Window System");   break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER: Log("Source: Shader Compiler"); break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:     Log("Source: Third Party");     break;
            case GL_DEBUG_SOURCE_APPLICATION:     Log("Source: Application");     break;
            case GL_DEBUG_SOURCE_OTHER:           Log("Source: Other");           break;
        }
        switch (type)
        {
            case GL_DEBUG_TYPE_ERROR:               Log("Type: Error");                break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: Log("Type: Deprecated Behaviour"); break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  Log("Type: Undefined Behaviour");  break;
            case GL_DEBUG_TYPE_PORTABILITY:         Log("Type: Portability");          break;
            case GL_DEBUG_TYPE_PERFORMANCE:         Log("Type: Performance");          break;
            case GL_DEBUG_TYPE_MARKER:              Log("Type: Marker");               break;
            case GL_DEBUG_TYPE_PUSH_GROUP:          Log("Type: Push Group");           break;
            case GL_DEBUG_TYPE_POP_GROUP:           Log("Type: Pop Group");            break;
            case GL_DEBUG_TYPE_OTHER:               Log("Type: Other");                break;
        }
        Log("------------------------------------------------------------");
        // clang-format on
    }
}

internal inline FILETIME Win32GetLastWriteTime(const char* filename)
{
    FILETIME result = {};

    WIN32_FIND_DATAA findData;
    HANDLE           findHandle = FindFirstFileA(filename, &findData);
    if (findHandle != INVALID_HANDLE_VALUE)
    {
        result = findData.ftLastWriteTime;
        FindClose(findHandle);
    }

    return result;
}

internal Win32GameCode Win32GameCodeLoad(const char* gameDLLFilename, const char* copyDLLFilename)
{
    Win32GameCode result = {};

    result.dllLastWriteTime = Win32GetLastWriteTime(gameDLLFilename);
    CopyFileA(gameDLLFilename, copyDLLFilename, FALSE);

    result.dllHandle = LoadLibraryA(copyDLLFilename);
    if (result.dllHandle)
    {
        result.UpdateAndRender = (GameUpdateAndRenderProc*)GetProcAddress(result.dllHandle, "GameUpdateAndRender");

        if (!result.UpdateAndRender)
        {
            Log("Unable to get game DLL 'GameUpdateAndRender' proc address, windows error code: %d", GetLastError());
            Assert(0);
        }
    }
    else
    {
        Log("Unable to load game DLL '%s', windows error code: %d", gameDLLFilename, GetLastError());
        Assert(0);
    }

    return result;
}

internal void Win32GameCodeRelease(Win32GameCode* gameCode)
{
    if (gameCode->dllHandle)
    {
        FreeLibrary(gameCode->dllHandle);
        gameCode->dllHandle = 0;
        Log("Game code released");
    }

    gameCode->dllLastWriteTime = {};
    gameCode->UpdateAndRender  = 0;

    Sleep(100);
}

// ----------------------------------------------------------------------------
// Time
internal inline LARGE_INTEGER Win32GetWallClock()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

internal inline f64 Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    return (f64)(end.QuadPart - start.QuadPart) / (f64)gWin32State.performanceCounterFreq;
}

internal inline void Win32UpdateGameButtonState(GameButtonState* buttonState, bool isDown)
{
    Assert(buttonState);

    buttonState->isDown = isDown;
}
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// XInput
typedef DWORD(WINAPI* XInputGetStateFunc)(DWORD, XINPUT_STATE*);
typedef DWORD(WINAPI* XInputGetCapabilitiesFunc)(DWORD, DWORD, XINPUT_CAPABILITIES*);

internal XInputGetStateFunc        XInputGetStateProc;
internal XInputGetCapabilitiesFunc XInputGetCapabilitiesProc;

#define XInputGetState        XInputGetStateProc
#define XInputGetCapabilities XInputGetCapabilitiesProc

internal void Win32XInputInit()
{
    //
    Log("XInput initializing...");

    // Windows 8 (XInput 1.4), DirectX SDK (XInput 1.3), Windows Vista (XInput 9.1.0)
    const char* library   = "xinput1_4.dll";
    HMODULE     XInputDLL = LoadLibraryA(library);

    if (!XInputDLL)
    {
        Log("Unable to load XInput dll: '%s'", library);
        Assert(0);
    }
    else
    {
        XInputGetStateProc        = (XInputGetStateFunc)GetProcAddress(XInputDLL, "XInputGetState");
        XInputGetCapabilitiesProc = (XInputGetCapabilitiesFunc)GetProcAddress(XInputDLL, "XInputGetCapabilities");

        Assert(XInputGetStateProc && XInputGetCapabilitiesProc);
    }
}

internal inline f32 Win32GetControllerStick(SHORT stickValue, SHORT deadzone)
{
    f32 value = 0.0f;

    if (stickValue < -deadzone)
    {
        value = (f32)((stickValue + deadzone) / (32768.0f - deadzone));
    }
    else if (stickValue > deadzone)
    {
        value = (f32)((stickValue - deadzone) / (32768.0f - deadzone));
    }

    return value;
}
//  ----------------------------------------------------------------------------

internal void Win32ProcessPendingMessages(Win32State* state)
{
    Assert(state);

    GameInputController* keyboard = &state->gameInput.keyboard;
    Mouse*               mouse    = &state->gameInput.mouse;

    MSG msg;
    while (PeekMessage(&msg, gWin32State.window, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_KEYDOWN:
        {
            u32  vkCode        = (u32)msg.wParam;
            bool wasDown       = (msg.lParam >> 30) & 1;
            bool isDown        = ((msg.lParam >> 31) & 1) == 0;
            bool altKeyWasDown = (msg.lParam >> 29) & 1;

            if (vkCode == 'W' || vkCode == VK_UP)
            {
                Win32UpdateGameButtonState(&keyboard->moveUp, isDown);
            }
            if (vkCode == 'S' || vkCode == VK_DOWN)
            {
                Win32UpdateGameButtonState(&keyboard->moveDown, isDown);
            }
            if (vkCode == 'A' || vkCode == VK_RIGHT)
            {
                Win32UpdateGameButtonState(&keyboard->moveRight, isDown);
            }
            if (vkCode == 'D' || vkCode == VK_LEFT)
            {
                Win32UpdateGameButtonState(&keyboard->moveLeft, isDown);
            }
            if (vkCode == VK_ESCAPE)
            {
                Win32UpdateGameButtonState(&keyboard->start, isDown);
            }
            if (vkCode == VK_BACK)
            {
                Win32UpdateGameButtonState(&keyboard->back, isDown);
            }
            if (vkCode == VK_F4 && altKeyWasDown)
            {
                Log("ALT+F4 pressed, closing game...");
                state->running = false;
            }
            break;
        }
        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
        {
            Win32UpdateGameButtonState(&mouse->left, msg.wParam & MK_LBUTTON);
            break;
        }
        case WM_MBUTTONUP:
        case WM_MBUTTONDOWN:
        {
            Win32UpdateGameButtonState(&mouse->middle, msg.wParam & MK_MBUTTON);
            break;
        }
        case WM_RBUTTONUP:
        case WM_RBUTTONDOWN:
        {
            Win32UpdateGameButtonState(&mouse->right, msg.wParam & MK_RBUTTON);
            break;
        }
        default:
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            break;
        }
        }
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
#if BUILD_TYPE_DEBUG
    AllocConsole();
    FILE* fconsole;
    freopen_s(&fconsole, "CONOUT$", "w", stdout);
#endif

    LARGE_INTEGER perfCounterFrequencyResult;
    QueryPerformanceFrequency(&perfCounterFrequencyResult);
    gWin32State.performanceCounterFreq = perfCounterFrequencyResult.QuadPart;

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

    Win32XInputInit();

    GameMemory gameMemory            = {};
    gameMemory.platform.ErrorMessage = Win32ErrorMessage;
    gameMemory.platform.Logf         = Win32Log;

    // OpenGL context creation and function loading
    {
        gWin32State.window        = window;
        gWin32State.deviceContext = GetWindowDC(gWin32State.window);

        PFNWGLCHOOSEPIXELFORMATARBPROC    wglChoosePixelFormatARB;
        PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

        PIXELFORMATDESCRIPTOR pfd = { 0 };
        pfd.nSize                 = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion              = 1;
        pfd.dwFlags               = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType            = PFD_TYPE_RGBA;
        pfd.cColorBits            = 32;
        pfd.cRedBits              = 8;
        pfd.cGreenBits            = 8;
        pfd.cBlueBits             = 8;
        pfd.cAlphaBits            = 8;
        pfd.cDepthBits            = 24;
        pfd.cStencilBits          = 8;
        pfd.iLayerType            = PFD_MAIN_PLANE;

        // Dummy context
        {
            HWND dummyWND = CreateWindowA(windowClass.lpszClassName, NULL, WS_POPUP, 0, 0, 0, 0, 0, 0, 0, 0);
            if (!dummyWND)
            {
                Win32ErrorMessage(PlatformErrorTypeFatal, "Unable to register OpenGL window handle");
            }
            HDC  dummyDC     = GetWindowDC(dummyWND);
            int  pixelFormat = ChoosePixelFormat(dummyDC, &pfd);
            BOOL result      = SetPixelFormat(dummyDC, pixelFormat, &pfd);
            if (!result)
            {
                Win32ErrorMessage(PlatformErrorTypeFatal, "Unable to set OpenGL pixel format");
            }

            HGLRC dummyglRC = wglCreateContext(dummyDC);
            wglMakeCurrent(dummyDC, dummyglRC);

            GL_PROC_ADDRESS(wglChoosePixelFormatARB);
            GL_PROC_ADDRESS(wglCreateContextAttribsARB);

            wglMakeCurrent(0, 0);
            ReleaseDC(dummyWND, dummyDC);
            wglDeleteContext(dummyglRC);
            DestroyWindow(dummyWND);
        }

        // clang-format off
        int pixelAttrs[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
            WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,     32,
            WGL_DEPTH_BITS_ARB,     24,
            WGL_STENCIL_BITS_ARB,   8,
            WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
            // TODO: Anti-Aliasing
            // WGL_SAMPLES_ARB, 4,
            0,
        };
        int glContextAttrs[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 2,
            WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#if BUILD_TYPE_DEBUG
            WGL_CONTEXT_FLAGS_ARB,         WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
            0,
        };
        // clang-format on

        int  pixelFormat;
        UINT numFormats;
        wglChoosePixelFormatARB(gWin32State.deviceContext, pixelAttrs, NULL, 1, &pixelFormat, &numFormats);
        BOOL result = SetPixelFormat(gWin32State.deviceContext, pixelFormat, &pfd);
        if (!result)
        {
            Win32ErrorMessage(PlatformErrorTypeFatal, "Unable to set OpenGL pixel format");
        }

        gWin32State.openglContext = wglCreateContextAttribsARB(gWin32State.deviceContext, 0, glContextAttrs);
        result                    = wglMakeCurrent(gWin32State.deviceContext, gWin32State.openglContext);
        if (!result)
        {
            Win32ErrorMessage(PlatformErrorTypeFatal, "Unable to set OpenGL context");
        }

        GLint major, minor, contextFlags;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);

        Log("OpenGL context created, version %d.%d", major, minor);

        if (contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
#ifdef BUILD_TYPE_RELEASE
            // Debug context must be disabled in release build
            Assert(0);
#endif

            PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback;
            PFNGLDEBUGMESSAGECONTROLPROC  glDebugMessageControl;
            GL_PROC_ADDRESS(glDebugMessageCallback);
            GL_PROC_ADDRESS(glDebugMessageControl);

            Log("OpenGL debug callback enabled");
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(Win32OpenGLDebugCallback, 0);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
        }

        PFNGLUSEPROGRAMPROC glUseProgram;
        GL_PROC_ADDRESS(glUseProgram);

        gameMemory.opengl.glClear      = glClear;
        gameMemory.opengl.glClearColor = glClearColor;
        gameMemory.opengl.glUseProgram = glUseProgram;
    }

#if BUILD_TYPE_DEBUG
    const char*   gameDLLFilename     = "Survivor.dll";
    const char*   tempGameDLLFilename = "CopySurvivor.dll";
    Win32GameCode game                = Win32GameCodeLoad(gameDLLFilename, tempGameDLLFilename);
#endif

    LARGE_INTEGER frameStartTime = Win32GetWallClock();

    gWin32State.running = true;
    while (gWin32State.running)
    {
#if BUILD_TYPE_DEBUG
        FILETIME lastGameDLLWriteTime = Win32GetLastWriteTime(gameDLLFilename);
        if (CompareFileTime(&lastGameDLLWriteTime, &game.dllLastWriteTime) == 1)
        {
            SYSTEMTIME lastDLLWriteTime, previousDLLWriteTime;
            FileTimeToSystemTime(&lastGameDLLWriteTime, &lastDLLWriteTime);
            FileTimeToSystemTime(&game.dllLastWriteTime, &previousDLLWriteTime);

            Log("Previous game dll write time: %02d:%02d:%02d%02d", previousDLLWriteTime.wHour,
                previousDLLWriteTime.wMinute, previousDLLWriteTime.wSecond, previousDLLWriteTime.wMilliseconds);
            Log("Current game dll write time: %02d:%02d:%02d%02d", lastDLLWriteTime.wHour, lastDLLWriteTime.wMinute,
                lastDLLWriteTime.wSecond, lastDLLWriteTime.wMilliseconds);

            Log("Reloading game code...");
            Win32GameCodeRelease(&game);
            game = Win32GameCodeLoad(gameDLLFilename, tempGameDLLFilename);
        }
#endif

        for (u32 controllerIndex = 0; controllerIndex < ArrayCount(gWin32State.gameInput.controllers);
             controllerIndex++)
        {
            GameInputController* controller = &gWin32State.gameInput.controllers[controllerIndex];

            for (u32 buttonIndex = 0; buttonIndex < ArrayCount(controller->buttons); buttonIndex++)
            {
                controller->buttons[buttonIndex].wasDown = controller->buttons[buttonIndex].isDown;
            }
        }

        Win32ProcessPendingMessages(&gWin32State);

        XINPUT_STATE         controllerState;
        DWORD                controllerIndex = 0;
        DWORD                result          = XInputGetState(controllerIndex, &controllerState);
        GameInputController* gamepad         = &gWin32State.gameInput.gamepad;

        if (result == ERROR_SUCCESS)
        {
            XINPUT_GAMEPAD gamepadState = controllerState.Gamepad;

            SHORT leftStickDeadzone  = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
            SHORT rightStickDeadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
            BYTE  triggerThreshold   = 45;

            gamepad->back.isDown         = gamepadState.wButtons & XINPUT_GAMEPAD_BACK;
            gamepad->start.isDown        = gamepadState.wButtons & XINPUT_GAMEPAD_START;
            gamepad->actionUp.isDown     = gamepadState.wButtons & XINPUT_GAMEPAD_Y;
            gamepad->actionDown.isDown   = gamepadState.wButtons & XINPUT_GAMEPAD_A;
            gamepad->actionRight.isDown  = gamepadState.wButtons & XINPUT_GAMEPAD_B;
            gamepad->actionLeft.isDown   = gamepadState.wButtons & XINPUT_GAMEPAD_X;
            gamepad->moveUp.isDown       = gamepadState.wButtons & XINPUT_GAMEPAD_DPAD_UP;
            gamepad->moveDown.isDown     = gamepadState.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
            gamepad->moveRight.isDown    = gamepadState.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
            gamepad->moveLeft.isDown     = gamepadState.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
            gamepad->leftTrigger.isDown  = gamepadState.bLeftTrigger >= triggerThreshold;
            gamepad->rightTrigger.isDown = gamepadState.bRightTrigger >= triggerThreshold;
            gamepad->stickLeft.x         = Win32GetControllerStick(gamepadState.sThumbLX, leftStickDeadzone);
            gamepad->stickLeft.y         = Win32GetControllerStick(gamepadState.sThumbLY, leftStickDeadzone);
            gamepad->stickRight.x        = Win32GetControllerStick(gamepadState.sThumbRX, rightStickDeadzone);
            gamepad->stickRight.y        = Win32GetControllerStick(gamepadState.sThumbRY, rightStickDeadzone);

            if (!gamepad->isConnected)
            {
                gamepad->isConnected = true;

                XINPUT_CAPABILITIES controllerCaps;
                DWORD getCapsResult = XInputGetCapabilities(controllerIndex, XINPUT_FLAG_GAMEPAD, &controllerCaps);
                if (getCapsResult == ERROR_SUCCESS)
                {
                    if (controllerCaps.Flags & XINPUT_CAPS_WIRELESS)
                    {
                        gamepad->isWireless = true;
                        Log("XInput controller 0 connected (wireless)");
                    }
                    else
                    {
                        gamepad->isWireless = false;
                        Log("XInput controller 0 connected (wired)");
                    }
                }
                else
                {
                    Log("XInput unable to get controller 0 capabilities, error code '%lu'", getCapsResult);
                }
            }
        }
        else if (result == ERROR_DEVICE_NOT_CONNECTED)
        {
            if (gamepad->isConnected)
            {
                Log("XInput controller 0 disconnected");
                gamepad->isConnected = false;
            }
        }
        else
        {
            Log("XInputGetState unable to get controller 0 state, error code: '%lu'", result);
        }

#ifdef BUILD_TYPE_DEBUG
        b32 exitGame = game.UpdateAndRender(&gameMemory, &gWin32State.gameInput, gWin32State.deltaTime);
#elif defined(BUILD_TYPE_RELEASE)
        b32 exitGame = GameUpdateAndRender(&gameMemory, &gWin32State.gameInput, gWin32State.deltaTime);
#endif
        SwapBuffers(gWin32State.deviceContext);

        LARGE_INTEGER frameEndTime = Win32GetWallClock();
        gWin32State.deltaTime      = Win32GetSecondsElapsed(frameStartTime, frameEndTime);
        frameStartTime             = frameEndTime;

        if (exitGame)
        {
            gWin32State.running = false;
        }
    }

    return 0;
}