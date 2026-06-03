#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include <stb_image.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define RECT_VERTEX_COUNT 4
#define RECT_INDEX_COUNT  6

internal void TextureQueueClear(Renderer* renderer);
internal u32  TextureQueueAppend(Renderer* renderer, Texture* texture);
internal void TextureQueueBind(Renderer* renderer, GLint arrayUniformLoc);
internal void TextureAlloc(Renderer* renderer, Texture* texture, void* imageBuffer, size_t size);
internal void Batch3DFlush(Renderer* renderer);
internal void Batch2DFlush(Renderer* renderer);
internal void Batch2DRect(Renderer* renderer, glm::vec2 topLeft, glm::vec2 bottomRight, Texture* texture,
                          glm::vec2 textureTopLeft, glm::vec2 textureBottomRight, glm::vec4 tintColor = color_white);

void RendererInit(Renderer* renderer, Arena* baseArena, OpenGL* gl, PlatformAPI* platform)
{
    SubArena(&renderer->arena, baseArena, Megabytes(10));
    renderer->gl       = gl;
    renderer->platform = platform;
    Arena* arena       = &renderer->arena;

    TextureQueueClear(renderer);

    // Batch3D
    {
        renderer->batch3D       = PushStruct(arena, Batch3D);
        Batch3D* batch          = renderer->batch3D;
        batch->maxVertexCount   = 32768;
        batch->vertexCount      = 0;
        batch->vertexBufferBase = PushArray(arena, batch->maxVertexCount, ColorVertex);
        batch->vertexBufferPtr  = batch->vertexBufferBase;

        size_t vertexSize = sizeof(ColorVertex);
        GPUBufferInit(renderer, &batch->buffer);
        GPUBufferVBOAlloc(renderer, &batch->buffer, 0, vertexSize * batch->maxVertexCount, vertexSize, GL_DYNAMIC_DRAW);
        GPUBufferVertexAttrib(renderer, &batch->buffer, 0, 3, GL_FLOAT, vertexSize, offsetof(ColorVertex, position));
        GPUBufferVertexAttrib(renderer, &batch->buffer, 1, 4, GL_FLOAT, vertexSize, offsetof(ColorVertex, color));

        FileReadResult debugVs = platform->FileReadEntire("../src/shaders/debug.vert");
        FileReadResult debugFs = platform->FileReadEntire("../src/shaders/debug.frag");

        ProgramInit(renderer, &batch->program);
        ProgramAttachShader(renderer, &batch->program, (char*)debugVs.content, debugVs.contentSize, GL_VERTEX_SHADER);
        ProgramAttachShader(renderer, &batch->program, (char*)debugFs.content, debugFs.contentSize, GL_FRAGMENT_SHADER);
        ProgramBuild(renderer, &batch->program);

        platform->FileFree(debugVs.content);
        platform->FileFree(debugFs.content);
    }

    // Batch2D
    {
        renderer->batch2D = PushStruct(arena, Batch2D);
        Batch2D* batch    = renderer->batch2D;

        batch->maxVertexCount   = 2048;
        batch->maxIndexCount    = batch->maxVertexCount * 6;
        batch->vertexCount      = 0;
        batch->indexCount       = 0;
        batch->vertexBufferBase = PushArray(arena, batch->maxVertexCount, BatchVertex);
        batch->indexBufferBase  = PushArray(arena, batch->maxIndexCount, u32);
        batch->vertexBufferPtr  = batch->vertexBufferBase;
        batch->indexBufferPtr   = batch->indexBufferBase;

        Program* program = &batch->program;

        FileReadResult vsFile = platform->FileReadEntire("../src/shaders/batch.vert");
        FileReadResult fsFile = platform->FileReadEntire("../src/shaders/batch.frag");

        ProgramInit(renderer, program);
        ProgramAttachShader(renderer, program, (char*)vsFile.content, vsFile.contentSize, GL_VERTEX_SHADER);
        ProgramAttachShader(renderer, program, (char*)fsFile.content, fsFile.contentSize, GL_FRAGMENT_SHADER);
        ProgramBuild(renderer, program);

        platform->FileFree(vsFile.content);
        platform->FileFree(fsFile.content);

        size_t vertexSize = sizeof(BatchVertex);

        GPUBuffer* buffer = &batch->buffer;

        GPUBufferInit(renderer, buffer);
        GPUBufferVBOAlloc(renderer, buffer, 0, vertexSize * batch->maxVertexCount, vertexSize, GL_DYNAMIC_DRAW);
        GPUBufferEBOAlloc(renderer, buffer, 0, sizeof(u32) * batch->maxIndexCount, sizeof(u32), GL_DYNAMIC_DRAW);
        GPUBufferVertexAttrib(renderer, buffer, 0, 3, GL_FLOAT, vertexSize, offsetof(BatchVertex, position));
        GPUBufferVertexAttrib(renderer, buffer, 1, 2, GL_FLOAT, vertexSize, offsetof(BatchVertex, uv));
        GPUBufferVertexAttrib(renderer, buffer, 2, 4, GL_FLOAT, vertexSize, offsetof(BatchVertex, color));
        GPUBufferVertexAttrib(renderer, buffer, 3, 1, GL_INT, vertexSize, offsetof(BatchVertex, textureIndex));

        u32 pixels = 0xFFFFFFFF;
        gl->GenTextures(1, &renderer->whiteTexture.id);
        gl->BindTexture(GL_TEXTURE_2D, renderer->whiteTexture.id);
        gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels);
    }
}

