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

inline b32 ButtonIsPressed(GameButtonState button) { return (button.isDown && !button.wasDown); }
inline b32 ButtonIsDown(GameButtonState button) { return button.isDown; }
inline b32 ButtonIsUp(GameButtonState button) { return !button.isDown; }
inline b32 ButtonWasDown(GameButtonState button) { return button.wasDown; }

struct GameInputController
{
    b32 isConnected;
    b32 isWireless;
    b32 isAnalog;

    glm::vec2 stickLeft;
    glm::vec2 stickRight;

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
    glm::uvec2 pos;
    glm::uvec2 offset;

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

struct DebugInput
{
    union
    {
        struct
        {
            GameButtonState f0;
            GameButtonState f1;
            GameButtonState f2;
            GameButtonState f3;
            GameButtonState f4;
            GameButtonState f5;
            GameButtonState space;
        };

        GameButtonState fkeys[7];
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

#if BUILD_TYPE_DEBUG
    DebugInput debug;
#endif
};

inline GameInputController* GetController(GameInput* gameInput, u32 controllerIndex)
{
    Assert(controllerIndex >= 0 && controllerIndex <= ArrayCount(gameInput->controllers));
    return &gameInput->controllers[controllerIndex];
}

struct FileReadResult
{
    void*  content;
    size_t contentSize;
};

struct AudioClip
{
    void* handle;
};

enum AudioClipType
{
    AudioClipType_Music,
    AudioClipType_Sfx
};

enum
{
    AudioClipPlayFlag_Loop = (1 << 0)
};

#define PLATFORM_ERROR_MESSAGE(name) void name(PlatformErrorType errorType, char* message)
typedef PLATFORM_ERROR_MESSAGE(PlatformErrorMessage);

#define PLATFORM_LOGF(name) void name(const char* fmt, ...)
typedef PLATFORM_LOGF(PlatformLog);

#define PLATFORM_FILE_READ_ENTIRE(name) FileReadResult name(char* filename)
typedef PLATFORM_FILE_READ_ENTIRE(PlatformFileReadEntire);

#define PLATFORM_FILE_FREE(name) void name(void* fileContent)
typedef PLATFORM_FILE_FREE(PlatformFileFree);

#define PLATFORM_FILE_WRITE_ENTIRE(name) void name(char* filename, void* fileContent, size_t size)
typedef PLATFORM_FILE_WRITE_ENTIRE(PlatformFileWriteEntire);

#define PLATFORM_WINDOW_GET_DIMENSION(name) glm::vec2 name()
typedef PLATFORM_WINDOW_GET_DIMENSION(PlatformWindowGetDimension);

#define PLATFORM_AUDIO_CLIP_LOAD(name) AudioClip* name(char* filename, AudioClipType type)
typedef PLATFORM_AUDIO_CLIP_LOAD(PlatformAudioClipLoad);

#define PLATFORM_AUDIO_CLIP_FREE(name) void name(AudioClip* clip)
typedef PLATFORM_AUDIO_CLIP_FREE(PlatformAudioClipFree);

#define PLATFORM_AUDIO_CLIP_PLAY(name) void name(AudioClip* clip, u32 flags)
typedef PLATFORM_AUDIO_CLIP_PLAY(PlatformAudioClipPlay);

#define PLATFORM_AUDIO_SET_VOLUME(name) void name(f32 db, AudioClipType type)
typedef PLATFORM_AUDIO_SET_VOLUME(PlatformAudioSetVolume);

struct PlatformAPI
{
    PlatformErrorMessage*       ErrorMessage;
    PlatformLog*                Logf;
    PlatformFileReadEntire*     FileReadEntire;
    PlatformFileFree*           FileFree;
    PlatformFileWriteEntire*    FileWriteEntire;
    PlatformWindowGetDimension* WindowGetDimension;
    PlatformAudioClipLoad*      AudioClipLoad;
    PlatformAudioClipFree*      AudioClipFree;
    PlatformAudioClipPlay*      AudioClipPlay;
    PlatformAudioSetVolume*     AudioSetVolume;
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