#include <windows.h>
#include <xinput.h>
#define XAUDIO2_HELPER_FUNCTIONS
#include <xaudio2.h>
#include <gl/glcorearb.h>
#include <gl/GL.h>
#include <gl/wglext.h>

#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

#ifdef BUILD_TYPE_DEBUG
#include <gl/glext.h>

#include "survivor_types.h"
#include "survivor_math.h"
#include "survivor_renderer_opengl.h"
#include "survivor_platform.h"
#elif defined(BUILD_TYPE_RELEASE)
#include "survivor.cpp"
#endif

#define GL_PROC_ADDRESS(name) name = (decltype(name))wglGetProcAddress(#name)

struct Wind32XAudio2
{
    IXAudio2*               engine;
    IXAudio2MasteringVoice* masteringVoice;
    IXAudio2SubmixVoice*    musicSubmixVoice;
    IXAudio2SubmixVoice*    sfxSubmixVoice;
};

struct Win32AudioClip
{
    WAVEFORMATEX         wave;
    XAUDIO2_BUFFER       buffer;
    size_t               length;
    void*                data;
    IXAudio2SourceVoice* sourceVoice;
    IXAudio2SubmixVoice* submixVoice;
};

struct Win32State
{
    HWND          window;
    HDC           deviceContext;
    HGLRC         openglContext;
    b32           running;
    b32           paused;
    s64           performanceCounterFreq;
    f32           deltaTime;
    GameInput     gameInput;
    Wind32XAudio2 xaudio2;
};

struct Win32GameCode
{
    HMODULE                  dllHandle;
    FILETIME                 dllLastWriteTime;
    GameUpdateAndRenderProc* UpdateAndRender;
};

global_variable Win32State gWin32State;

internal PLATFORM_ERROR_MESSAGE(Win32ErrorMessage)
{
    const char* caption = "LastSurvivor Warning";

    UINT mboxType = MB_OK;
    if (errorType == PlatformErrorType_Fatal)
    {
        caption = "LastSurvivor Fatal Error";
        mboxType |= MB_ICONSTOP;
    }
    else
    {
        mboxType |= MB_ICONWARNING;
    }

    MessageBoxEx(gWin32State.window, message, caption, mboxType, 0);

    if (errorType == PlatformErrorType_Fatal)
    {
        ExitProcess(1);
    }
}

internal PLATFORM_LOGF(Win32Log)
{
#if BUILD_TYPE_DEBUG
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
#endif
}

internal PLATFORM_FILE_READ_ENTIRE(Win32FileReadEntire)
{
    FileReadResult result = { 0 };

    FILE* file = fopen(filename, "rb");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        if (size != -1L)
        {
            result.contentSize = (size_t)size;
            result.content     = malloc(sizeof(u8) * size);
            fseek(file, 0, SEEK_SET);
            fread(result.content, 1, result.contentSize, file);
            fclose(file);
        }
        else
        {
            Log("Unable to reach the end of the file '%s'", filename);
        }
    }
    else
    {
        Log("Unable to open file '%s'", filename);
    }

    return result;
}

internal PLATFORM_FILE_FREE(Win32FileFree)
{
    if (fileContent)
    {
        free(fileContent);
    }
}

internal PLATFORM_WINDOW_GET_DIMENSION(Win32WindowGetDimension)
{
    v2u result;

    RECT rect;
    GetClientRect(gWin32State.window, &rect);
    result.w = (u32)(rect.right - rect.left);
    result.h = (u32)(rect.bottom - rect.top);

    return result;
}

