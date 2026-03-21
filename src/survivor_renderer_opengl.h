#pragma once

#define MAX_TEXTURE_COUNT      16
#define INVALID_TEXTURE        0xFFFFFFFF
#define TTF_FIRST_GLYPH_OFFSET 32 // Space ascii code
#define TTF_GLYPH_COUNT        95

#ifdef DrawText
#undef DrawText
#endif

struct RenderCommandQueue
{
    u8* pushBufferBase;
    u8* pushBufferPtr;
    u32 pushBufferSize;
};

struct Program
{
    GLuint id;
};

struct Texture
{
    GLuint id;
    u32    width;
    u32    height;
};

struct GPUBuffer
{
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    u32    vertexCount;
    u32    indexCount;
};

struct BatchVertex
{
    v3  position;
    v2  uv;
    v4  color;
    u32 textureIndex;
};

struct ColorVertex
{
    v3 position;
    v4 color;
};

struct Batch2D
{
    GPUBuffer    buffer;
    Program      program;
    BatchVertex* vertexBufferBase;
    BatchVertex* vertexBufferPtr;
    u32*         indexBufferBase;
    u32*         indexBufferPtr;
    u32          vertexCount;
    u32          indexCount;
    u32          maxVertexCount;
    u32          maxIndexCount;
};

struct Batch3D
{
    Program      program;
    GPUBuffer    buffer;
    ColorVertex* vertexBufferBase;
    ColorVertex* vertexBufferPtr;
    u32          vertexCount;
    u32          maxVertexCount;
};

struct TTFGlyph
{
    u16 x0, y0, x1, y1; // Bounding-box
    f32 xoff, yoff, xadvance;
    f32 s0, t0, s1, t1; // Texture coordinates, relative to bounding-box.
};

struct TextureQueue
{
    GLuint ids[MAX_TEXTURE_COUNT];
    u32    count;
};

struct Renderer
{
    Arena              arena;
    PlatformAPI*       platform;
    RenderCommandQueue commandQueue;
    // TODO: Use arena
    u8           commandQueueMemory[65536];
    OpenGL*      gl;
    Batch3D*     batch3D;
    Batch2D*     batch2D;
    mat4x4       viewProj;
    TextureQueue textureQueue;
    // TODO: Rethink who owns the font atlas
    TTFGlyph ttfChars[TTF_GLYPH_COUNT];
    Texture  glyphAtlas;
    Texture  whiteTexture;
};

// ----------------------------------------------------------------------------
// Render commands
enum RenderCommandType
{
    RenderCommandType_FramebufferClear,
    RenderCommandType_GeometryBufferDraw,
    RenderCommandType_ProgramUse,
    RenderCommandType_ProgramUploadUniform
};

struct RenderCommandHeader
{
    u32 type;
};

struct FramebufferClear
{
    v3 color;
};

struct GeometryBufferDraw
{
    GPUBuffer buffer;
    GLenum    primitive;
};

struct ProgramUse
{
    Program program;
};

enum UniformType
{
    UniformType_Mat4x4,
    UniformType_Int,
    UniformType_IntArray,
    UniformType_Vec3,
    UniformType_Vec4
};

struct ProgramUploadUniform
{
    GLuint      programId;
    char        name[32];
    UniformType type;
    union
    {
        mat4x4 mat4x4;
        int    integer;
        v3     vec3;
        v4     vec4;
        struct
        {
            int* integerArray;
            int  count;
        };
    };
};
//  ----------------------------------------------------------------------------

void RendererInit(Renderer* renderer, OpenGL* opengl, PlatformAPI* platform);
void RendererFrameBegin(Renderer* renderer, mat4x4 viewProj);
void RendererFrameEnd(Renderer* renderer);
void RendererTTFLoad(Renderer* renderer, char* filename);

void GPUBufferInit(Renderer* renderer, GPUBuffer* buffer);
void GPUBufferVBOAlloc(Renderer* renderer, GPUBuffer* buffer, void* data, size_t size, size_t vertexSize, GLenum usage);
void GPUBufferVBOSubdata(Renderer* renderer, GPUBuffer* buffer, void* data, size_t size);
void GPUBufferEBOAlloc(Renderer* renderer, GPUBuffer* buffer, void* data, size_t size, size_t indexSize, GLenum usage);
void GPUBufferEBOSubdata(Renderer* renderer, GPUBuffer* buffer, void* data, size_t size);
void GPUBufferVertexAttrib(Renderer* renderer, GPUBuffer* buffer, u32 index, u32 componentCount, GLenum type,
                           size_t stride, size_t offset);

void ProgramInit(Renderer* renderer, Program* program);
void ProgramAttachShader(Renderer* renderer, Program* program, char* source, size_t length, GLenum type);
void ProgramBuild(Renderer* renderer, Program* program);

void TextureInit(Renderer* renderer, Texture* texture, char* filename);

void DrawLine(Renderer* renderer, v3 p0, v3 p1, v4 color);
void DrawRect(Renderer* renderer, v2 position, v2 size, Texture* texture, v2 texturePosition, v2 textureSize);
// TODO: Font scaling
void DrawText(Renderer* renderer, char* text, v2 position, v4 color, f32 scale = 1.0f);