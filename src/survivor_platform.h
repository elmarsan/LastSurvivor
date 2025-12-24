#pragma once

enum PlatformErrorType
{
    PlatformErrorTypeFatal,
    PlatformErrorTypeNonFatal
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

#define PLATFORM_ERROR_MESSAGE(name) void name(PlatformErrorType errorType, const char* message)
typedef PLATFORM_ERROR_MESSAGE(PlatformErrorMessage);

#define PLATFORM_LOGF(name) void name(const char* fmt, ...)
typedef PLATFORM_LOGF(PlatformLog);

struct PlatformAPI
{
    PlatformErrorMessage* ErrorMessage;
    PlatformLog*          Logf;
};

struct GameMemory
{
    PlatformAPI platform;
    OpenGL      opengl;
};

#define GAME_UPDATE_AND_RENDER(name) b32 name(GameMemory* memory, GameInput* input, f32 delta)
typedef GAME_UPDATE_AND_RENDER(GameUpdateAndRenderProc);