void RendererFrameBegin(Renderer* renderer, glm::mat4 viewProj)
{
    RenderCommandQueue* queue = &renderer->commandQueue;

    queue->pushBufferBase = renderer->commandQueueMemory;
    queue->pushBufferPtr  = queue->pushBufferBase;
    queue->pushBufferSize = sizeof(renderer->commandQueueMemory);

    renderer->viewProj = viewProj;
}

void RendererFrameEnd(Renderer* renderer)
{
    RenderCommandQueue* queue    = &renderer->commandQueue;
    OpenGL*             gl       = renderer->gl;
    PlatformAPI*        platform = renderer->platform;

    gl->Enable(GL_DEPTH_TEST);

    // gl->PolygonMode(GL_FRONT_AND_BACK , GL_LINE);
    // gl->PolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    gl->Enable(GL_BLEND);
    //gl->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    //gl->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_COLOR);  
    

#pragma warning(push)
#pragma warning(disable : 4456)
    for (u8* command = queue->pushBufferBase; command < queue->pushBufferPtr; /**/)
    {
        RenderCommandHeader* header = (RenderCommandHeader*)command;
        command += sizeof(RenderCommandHeader);
        void* payload = (u8*)header + sizeof(*header);

        switch (header->type)
        {
        case RenderCommandType_FramebufferClear:
        {
            command += sizeof(FramebufferClear);
            FramebufferClear* command = (FramebufferClear*)payload;

            gl->ClearColor(command->color.x, command->color.y, command->color.z, 1.0f);
            gl->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            break;
        }
        case RenderCommandType_GeometryBufferDraw:
        {
            command += sizeof(GeometryBufferDraw);
            GeometryBufferDraw* command = (GeometryBufferDraw*)payload;
            GPUBuffer*          buffer  = &command->buffer;

            gl->BindVertexArray(buffer->VAO);

            // Indexed
            if (buffer->indexCount > 0)
            {
                gl->DrawElements(command->primitive, buffer->indexCount, GL_UNSIGNED_INT, 0);
            }
            else
            {
                gl->DrawArrays(command->primitive, 0, buffer->vertexCount);
            }
            break;
        }
        case RenderCommandType_ProgramUse:
        {
            command += sizeof(ProgramUse);
            ProgramUse* command = (ProgramUse*)payload;

            gl->UseProgram(command->program.id);
            break;
        }
        case RenderCommandType_ProgramUploadUniform:
        {
            command += sizeof(ProgramUploadUniform);
            ProgramUploadUniform* command = (ProgramUploadUniform*)payload;

            GLint loc = gl->GetUniformLocation(command->programId, command->name);
            if (loc != -1)
            {
                switch (command->type)
                {
                case UniformType_Mat4x4:
                {
                    gl->UniformMatrix4fv(loc, 1, GL_FALSE, &command->mat4x4[0][0]);
                    break;
                }
                case UniformType_Int:
                {
                    gl->Uniform1i(loc, command->integer);
                    break;
                }
                case UniformType_IntArray:
                {
                    gl->Uniform1iv(loc, command->count, command->integerArray);
                    break;
                }
                case UniformType_Vec3:
                {
                    gl->Uniform3fv(loc, 1, &command->vec3.x);
                    break;
                }
                case UniformType_Vec4:
                {
                    gl->Uniform4fv(loc, 1, &command->vec4.x);
                    break;
                }
                    InvalidDefaultCase;
                }
            }
            else
            {
                platform->Logf("Uniform '%s' not found in program '%d'", command->name, command->programId);
            }

            break;
        }
        case RenderCommandType_BindTexture:
        {
            command += sizeof(BindTexture);
            BindTexture* command = (BindTexture*)payload;

            gl->ActiveTexture(GL_TEXTURE0 + command->unit);
            gl->BindTexture(GL_TEXTURE_2D, command->id);

            break;
        }
            InvalidDefaultCase;
        }
    }
#pragma warning(pop)

    // Batch3D
    if (renderer->batch3D->vertexCount > 0)
    {
        Batch3DFlush(renderer);
    }

    // Batch2D
    if (renderer->batch2D->vertexCount > 0 || renderer->batch2D->indexCount > 0)
    {
        Batch2DFlush(renderer);
    }
}

