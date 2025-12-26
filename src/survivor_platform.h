#pragma once

enum PlatformErrorType
{
    PlatformErrorType_Fatal,
    PlatformErrorType_NonFatal
};

struct GameButtonState
{
    b32 isDown;
    b32 wasDown;
};

struct GameInputController
{
    b32 isConnected;
    b32 isWireless;

    v2 stickLeft;
    v2 stickRight;

    union
    {
        struct
        {
            GameButtonState moveUp;
            GameButtonState moveDown;
            GameButtonState moveLeft;
            GameButtonState moveRight;

            GameButtonState actionUp;
            GameButtonState actionDown;
            GameButtonState actionRight;
            GameButtonState actionLeft;

            GameButtonState rightTrigger;
            GameButtonState leftTrigger;

            GameButtonState back;
            GameButtonState start;
        };

        GameButtonState buttons[12];
    };
};

struct Mouse
{
    v2u pos;

    union
    {
        struct
        {
            GameButtonState left;
            GameButtonState middle;
            GameButtonState right;
        };

        GameButtonState buttons[3];
    };
};

struct GameInput
{
    Mouse mouse;

    union
    {
        struct
        {
            GameInputController keyboard;
            GameInputController gamepad;
        };

        GameInputController controllers[2];
    };
};

struct FileReadResult
{
    void* content;
    u64   contentSize;
};

#define PLATFORM_ERROR_MESSAGE(name) void name(PlatformErrorType errorType, const char* message)
typedef PLATFORM_ERROR_MESSAGE(PlatformErrorMessage);

#define PLATFORM_LOGF(name) void name(const char* fmt, ...)
typedef PLATFORM_LOGF(PlatformLog);

#define PLATFORM_FILE_READ_ENTIRE(name) FileReadResult name(const char* filename)
typedef PLATFORM_FILE_READ_ENTIRE(PlatformFileReadEntire);

#define PLATFORM_FILE_FREE(name) void name(void* fileContent)
typedef PLATFORM_FILE_FREE(PlatformFileFree);

#define PLATFORM_WINDOW_GET_DIMENSION(name) v2u name()
typedef PLATFORM_WINDOW_GET_DIMENSION(PlatformWindowGetDimension);

struct PlatformAPI
{
    PlatformErrorMessage*       ErrorMessage;
    PlatformLog*                Logf;
    PlatformFileReadEntire*     FileReadEntire;
    PlatformFileFree*           FileFree;
    PlatformWindowGetDimension* WindowGetDimension;
};

struct GameMemory
{
    PlatformAPI platform;
    OpenGL      opengl;
    void*       permanentStorage;
    u64         permanentStorageSize;
};

#define GAME_UPDATE_AND_RENDER(name) b32 name(GameMemory* memory, GameInput* input, f32 delta)
typedef GAME_UPDATE_AND_RENDER(GameUpdateAndRenderProc);