#pragma once

#include "survivor_types.h"
#include "survivor_renderer_opengl.h"
#include "survivor_platform.h"
#include "survivor_math.h"
#include "survivor_memory.h"
#include "survivor_camera.h"
#include "survivor_debug.h"

#define TTF_FIRST_GLYPH_OFFSET 32 // Space ascii code
#define TTF_GLYPH_COUNT        95

struct Player
{
    v3  position;
    v3  velocity;
    f32 yaw; // Radians
};

struct Vertex
{
    v3 position;
    v3 normal;
    v2 uv;
};

struct BatchVertex
{
    v3  position;
    v2  uv;
    v4  color;
    u32 textureIndex;
};

struct BatchBuffer
{
    GeometryBuffer buffer;
    Program        program;
    BatchVertex*   vertexBufferBase;
    BatchVertex*   vertexBufferPtr;
    u32*           indexBufferBase;
    u32*           indexBufferPtr;
    u32            maxVertexCount;
    u32            maxIndexCount;
    u32            vertexCount;
    u32            indexCount;
};

struct TTFGlyph
{
    u16 x0, y0, x1, y1; // Bounding-box
    f32 xoff, yoff, xadvance;
    f32 s0, t0, s1, t1; // Texture coordinates, relative to bounding-box.
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
    Texture*        whiteTexture;
    Texture*        crosshairAtlas;
    Texture*        glyphAtlas;
    BatchBuffer*    batchBuffer;
    TTFGlyph        ttfChars[TTF_GLYPH_COUNT];

#ifdef BUILD_TYPE_DEBUG
    DebugState* debug;
#endif
};