#define PushRenderCommand(queue, type) (type*)PushRenderCommand_(queue, sizeof(type), RenderCommandType_##type)
inline void* PushRenderCommand_(RenderCommandQueue* queue, size_t size, RenderCommandType type)
{
    void*  result    = { 0 }; // Command struct
    size_t totalSize = size + sizeof(RenderCommandHeader);

    u8* pushBufferEnd = queue->pushBufferBase + queue->pushBufferSize;
    if ((queue->pushBufferPtr + totalSize) <= pushBufferEnd)
    {
        RenderCommandHeader* commandHeader = (RenderCommandHeader*)queue->pushBufferPtr;
        queue->pushBufferPtr += totalSize;

        commandHeader->type = type;
        result              = (u8*)commandHeader + sizeof(*commandHeader);
    }
    else
    {
        Log("RenderCommandQueue run out of space");
        InvalidCodePath;
    }

    return result;
}

inline void PushRenderProgramUse(Renderer* renderer, GLuint programId)
{
    ProgramUse* command = PushRenderCommand(&renderer->commandQueue, ProgramUse);
    command->program.id = programId;
}

inline void PushRenderUploadUniformMat4x4(Renderer* renderer, GLuint programId, char* name, glm::mat4 mat4)
{
    ProgramUploadUniform* command = PushRenderCommand(&renderer->commandQueue, ProgramUploadUniform);
    sprintf(command->name, "%s", name);
    command->programId = programId;
    command->mat4x4    = mat4;
    command->type      = UniformType_Mat4x4;
}

inline void PushRenderUploadUniformInt(Renderer* renderer, GLuint programId, char* name, int integer)
{
    ProgramUploadUniform* command = PushRenderCommand(&renderer->commandQueue, ProgramUploadUniform);
    sprintf(command->name, "%s", name);
    command->programId = programId;
    command->integer   = integer;
    command->type      = UniformType_Int;
}

inline void PushRenderUploadUniformIntArray(Renderer* renderer, GLuint programId, char* name, int* array, int count)
{
    ProgramUploadUniform* command = PushRenderCommand(&renderer->commandQueue, ProgramUploadUniform);
    sprintf(command->name, "%s", name);
    command->programId    = programId;
    command->integerArray = array;
    command->count        = count;
    command->type         = UniformType_IntArray;
}

inline void PushRenderUploadUniformVec3(Renderer* renderer, GLuint programId, char* name, glm::vec3 vec3)
{
    ProgramUploadUniform* command = PushRenderCommand(&renderer->commandQueue, ProgramUploadUniform);
    sprintf(command->name, "%s", name);
    command->programId = programId;
    command->vec3      = vec3;
    command->type      = UniformType_Vec3;
}

