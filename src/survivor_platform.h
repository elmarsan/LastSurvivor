#pragma once

enum PlatformErrorType
{
    PlatformErrorTypeFatal,
    PlatformErrorTypeNonFatal
};

#define PLATFORM_ERROR_MESSAGE(name) void name(PlatformErrorType errorType, const char* message)
typedef PLATFORM_ERROR_MESSAGE(PlatformErrorMessage);

struct PlatformAPI
{
    PlatformErrorMessage* ErrorMessage;
};

struct GameMemory
{
    PlatformAPI platform;
    OpenGL      opengl;
};

#define GAME_UPDATE_AND_RENDER(name) b32 name(GameMemory* memory, f32 delta)
typedef GAME_UPDATE_AND_RENDER(GameUpdateAndRenderProc);