#pragma once

#include "survivor_types.h"
#include "survivor_math.h"
#include "survivor_physics.h"
#include "survivor_renderer_opengl.h"
#include "survivor_platform.h"
#include "survivor_memory.h"
#include "survivor_camera.h"
#include "survivor_debug.h"
#include "survivor_obj.h"
#include "survivor_world.h"
#include "survivor_entity.h"

#define TTF_FIRST_GLYPH_OFFSET 32 // Space ascii code
#define TTF_GLYPH_COUNT        95
#define MAX_ENTITY_COUNT       128

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

struct Graph
{
    std::vector<cell_index>       nodes;
    std::vector<std::vector<u32>> edges;
};

struct CellInfo
{
    Entity* entities[4];
    u32     entityCount;
};

struct GameState
{
    b32   initialized;
    Arena arena;

    GeometryBuffer* planeBuffer;
    GeometryBuffer* characterBuffer;
    GeometryBuffer* fenceBuffer;
    AABB*           fenceAABB;
    Program*        program;
    Camera*         camera;
    AudioClip*      pistolShot;
    AudioClip*      backgroundMusic;
    Texture*        whiteTexture;
    Texture*        crosshairAtlas;
    Texture*        glyphAtlas;
    Texture*        fenceDiffuseMapTexture;
    BatchBuffer*    batchBuffer;
    TTFGlyph        ttfChars[TTF_GLYPH_COUNT];
    Entity          entities[MAX_ENTITY_COUNT];
    u32             entityCount;
    Graph*          graph;
    b32             buildMode;
    CellInfo*       cellInfo;

#ifdef BUILD_TYPE_DEBUG
    DebugState* debug;
#endif
};