inline void PushRenderUploadUniformVec4(Renderer* renderer, GLuint programId, char* name, glm::vec4 vec4)
{
    ProgramUploadUniform* command = PushRenderCommand(&renderer->commandQueue, ProgramUploadUniform);
    sprintf(command->name, "%s", name);
    command->programId = programId;
    command->vec4      = vec4;
    command->type      = UniformType_Vec4;
}

inline void PushRenderDrawBuffer(Renderer* renderer, GPUBuffer* buffer, GLenum primitive = GL_TRIANGLES)
{
    GeometryBufferDraw* command = PushRenderCommand(&renderer->commandQueue, GeometryBufferDraw);
    command->buffer             = *buffer;
    command->primitive          = primitive;
}

inline void PushRenderBindTexture(Renderer* renderer, Texture* texture, GLuint unit)
{
    BindTexture* command = PushRenderCommand(&renderer->commandQueue, BindTexture);
    command->id          = texture->id;
    command->unit        = unit;
}

internal void Batch3DFlush(Renderer* renderer)
{
    Batch3D* batch       = renderer->batch3D;
    OpenGL*  gl          = renderer->gl;
    GLint    viewProjLoc = gl->GetUniformLocation(batch->program.id, "viewProj");

    GPUBufferVBOSubdata(renderer, &batch->buffer, batch->vertexBufferBase, sizeof(ColorVertex) * batch->vertexCount);

    gl->LineWidth(2.0f);
    gl->Enable(GL_DEPTH_TEST);
    gl->UseProgram(batch->program.id);
    gl->UniformMatrix4fv(viewProjLoc, 1, GL_FALSE, &renderer->viewProj[0][0]);
    gl->BindVertexArray(batch->buffer.VAO);
    gl->DrawArrays(GL_LINES, 0, batch->vertexCount);

    batch->vertexBufferPtr = batch->vertexBufferBase;
    batch->vertexCount     = 0;
}

void DrawLine(Renderer* renderer, glm::vec3 p0, glm::vec3 p1, glm::vec4 color)
{
    u32 lineVertices = 2;

    Batch3D* batch3D = renderer->batch3D;
    if (batch3D->vertexCount + lineVertices > batch3D->maxVertexCount)
    {
        Batch3DFlush(renderer);
    }

    // Start
    batch3D->vertexBufferPtr->position = p0;
    batch3D->vertexBufferPtr->color    = color;
    batch3D->vertexBufferPtr++;
    // End
    batch3D->vertexBufferPtr->position = p1;
    batch3D->vertexBufferPtr->color    = color;
    batch3D->vertexBufferPtr++;

    batch3D->vertexCount += lineVertices;
}

internal void Batch2DFlush(Renderer* renderer)
{
    Batch2D*   batch     = renderer->batch2D;
    OpenGL*    gl        = renderer->gl;
    glm::uvec2 windowDim = renderer->platform->WindowGetDimension();
    glm::mat4  view2D    = glm::ortho(0.0f, (f32)windowDim.x, (f32)windowDim.y, 0.0f);

    GPUBufferVBOSubdata(renderer, &batch->buffer, batch->vertexBufferBase, sizeof(BatchVertex) * batch->vertexCount);
    GPUBufferEBOSubdata(renderer, &batch->buffer, batch->indexBufferBase, sizeof(u32) * batch->indexCount);

    GLint viewProjLoc     = gl->GetUniformLocation(batch->program.id, "viewProj");
    GLint textureArrayLoc = gl->GetUniformLocation(batch->program.id, "textureArray");

    gl->UseProgram(batch->program.id);
    gl->Disable(GL_DEPTH_TEST);
    gl->Enable(GL_BLEND);
    gl->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    TextureQueueBind(renderer, textureArrayLoc);
    gl->UniformMatrix4fv(viewProjLoc, 1, GL_FALSE, &view2D[0][0]);
    gl->BindVertexArray(batch->buffer.VAO);
    gl->DrawElements(GL_TRIANGLES, batch->indexCount, GL_UNSIGNED_INT, 0);

    batch->vertexBufferPtr = batch->vertexBufferBase;
    batch->indexBufferPtr  = batch->indexBufferBase;
    batch->vertexCount     = 0;
    batch->indexCount      = 0;
    TextureQueueClear(renderer);
}

