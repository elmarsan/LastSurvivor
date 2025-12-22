#include <windows.h>
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
    HWND  window;
    HDC   deviceContext;
    HGLRC openglContext;
    b32   running;
    s64   performanceCounterFreq;
    f32   deltaTime;
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

    GameMemory gameMemory = {};

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

        MSG message;
        while (PeekMessage(&message, gWin32State.window, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

#ifdef BUILD_TYPE_DEBUG
        b32 exitGame = game.UpdateAndRender(&gameMemory, gWin32State.deltaTime);
#elif defined(BUILD_TYPE_RELEASE)
        b32 exitGame = GameUpdateAndRender(&gameMemory, gWin32State.deltaTime);
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