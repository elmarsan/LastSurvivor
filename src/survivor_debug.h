#pragma once

struct DebugVertex
{
    v3 position;
    v4 color;
};

struct DebugState
{
    Arena arena;
    Arena frameArena;

    // Permanent
    Program        program;
    GeometryBuffer buffer;
    DebugVertex*   vertexBufferBase;
    DebugVertex*   vertexBufferPtr;
    u32*           indexBufferBase;
    u32*           indexBufferPtr;

    // Frame lifetime
    mat4x4* viewProj;
    u32     indexCount;
    u32     vertexCount;
};

void DebugInit(DebugState* state, OpenGL* opengl, PlatformAPI* platform);
void DebugFrameBegin(DebugState* state, OpenGL* opengl, mat4x4 viewProj);
void DebugFrameEnd(DebugState* state, OpenGL* opengl);
void DebugDrawLine(DebugState* state, OpenGL* opengl, v3 start, v3 end, v4 color);
void DebugDrawPlane(DebugState* state, OpenGL* opengl, v3 position, v3 size, v4 color);
void DebugDrawAABB(DebugState* state, OpenGL* opengl, v3 worldPosition, f32 yRotation, AABB aabb, v4 color);