internal void Batch2DRect(Renderer* renderer, glm::vec2 position, glm::vec2 size, Texture* texture,
                          glm::vec2 texturePosition, glm::vec2 textureSize, glm::vec4 tintColor)
{
    Batch2D* batch = renderer->batch2D;

    if ((batch->vertexCount + RECT_VERTEX_COUNT > batch->maxVertexCount) ||
        (batch->indexCount + RECT_INDEX_COUNT > batch->maxIndexCount) ||
        renderer->textureQueue.count == MAX_TEXTURE_COUNT)
    {
        Batch2DFlush(renderer);
    }

    u32 textureIndex = 0;
    f32 textureW     = 1.0f;
    f32 textureH     = 1.0f;
    f32 textureX     = 1.0f;
    f32 textureY     = 1.0f;

    if (texture)
    {
        textureIndex = TextureQueueAppend(renderer, texture);
        textureW     = (1.0f / texture->width) * textureSize.x;
        textureH     = (1.0f / texture->height) * textureSize.y;
        textureX     = (1.0f / texture->width) * texturePosition.x;
        textureY     = (1.0f / texture->height) * texturePosition.y;
    }

    u32 rectIndices[] = { 0, 1, 2, 0, 2, 3 };
    for (u32 index = 0; index < RECT_INDEX_COUNT; index++)
    {
        *batch->indexBufferPtr = rectIndices[index] + batch->vertexCount;
        batch->indexBufferPtr++;

        batch->indexCount++;
    }

    // Top-right
    batch->vertexBufferPtr->position     = { position.x + size.x, position.y, 0.0f };
    batch->vertexBufferPtr->uv           = { textureX + textureW, textureY };
    batch->vertexBufferPtr->color        = tintColor;
    batch->vertexBufferPtr->textureIndex = textureIndex;
    batch->vertexBufferPtr++;
    // Bottom-right
    batch->vertexBufferPtr->position     = { position.x + size.x, position.y + size.y, 0.0f };
    batch->vertexBufferPtr->uv           = { textureX + textureW, textureY + textureH };
    batch->vertexBufferPtr->color        = tintColor;
    batch->vertexBufferPtr->textureIndex = textureIndex;
    batch->vertexBufferPtr++;
    // Bottom-left
    batch->vertexBufferPtr->position     = { position.x, position.y + size.y, 0.0f };
    batch->vertexBufferPtr->uv           = { textureX, textureY + textureH };
    batch->vertexBufferPtr->color        = tintColor;
    batch->vertexBufferPtr->textureIndex = textureIndex;
    batch->vertexBufferPtr++;
    // Top-left
    batch->vertexBufferPtr->position     = { position.x, position.y, 0.0f };
    batch->vertexBufferPtr->uv           = { textureX, textureY };
    batch->vertexBufferPtr->color        = tintColor;
    batch->vertexBufferPtr->textureIndex = textureIndex;
    batch->vertexBufferPtr++;

    batch->vertexCount += 4;
}

void DrawRect(Renderer* renderer, glm::vec2 position, glm::vec2 size, Texture* texture, glm::vec2 texturePosition,
              glm::vec2 textureSize)
{
    Batch2DRect(renderer, position, size, texture, texturePosition, textureSize);
}

void DrawRect(Renderer* renderer, glm::vec2 position, glm::vec2 size, glm::vec4 color)
{
    glm::vec2 vec2Zero{ 0.0f, 0.0f };
    Batch2DRect(renderer, position, size, 0, vec2Zero, vec2Zero, color);
}

