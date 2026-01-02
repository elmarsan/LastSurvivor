#pragma once

#include "survivor_types.h"
#include "survivor_renderer_opengl.h"
#include "survivor_platform.h"
#include "survivor_math.h"
#include "survivor_memory.h"
#include "survivor_camera.h"
#include "survivor_debug.h"

struct Player
{
    v3 position;
    v3 target;
    v3 speed;
};

struct GameState
{
    b32   initialized;
    Arena arena;

    GeometryBuffer* cubeBuffer;
    Program*        program;
    Player*         player;
    Camera*         camera;
    AudioClip*      pistolShot;
    AudioClip*      backgroundMusic;

#ifdef BUILD_TYPE_DEBUG
    DebugState* debug;
#endif
};