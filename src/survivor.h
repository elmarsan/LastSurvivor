#pragma once

#include "survivor_types.h"
#include "survivor_renderer_opengl.h"
#include "survivor_platform.h"
#include "survivor_math.h"
#include "survivor_memory.h"
#include "survivor_camera.h"
#include "survivor_debug.h"
#include "survivor_geometry.h"

struct Player
{
    v3  position;
    v3  target;
    v3  velocity;
    f32 yaw; // Radians
};

struct Vertex
{
    v3 position;
    v3 normal;
    v2 uv;
};

struct Vertex2D
{
    v2 position;
    v2 uv;
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

    Program*        program2D;
    GeometryBuffer* quad2DBuffer;
    Texture*        texture;
    v2u             cursor;

#ifdef BUILD_TYPE_DEBUG
    DebugState* debug;
#endif
};