void DrawText(Renderer* renderer, char* text, glm::vec2 position, glm::vec4 color, f32 scale)
{
    Batch2D* batch = renderer->batch2D;

    size_t textLength      = strlen(text);
    u32    textVertexCount = (u32)textLength * RECT_VERTEX_COUNT;
    u32    textIndexCount  = (u32)textLength * RECT_INDEX_COUNT;

    if ((batch->vertexCount + textVertexCount > batch->maxVertexCount) ||
        (batch->indexCount + textIndexCount > batch->maxIndexCount))
    {
        Assert(0);
    }

    u16   baseline = 0;
    char* textPtr  = text;
    while (*text)
    {
        TTFGlyph* ttfChar = &renderer->ttfChars[*text++ - TTF_FIRST_GLYPH_OFFSET];
        baseline          = Max(baseline, ttfChar->y1 - ttfChar->y0);
    }
    text = textPtr;
    position.y += baseline * scale;

    glm::vec2 glyphPos{ position.x, 0.0f };
    while (*text)
    {
        TTFGlyph* ttfChar = &renderer->ttfChars[*text++ - TTF_FIRST_GLYPH_OFFSET];

        glyphPos.x += (ttfChar->xoff * scale);
        glyphPos.y = position.y + (ttfChar->yoff * scale);
        glm::vec2 glyphSize{ ((f32)ttfChar->x1 - (f32)ttfChar->x0) * scale,
                             ((f32)ttfChar->y1 - (f32)ttfChar->y0) * scale };

        glm::vec2 textureSize{ (f32)ttfChar->x0 + ttfChar->s0, (f32)ttfChar->y0 + ttfChar->t0 };
        glm::vec2 texturePosition{ ((f32)ttfChar->x1 - (f32)ttfChar->x0) - ttfChar->s1,
                                   ((f32)ttfChar->y1 - (f32)ttfChar->y0) - ttfChar->t1 };

        Batch2DRect(renderer, glyphPos, glyphSize, &renderer->glyphAtlas, textureSize, texturePosition, color);
        glyphPos.x += (ttfChar->xadvance * scale);
    }
}

void RendererTTFLoad(Renderer* renderer, char* filename)
{
    PlatformAPI* platform = renderer->platform;
    Arena*       arena    = &renderer->arena;
    OpenGL*      gl       = renderer->gl;

    FileReadResult fontFile = platform->FileReadEntire(filename);
    if (fontFile.contentSize > 0)
    {
        stbtt_fontinfo fontInfo   = { 0 };
        u8*            fontBuffer = (u8*)fontFile.content;

        if (stbtt_InitFont(&fontInfo, fontBuffer, 0))
        {
            int fontAtlasWidth  = 1024;
            int fontAtlasHeight = 1024;
            f32 fontSize        = 64.0f;

            TemporaryMemory tempMemory = TemporaryMemoryBegin(arena);
            {
                u8* bitmapFontBuffer = PushArray(arena, fontAtlasWidth * fontAtlasHeight, u8);

                stbtt_pack_context packCtx;
                stbtt_packedchar   packedChars[TTF_GLYPH_COUNT];

                stbtt_PackBegin(&packCtx, bitmapFontBuffer, fontAtlasWidth, fontAtlasHeight, 0, 1, 0);
                stbtt_PackFontRange(&packCtx, fontBuffer, 0, fontSize, TTF_FIRST_GLYPH_OFFSET, TTF_GLYPH_COUNT,
                                    packedChars);
                stbtt_PackEnd(&packCtx);

                for (u32 charIndex = 0; charIndex < TTF_GLYPH_COUNT; charIndex++)
                {
                    f32 x, y;

                    stbtt_aligned_quad alignedQuad;
                    stbtt_GetPackedQuad(packedChars, fontAtlasWidth, fontAtlasHeight, (int)charIndex, &x, &y,
                                        &alignedQuad, 0);

                    TTFGlyph* ttfChar = &renderer->ttfChars[charIndex];
                    ttfChar->x0       = packedChars[charIndex].x0;
                    ttfChar->y0       = packedChars[charIndex].y0;
                    ttfChar->x1       = packedChars[charIndex].x1;
                    ttfChar->y1       = packedChars[charIndex].y1;
                    ttfChar->xoff     = packedChars[charIndex].xoff;
                    ttfChar->yoff     = packedChars[charIndex].yoff;
                    ttfChar->xadvance = packedChars[charIndex].xadvance;
                    ttfChar->s0       = alignedQuad.s0;
                    ttfChar->t0       = alignedQuad.t0;
                    ttfChar->s1       = alignedQuad.s1;
                    ttfChar->t1       = alignedQuad.t1;
                }

                platform->FileFree(fontFile.content);

                gl->GenTextures(1, &renderer->glyphAtlas.id);
                gl->BindTexture(GL_TEXTURE_2D, renderer->glyphAtlas.id);
                gl->TexImage2D(GL_TEXTURE_2D, 0, GL_R8, (GLsizei)fontAtlasWidth, (GLsizei)fontAtlasHeight, 0, GL_RED,
                               GL_UNSIGNED_BYTE, (void*)bitmapFontBuffer);
                GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
                gl->TexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
                gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                renderer->glyphAtlas.width  = (u32)fontAtlasWidth;
                renderer->glyphAtlas.height = (u32)fontAtlasHeight;
            }
            TemporaryMemoryEnd(tempMemory);
        }
        else
        {
            platform->Logf("Unable to init .ttf font");
            Assert(0);
        }
    }
    else
    {
        platform->Logf("Unable to load font: '%s'", filename);
        Assert(0);
    }
}