internal void APIENTRY Win32OpenGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                                const GLchar* message, const void* userParam)
{
    // Intel integrated gpu error
    // API_ID_LINE_WIDTH deprecated behavior warning has been generated. Wide lines have been deprecated.
    // glLineWidth set to 2.000000. glLineWidth with width greater than 1.0 will generate GL_INVALID_VALUE error in
    // future versions
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    if (strcmp(vendor, "Intel") == 0 && id == 7)
    {
        return;
    }

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

internal inline FILETIME Win32GetFileLastWriteTime(const char* filename)
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
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// DLL hot reloading
internal Win32GameCode Win32GameCodeLoad(const char* gameDLLFilename, const char* copyDLLFilename)
{
    Win32GameCode result = {};

    result.dllLastWriteTime = Win32GetFileLastWriteTime(gameDLLFilename);
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

internal inline void Win32UpdateGameButtonState(GameButtonState* buttonState, b32 isDown)
{
    Assert(buttonState);

    buttonState->isDown = isDown;
}
//  ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// XAudio2
internal PLATFORM_AUDIO_CLIP_LOAD(Win32AudioClipLoad)
{
    AudioClip* audioClip = (AudioClip*)malloc(sizeof(audioClip));
    audioClip->handle    = malloc(sizeof(Win32AudioClip));

    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalPCMFrameCount;
    drwav_int16* pSampleData =
        drwav_open_file_and_read_pcm_frames_s16(filename, &channels, &sampleRate, &totalPCMFrameCount, 0);
    if (!pSampleData)
    {
        Log("Audio file '%s' not found", filename);
        Assert(0);
    }

    WAVEFORMATEX wave    = { 0 };
    wave.wFormatTag      = WAVE_FORMAT_PCM;
    wave.nChannels       = (WORD)channels;
    wave.nSamplesPerSec  = sampleRate;
    wave.wBitsPerSample  = 16;
    wave.nBlockAlign     = (wave.nChannels * wave.wBitsPerSample) / 8;
    wave.nAvgBytesPerSec = wave.nSamplesPerSec * wave.nBlockAlign;
    wave.cbSize          = 0;

    XAUDIO2_BUFFER buffer = { 0 };
    buffer.AudioBytes     = (UINT32)(totalPCMFrameCount * wave.nBlockAlign);
    buffer.pAudioData     = (BYTE*)pSampleData;
    buffer.Flags          = XAUDIO2_END_OF_STREAM;

    Win32AudioClip* handle = (Win32AudioClip*)audioClip->handle;
    handle->data           = (s16*)pSampleData;
    handle->buffer         = buffer;
    handle->wave           = wave;
    handle->length         = totalPCMFrameCount;

    Wind32XAudio2* xaudio2 = &gWin32State.xaudio2;
    if (type == AudioClipType_Music)
    {
        handle->submixVoice = xaudio2->musicSubmixVoice;
    }
    else if (type == AudioClipType_Sfx)
    {
        handle->submixVoice = xaudio2->sfxSubmixVoice;
    }
    else
    {
        InvalidCodePath;
    }

    XAUDIO2_SEND_DESCRIPTOR sendDescriptor = { 0 };
    XAUDIO2_VOICE_SENDS     voiceSends     = { 0 };
    sendDescriptor.pOutputVoice            = handle->submixVoice;
    voiceSends.SendCount                   = 1;
    voiceSends.pSends                      = &sendDescriptor;

    HRESULT result = xaudio2->engine->CreateSourceVoice(&handle->sourceVoice, &wave, 0, XAUDIO2_DEFAULT_FREQ_RATIO, 0,
                                                        &voiceSends, 0);
    if (result != S_OK)
    {
        Assert(0);
    }

    return audioClip;
}

internal PLATFORM_AUDIO_CLIP_FREE(Win32AudioClipFree)
{
    Win32AudioClip* win32AudioClip = (Win32AudioClip*)clip->handle;
    if (win32AudioClip->data)
    {
        drwav_free(win32AudioClip->data, 0);
    }
}

internal PLATFORM_AUDIO_CLIP_PLAY(Win32AudioClipPlay)
{
    Win32AudioClip* win32AudioClip = (Win32AudioClip*)clip->handle;
    XAUDIO2_BUFFER* buffer         = &win32AudioClip->buffer;

    if (flags & AudioClipPlayFlag_Loop)
    {
        buffer->LoopBegin  = 0;
        buffer->LoopLength = (UINT32)win32AudioClip->length;
        buffer->LoopCount  = XAUDIO2_LOOP_INFINITE;
    }
    else
    {
        buffer->LoopLength = 0;
        buffer->LoopCount  = 0;
    }

    win32AudioClip->sourceVoice->FlushSourceBuffers();
    win32AudioClip->sourceVoice->SubmitSourceBuffer(buffer);
    win32AudioClip->sourceVoice->Start(0, XAUDIO2_COMMIT_NOW);
}

internal PLATFORM_AUDIO_SET_VOLUME(Win32AudioSetVolume)
{
    Wind32XAudio2* xaudio2 = &gWin32State.xaudio2;

    f32 volume = XAudio2DecibelsToAmplitudeRatio(db);

    if (type == AudioClipType_Music)
    {
        xaudio2->musicSubmixVoice->SetVolume(volume);
    }
    else if (type == AudioClipType_Sfx)
    {
        xaudio2->sfxSubmixVoice->SetVolume(volume);
    }
    else
    {

        InvalidCodePath;
    }
}

internal void Win32XAudio2Init(Win32State* state)
{
    Wind32XAudio2* xaudio2 = &state->xaudio2;

    if (SUCCEEDED(CoInitializeEx(0, COINIT_MULTITHREADED)))
    {
        HRESULT result = XAudio2Create(&xaudio2->engine, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (result == S_OK)
        {
            result =
                xaudio2->engine->CreateMasteringVoice(&xaudio2->masteringVoice, XAUDIO2_DEFAULT_CHANNELS,
                                                      XAUDIO2_DEFAULT_SAMPLERATE, 0, 0, 0, AudioCategory_GameMedia);
            Assert(result == S_OK);

            XAUDIO2_VOICE_DETAILS details;
            xaudio2->masteringVoice->GetVoiceDetails(&details);

            result = xaudio2->engine->CreateSubmixVoice(&xaudio2->musicSubmixVoice, details.InputChannels,
                                                        details.InputSampleRate, 0, 0, 0, 0);
            Assert(result == S_OK);
            result = xaudio2->engine->CreateSubmixVoice(&xaudio2->sfxSubmixVoice, details.InputChannels,
                                                        details.InputSampleRate, 0, 0, 0, 0);
            Assert(result == S_OK);
        }
        else
        {
            Win32ErrorMessage(PlatformErrorType_Fatal, "Unable to initialize create XAudio2 engine");
        }
    }
    else
    {
        Win32ErrorMessage(PlatformErrorType_Fatal, "Unable to initialize COM library");
    }
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
            u32 vkCode        = (u32)msg.wParam;
            b32 wasDown       = (msg.lParam >> 30) & 1;
            b32 isDown        = ((msg.lParam >> 31) & 1) == 0;
            b32 altKeyWasDown = (msg.lParam >> 29) & 1;

            if (wasDown != isDown)
            {
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
                    Win32UpdateGameButtonState(&keyboard->moveLeft, isDown);
                }
                if (vkCode == 'D' || vkCode == VK_LEFT)
                {
                    Win32UpdateGameButtonState(&keyboard->moveRight, isDown);
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
#if BUILD_TYPE_DEBUG
                DebugInput* debug = &state->gameInput.debug;

                if (vkCode == VK_F1)
                {
                    Win32UpdateGameButtonState(&debug->f1, isDown);
                }
                if (vkCode == VK_F2)
                {
                    Win32UpdateGameButtonState(&debug->f2, isDown);
                }
                if (vkCode == VK_F3)
                {
                    Win32UpdateGameButtonState(&debug->f3, isDown);
                }
                if (vkCode == VK_F4)
                {
                    Win32UpdateGameButtonState(&debug->f4, isDown);
                }
                if (vkCode == VK_F5)
                {
                    Win32UpdateGameButtonState(&debug->f5, isDown);
                }
                if (vkCode == VK_SPACE)
                {
                    Win32UpdateGameButtonState(&debug->space, isDown);
                }
#endif
            }
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

    // Handle mouse input
    {
        POINT point;
        GetCursorPos(&point);
        ScreenToClient(state->window, &point);

        u32 newMouseX = (u32)point.x;
        u32 newMouseY = (u32)point.y;
        s32 offsetX   = (s32)newMouseX - (s32)mouse->pos.x;
        s32 offsetY   = (s32)newMouseY - (s32)mouse->pos.y;

        mouse->pos.x    = newMouseX;
        mouse->pos.y    = newMouseY;
        mouse->offset.x = offsetX;
        mouse->offset.y = offsetY;

        Win32UpdateGameButtonState(&mouse->left, GetKeyState(VK_LBUTTON) & (1 << 15));
        Win32UpdateGameButtonState(&mouse->middle, GetKeyState(VK_MBUTTON) & (1 << 15));
        Win32UpdateGameButtonState(&mouse->right, GetKeyState(VK_RBUTTON) & (1 << 15));
    }
}

LRESULT WndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (msg)
    {
    case WM_SIZE:
    {
        v2u dimension = Win32WindowGetDimension();
        glViewport(0, 0, GLsizei(dimension.w), GLsizei(dimension.h));
        break;
    }
    case WM_CLOSE:
    {
        gWin32State.running = false;
        break;
    }
    case WM_KILLFOCUS:
    {
        GameInput* gameInput = &gWin32State.gameInput;
        for (u32 controllerIndex = 0; controllerIndex < ArrayCount(gameInput->controllers); controllerIndex++)
        {
            GameInputController* controller = GetController(gameInput, controllerIndex);

            for (u32 buttonIndex = 0; buttonIndex < ArrayCount(controller->buttons); buttonIndex++)
            {
                controller->buttons[buttonIndex].isDown  = false;
                controller->buttons[buttonIndex].wasDown = false;
            }
        }

        Mouse* mouse = &gWin32State.gameInput.mouse;
        for (u32 mouseButtonIndex = 0; mouseButtonIndex < ArrayCount(mouse->buttons); mouseButtonIndex++)
        {
            mouse->buttons[mouseButtonIndex].isDown  = false;
            mouse->buttons[mouseButtonIndex].wasDown = false;
        }

        gWin32State.paused = true;
        break;
    }
    case WM_SETFOCUS:
    {
        gWin32State.paused = false;
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
        Win32ErrorMessage(PlatformErrorType_Fatal, "Unable to register game window handle");
    }

    DWORD styles = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    HWND  window = CreateWindow(windowClass.lpszClassName, "LastSurvivor", styles, CW_USEDEFAULT, CW_USEDEFAULT, 1280,
                                800, 0, 0, hInstance, 0);

    if (!window)
    {
        Win32ErrorMessage(PlatformErrorType_Fatal, "Unable to open game window");
    }

    Win32XInputInit();
    Win32XAudio2Init(&gWin32State);

    GameMemory gameMemory                  = {};
    gameMemory.platform.ErrorMessage       = Win32ErrorMessage;
    gameMemory.platform.Logf               = Win32Log;
    gameMemory.platform.FileReadEntire     = Win32FileReadEntire;
    gameMemory.platform.FileFree           = Win32FileFree;
    gameMemory.platform.WindowGetDimension = Win32WindowGetDimension;
    gameMemory.platform.AudioClipLoad      = Win32AudioClipLoad;
    gameMemory.platform.AudioClipPlay      = Win32AudioClipPlay;
    gameMemory.platform.AudioClipFree      = Win32AudioClipFree;
    gameMemory.platform.AudioSetVolume     = Win32AudioSetVolume;

    gameMemory.permanentStorageSize = Gigabytes(1);
    gameMemory.permanentStorage =
        VirtualAlloc(NULL, (size_t)gameMemory.permanentStorageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!gameMemory.permanentStorage)
    {
        Win32ErrorMessage(PlatformErrorType_Fatal, "Unable to allocate permanent storage size");
    }

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
                Win32ErrorMessage(PlatformErrorType_Fatal, "Unable to register OpenGL window handle");
            }
            HDC  dummyDC     = GetWindowDC(dummyWND);
            int  pixelFormat = ChoosePixelFormat(dummyDC, &pfd);
            BOOL result      = SetPixelFormat(dummyDC, pixelFormat, &pfd);
            if (!result)
            {
                Win32ErrorMessage(PlatformErrorType_Fatal, "Unable to set OpenGL pixel format");
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
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
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
            Win32ErrorMessage(PlatformErrorType_Fatal, "Unable to set OpenGL pixel format");
        }

        gWin32State.openglContext = wglCreateContextAttribsARB(gWin32State.deviceContext, 0, glContextAttrs);
        result                    = wglMakeCurrent(gWin32State.deviceContext, gWin32State.openglContext);
        if (!result)
        {
            Win32ErrorMessage(PlatformErrorType_Fatal, "Unable to set OpenGL context");
        }

        GLint contextFlags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);

#if BUILD_TYPE_DEBUG
        const GLubyte* vendor   = glGetString(GL_VENDOR);
        const GLubyte* renderer = glGetString(GL_RENDERER);
        const GLubyte* version  = glGetString(GL_VERSION);

        Log("------------------------------------------------------------");
        Log("OpenGL context created");
        Log("Vendor   %s", vendor);
        Log("Renderer %s", renderer);
        Log("Version  %s", version);
        Log("------------------------------------------------------------");
#endif

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

        gameMemory.opengl.glEnable            = glEnable;
        gameMemory.opengl.glDisable           = glDisable;
        gameMemory.opengl.glClear             = glClear;
        gameMemory.opengl.glClearColor        = glClearColor;
        gameMemory.opengl.glDrawArrays        = glDrawArrays;
        gameMemory.opengl.glDrawElements      = glDrawElements;
        gameMemory.opengl.glLineWidth         = glLineWidth;
        gameMemory.opengl.glPolygonMode       = glPolygonMode;
        gameMemory.opengl.glGenTextures       = glGenTextures;
        gameMemory.opengl.glBindTexture       = glBindTexture;
        gameMemory.opengl.glTexImage2D        = glTexImage2D;
        gameMemory.opengl.glTexParameteri     = glTexParameteri;
        gameMemory.opengl.glTexParameteriv    = glTexParameteriv;
        gameMemory.opengl.glBlendFunc         = glBlendFunc;
        gameMemory.opengl.glActiveTexture     = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
        gameMemory.opengl.glCreateProgram     = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
        gameMemory.opengl.glCreateShader      = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
        gameMemory.opengl.glAttachShader      = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
        gameMemory.opengl.glDeleteShader      = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
        gameMemory.opengl.glLinkProgram       = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
        gameMemory.opengl.glDeleteProgram     = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
        gameMemory.opengl.glShaderSource      = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
        gameMemory.opengl.glUseProgram        = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
        gameMemory.opengl.glGetShaderiv       = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
        gameMemory.opengl.glGetShaderInfoLog  = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
        gameMemory.opengl.glCompileShader     = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
        gameMemory.opengl.glGetProgramiv      = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
        gameMemory.opengl.glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
        gameMemory.opengl.glGenBuffers        = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
        gameMemory.opengl.glGenVertexArrays   = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
        gameMemory.opengl.glBindBuffer        = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
        gameMemory.opengl.glBindVertexArray   = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
        gameMemory.opengl.glBufferData        = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
        gameMemory.opengl.glBufferSubData     = (PFNGLBUFFERSUBDATAPROC)wglGetProcAddress("glBufferSubData");
        gameMemory.opengl.glDeleteBuffers     = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
        gameMemory.opengl.glEnableVertexAttribArray =
            (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
        gameMemory.opengl.glVertexAttribPointer =
            (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
        gameMemory.opengl.glVertexAttribIPointer =
            (PFNGLVERTEXATTRIBIPOINTERPROC)wglGetProcAddress("glVertexAttribIPointer");
        gameMemory.opengl.glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)wglGetProcAddress("glDeleteVertexArrays");
        gameMemory.opengl.glActiveTexture      = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
        gameMemory.opengl.glGenerateMipmap     = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
        gameMemory.opengl.glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
        gameMemory.opengl.glUniformMatrix4fv   = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
        gameMemory.opengl.glUniform1i          = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
        gameMemory.opengl.glUniform1ui         = (PFNGLUNIFORM1UIPROC)wglGetProcAddress("glUniform1ui");
        gameMemory.opengl.glUniform1fv         = (PFNGLUNIFORM1FVPROC)wglGetProcAddress("glUniform1fv");
        gameMemory.opengl.glUniform3fv         = (PFNGLUNIFORM3FVPROC)wglGetProcAddress("glUniform3fv");
        gameMemory.opengl.glUniform4fv         = (PFNGLUNIFORM4FVPROC)wglGetProcAddress("glUniform4fv");
        gameMemory.opengl.glUniform1iv         = (PFNGLUNIFORM1IVPROC)wglGetProcAddress("glUniform1iv");
    }

#if BUILD_TYPE_DEBUG
    const char*   gameDLLFilename     = "Survivor.dll";
    const char*   tempGameDLLFilename = "CopySurvivor.dll";
    Win32GameCode game                = Win32GameCodeLoad(gameDLLFilename, tempGameDLLFilename);
#endif

    gWin32State.gameInput.keyboard.isAnalog    = false;
    gWin32State.gameInput.keyboard.isConnected = true;
    gWin32State.gameInput.gamepad.isAnalog     = true;

    LARGE_INTEGER frameStartTime = Win32GetWallClock();

    gWin32State.running = true;
    while (gWin32State.running)
    {
#if BUILD_TYPE_DEBUG
        FILETIME lastGameDLLWriteTime = Win32GetFileLastWriteTime(gameDLLFilename);
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

        for (u32 mouseButtonIndex = 0; mouseButtonIndex < ArrayCount(gWin32State.gameInput.mouse.buttons);
             mouseButtonIndex++)
        {
            Mouse* mouse                             = &gWin32State.gameInput.mouse;
            mouse->buttons[mouseButtonIndex].wasDown = mouse->buttons[mouseButtonIndex].isDown;
        }

#if BUILD_TYPE_DEBUG
        for (u32 debugBtnIndex = 0; debugBtnIndex < ArrayCount(gWin32State.gameInput.debug.fkeys); debugBtnIndex++)
        {
            gWin32State.gameInput.debug.fkeys[debugBtnIndex].wasDown =
                gWin32State.gameInput.debug.fkeys[debugBtnIndex].isDown;
        }
#endif

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

        b32 exitGame = false;
        if (!gWin32State.paused)
        {
#ifdef BUILD_TYPE_DEBUG
            exitGame = game.UpdateAndRender(&gameMemory, &gWin32State.gameInput, gWin32State.deltaTime);
#elif defined(BUILD_TYPE_RELEASE)
            exitGame = GameUpdateAndRender(&gameMemory, &gWin32State.gameInput, gWin32State.deltaTime);
#endif
            SwapBuffers(gWin32State.deviceContext);
        }

        LARGE_INTEGER frameEndTime = Win32GetWallClock();
        gWin32State.deltaTime      = (f32)Win32GetSecondsElapsed(frameStartTime, frameEndTime);
        frameStartTime             = frameEndTime;

        if (exitGame)
        {
            gWin32State.running = false;
        }
    }

    return 0;
}