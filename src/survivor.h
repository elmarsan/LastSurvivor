#pragma once

#include "survivor_types.h"
#include "survivor_math.h"
#include "survivor_renderer_opengl.h"
#include "survivor_platform.h"
#include "survivor_memory.h"
#include "survivor_camera.h"
#include "survivor_debug.h"

#define TTF_FIRST_GLYPH_OFFSET 32 // Space ascii code
#define TTF_GLYPH_COUNT        95
#define MAX_ENTITY_COUNT       128

enum EntityType
{
    EntityType_Player,
    EntityType_Enemy,
    EntityType_Object
};

enum EntityFlag
{
    EntityFlag_InKnockback = (1 << 0)
};

struct Entity
{
    EntityType type;
    v3         position;
    v3         velocity;
    v3         size;
    f32        yaw;          // TODO: Replace by v3/quat for rotations?
    Entity*    targetEntity; // TODO: Needed? All enemies will follow player
    AABB       aabb;
    u32        flags;
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
    GeometryBuffer* planeBuffer; // TODO: Batch
    GeometryBuffer* objBuffer;
    Program*        program;
    Camera*         camera;
    AudioClip*      pistolShot;
    AudioClip*      backgroundMusic;
    Texture*        whiteTexture;
    Texture*        crosshairAtlas;
    Texture*        glyphAtlas;
    BatchBuffer*    batchBuffer;
    TTFGlyph        ttfChars[TTF_GLYPH_COUNT];

    Entity entities[MAX_ENTITY_COUNT];
    u32    entityCount;

#ifdef BUILD_TYPE_DEBUG
    DebugState* debug;
#endif
};