void GPUBufferInit(Renderer* renderer, GPUBuffer* buffer)
{
    renderer->gl->GenVertexArrays(1, &buffer->VAO);

    buffer->VBO         = 0;
    buffer->EBO         = 0;
    buffer->vertexCount = 0;
    buffer->indexCount  = 0;
}

void GPUBufferVBOAlloc(Renderer* renderer, GPUBuffer* buffer, void* data, size_t size, size_t vertexSize, GLenum usage)
{
    OpenGL* gl          = renderer->gl;
    buffer->vertexCount = (u32)(size / vertexSize);
    gl->GenBuffers(1, &buffer->VBO);
    gl->BindVertexArray(buffer->VAO);
    gl->BindBuffer(GL_ARRAY_BUFFER, buffer->VBO);
    gl->BufferData(GL_ARRAY_BUFFER, (GLsizei)size, data, usage);
}

void GPUBufferVBOSubdata(Renderer* renderer, GPUBuffer* buffer, void* data, size_t size)
{
    OpenGL* gl = renderer->gl;
    gl->BindVertexArray(buffer->VAO);
    gl->BindBuffer(GL_ARRAY_BUFFER, buffer->VBO);
    gl->BufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

void GPUBufferEBOAlloc(Renderer* renderer, GPUBuffer* buffer, void* data, size_t size, size_t indexSize, GLenum usage)
{
    OpenGL* gl         = renderer->gl;
    buffer->indexCount = (u32)(size / indexSize);
    gl->GenBuffers(1, &buffer->EBO);
    gl->BindVertexArray(buffer->VAO);
    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->EBO);
    gl->BufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei)size, data, usage);
}

void GPUBufferEBOSubdata(Renderer* renderer, GPUBuffer* buffer, void* data, size_t size)
{
    OpenGL* gl = renderer->gl;
    gl->BindVertexArray(buffer->VAO);
    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->EBO);
    gl->BufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size, data);
}

void GPUBufferVertexAttrib(Renderer* renderer, GPUBuffer* buffer, u32 index, u32 componentCount, GLenum type,
                           size_t stride, size_t offset)
{
    OpenGL* gl = renderer->gl;
    gl->BindVertexArray(buffer->VAO);
    if (type == GL_BYTE || type == GL_UNSIGNED_BYTE || type == GL_SHORT || type == GL_UNSIGNED_SHORT ||
        type == GL_INT || type == GL_UNSIGNED_INT)
    {
        gl->VertexAttribIPointer((GLuint)index, (GLint)componentCount, type, (GLsizei)stride, (void*)offset);
    }
    else
    {
        gl->VertexAttribPointer((GLuint)index, (GLint)componentCount, type, false, (GLsizei)stride, (void*)offset);
    }
    gl->EnableVertexAttribArray((GLuint)index);
}

void ProgramInit(Renderer* renderer, Program* program) { program->id = renderer->gl->CreateProgram(); }

void ProgramAttachShader(Renderer* renderer, Program* program, char* source, size_t length, GLenum type)
{
    OpenGL*      gl       = renderer->gl;
    PlatformAPI* platform = renderer->platform;

    GLuint shader     = gl->CreateShader(type);
    GLint  sourceSize = (GLint)length;
    gl->ShaderSource(shader, 1, &source, &sourceSize);
    gl->CompileShader(shader);

    GLint ok;
    gl->GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char infoBuffer[512];
        gl->GetShaderInfoLog(shader, sizeof(infoBuffer), NULL, infoBuffer);
        platform->Logf("OpenGL compiling shader: '%s'", infoBuffer);
        Assert(0);
    }

    gl->AttachShader(program->id, shader);
}

