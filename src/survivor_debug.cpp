#define MAX_DEBUG_VERTICES 1024
#define MAX_DEBUG_INDICES  MAX_DEBUG_VERTICES * 3

internal void DebugFlush(DebugState* state, OpenGL* opengl);

void DebugInit(DebugState* state, OpenGL* opengl, PlatformAPI* platform)
{
    FileReadResult debugVs = platform->FileReadEntire("../src/shaders/debug.vert");
    FileReadResult debugFs = platform->FileReadEntire("../src/shaders/debug.frag");

    ProgramInit(opengl, &state->program);
    ProgramAttachShader(opengl, &state->program, (const char*)debugVs.content, debugVs.contentSize, GL_VERTEX_SHADER);
    ProgramAttachShader(opengl, &state->program, (const char*)debugFs.content, debugFs.contentSize, GL_FRAGMENT_SHADER);
    ProgramBuild(opengl, &state->program);

    platform->FileFree(debugVs.content);
    platform->FileFree(debugFs.content);

    size_t vertexSize = sizeof(DebugVertex);

    GeometryBufferInit(opengl, &state->buffer, GL_LINES);
    GeometryBufferVBOAlloc(opengl, &state->buffer, 0, vertexSize * MAX_DEBUG_VERTICES, vertexSize, GL_DYNAMIC_DRAW);
    GeometryBufferEBOAlloc(opengl, &state->buffer, 0, sizeof(u32) * MAX_DEBUG_INDICES, sizeof(u32), GL_DYNAMIC_DRAW);
    GeometryBufferVertexAttrib(opengl, &state->buffer, 0, 3, GL_FLOAT, vertexSize, offsetof(DebugVertex, position));
    GeometryBufferVertexAttrib(opengl, &state->buffer, 1, 3, GL_FLOAT, vertexSize, offsetof(DebugVertex, color));

    state->vertexBufferBase = PushArray(&state->arena, MAX_DEBUG_VERTICES, DebugVertex);
    state->vertexBufferPtr  = state->vertexBufferBase;

    state->indexBufferBase = PushArray(&state->arena, MAX_DEBUG_INDICES, u32);
    state->indexBufferPtr  = state->indexBufferBase;
}

void DebugFrameBegin(DebugState* state, OpenGL* opengl, mat4x4 viewProj)
{
    ArenaClear(&state->frameArena);
    state->viewProj  = PushStruct(&state->frameArena, mat4x4);
    *state->viewProj = viewProj;

    state->vertexBufferPtr = state->vertexBufferBase;
    state->indexBufferPtr  = state->indexBufferBase;
    state->indexCount      = 0;
    state->vertexCount     = 0;
}

void DebugFrameEnd(DebugState* state, OpenGL* opengl) { DebugFlush(state, opengl); }

void DebugDrawLine(DebugState* state, OpenGL* opengl, v3 start, v3 end, v3 color)
{
    u32 lineVertices = 2;
    u32 lineIndices  = 2;

    if (state->vertexCount + lineIndices > MAX_DEBUG_VERTICES || state->indexCount + lineIndices > MAX_DEBUG_INDICES)
    {
        DebugFlush(state, opengl);
    }

    u32 baseVertex = state->vertexCount;

    v3 points[2] = { start, end };

    for (u32 i = 0; i < lineVertices; i++)
    {
        state->vertexBufferPtr->position = points[i];
        state->vertexBufferPtr->color    = color;
        state->vertexBufferPtr++;

        state->vertexCount++;
    }

    *state->indexBufferPtr++ = baseVertex + 0;
    *state->indexBufferPtr++ = baseVertex + 1;

    state->indexCount += lineVertices;
}

void DebugDrawPlane(DebugState* state, OpenGL* opengl, v3 position, v3 color)
{
    u32 vertexCount = ArrayCount(planeVertexs);
    u32 indexCount  = ArrayCount(planeLineIndices);

    if (state->vertexCount + vertexCount > MAX_DEBUG_VERTICES || state->indexCount + indexCount > MAX_DEBUG_INDICES)
    {
        DebugFlush(state, opengl);
    }

    mat4x4 model = Translate(Identity(), position);

    u32 baseVertex = state->vertexCount;

    for (u32 vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
    {
        v4 localPos;
        localPos.x = planeVertexs[vertexIndex].x;
        localPos.y = planeVertexs[vertexIndex].y;
        localPos.z = planeVertexs[vertexIndex].z;
        localPos.w = 1.0f;

        v4 worldPos4 = model * localPos;
        v3 worldPos3;
        worldPos3.x = worldPos4.x;
        worldPos3.y = worldPos4.y;
        worldPos3.z = worldPos4.z;

        state->vertexBufferPtr->position = worldPos3;
        state->vertexBufferPtr->color    = color;
        state->vertexBufferPtr++;

        state->vertexCount++;
    }

    for (u32 index = 0; index < indexCount; index++)
    {
        *state->indexBufferPtr = planeLineIndices[index] + baseVertex;
        state->indexBufferPtr++;

        state->indexCount++;
    }
}

internal void DebugFlush(DebugState* state, OpenGL* opengl)
{
    Program* debugProgram = &state->program;

    opengl->glLineWidth(2.0f);

    GeometryBufferVBOSubdata(opengl, &state->buffer, state->vertexBufferBase, sizeof(DebugVertex) * state->vertexCount);
    GeometryBufferEBOSubdata(opengl, &state->buffer, state->indexBufferBase, sizeof(u32) * state->indexCount);

    opengl->glUseProgram(debugProgram->id);
    GLint viewProjLoc = opengl->glGetUniformLocation(debugProgram->id, "viewProj");
    opengl->glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, state->viewProj->ptr);

    opengl->glBindVertexArray(state->buffer.VAO);
    opengl->glDrawElements(GL_LINES, state->indexCount, GL_UNSIGNED_INT, 0);

    state->vertexBufferPtr = state->vertexBufferBase;
    state->indexBufferPtr  = state->indexBufferBase;
    state->vertexCount     = 0;
    state->indexCount      = 0;
}