void ProgramBuild(Renderer* renderer, Program* program)
{
    OpenGL*      gl       = renderer->gl;
    PlatformAPI* platform = renderer->platform;

    gl->LinkProgram(program->id);

    GLint ok;
    gl->GetProgramiv(program->id, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char infoBuffer[512];
        gl->GetProgramInfoLog(program->id, sizeof(infoBuffer), NULL, infoBuffer);
        platform->Logf("OpenGL linking program: '%s'", infoBuffer);
        Assert(0);
    }
}

void TextureInit(Renderer* renderer, Texture* texture, char* filename)
{
    PlatformAPI* platform = renderer->platform;

    FileReadResult file = platform->FileReadEntire(filename);
    if (file.content)
    {
        TextureAlloc(renderer, texture, file.content, file.contentSize);
        platform->FileFree(file.content);
    }
    else
    {
        platform->Logf("Unable to load texture: '%s'", filename);
        Assert(0);
    }
}

internal void TextureAlloc(Renderer* renderer, Texture* texture, void* imageBuffer, size_t size)
{
    OpenGL* gl = renderer->gl;

    int width;
    int height;
    int numChannels;

    void* pixels = stbi_load_from_memory((u8*)imageBuffer, (int)size, &width, &height, &numChannels, 0);
    if (pixels)
    {
        texture->width  = (u32)width;
        texture->height = (u32)height;

        GLint  internalFormat = 0;
        GLenum format         = 0;
        // TODO: Handle different component types
        GLenum type = GL_UNSIGNED_BYTE;

        switch (numChannels)
        {
        case 1:
        {
            internalFormat = format = GL_RED;
            break;
        }
        case 2:
        {
            internalFormat = format = GL_RG;
            break;
        }
        case 3:
        {
            internalFormat = format = GL_RGB;
            break;
        }
        case 4:
        {
            internalFormat = format = GL_RGBA;
            break;
        }
            InvalidDefaultCase;
        }

        gl->GenTextures(1, &texture->id);
        gl->BindTexture(GL_TEXTURE_2D, texture->id);
        gl->TexImage2D(GL_TEXTURE_2D, 0, internalFormat, (GLsizei)width, (GLsizei)height, 0, format, type, pixels);
        gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        Assert(0);
    }
}

internal void TextureQueueClear(Renderer* renderer)
{
    TextureQueue* queue = &renderer->textureQueue;

    memset(queue->ids, INVALID_TEXTURE, sizeof(u32) * ArrayCount(queue->ids));
    queue->count  = 1;
    queue->ids[0] = renderer->whiteTexture.id;
}

internal u32 TextureQueueAppend(Renderer* renderer, Texture* texture)
{
    TextureQueue* queue = &renderer->textureQueue;

    // Queue texture
    u32 textureIndex = 0;
    for (u32 queueTextureIndex = 0; queueTextureIndex < ArrayCount(queue->ids); queueTextureIndex++)
    {
        // Already queued
        if (queue->ids[queueTextureIndex] == texture->id)
        {
            textureIndex = queueTextureIndex;
            break;
        }
        // Empty slot
        else if (queue->ids[queueTextureIndex] == INVALID_TEXTURE)
        {
            textureIndex = queueTextureIndex;
            queue->count++;
            break;
        }
    }
    queue->ids[textureIndex] = texture->id;

    return textureIndex;
}

internal void TextureQueueBind(Renderer* renderer, GLint arrayUniformLoc)
{
    Assert(arrayUniformLoc != -1);

    OpenGL*       gl                              = renderer->gl;
    TextureQueue* queue                           = &renderer->textureQueue;
    int           textureArray[MAX_TEXTURE_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

    gl->Uniform1iv(arrayUniformLoc, queue->count, textureArray);
    for (u32 textureIndex = 0; textureIndex < queue->count; textureIndex++)
    {
        gl->ActiveTexture(GL_TEXTURE0 + textureIndex);
        gl->BindTexture(GL_TEXTURE_2D, queue->ids[textureIndex]);
    }
}