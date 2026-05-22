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
    char   name[64];
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
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec4 color;
    u32       textureIndex;
};

struct ColorVertex
{
    glm::vec3 position;
    glm::vec4 color;
};

struct Vertex
{
    glm::vec3  position;
    glm::vec3  normal;
    glm::vec2  uv;
    glm::uvec4 joints;
    glm::vec4  weights;
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
    u8        commandQueueMemory[65536];
    OpenGL*   gl;
    Batch3D*  batch3D;
    Batch2D*  batch2D;
    glm::mat4 viewProj;
    // Batch texture queue
    TextureQueue textureQueue;
    // TODO: Move font glyphs and atlas to assets
    TTFGlyph ttfChars[TTF_GLYPH_COUNT];
    Texture  glyphAtlas;
    // TODO: Move to assets
    Texture whiteTexture;
};

// ----------------------------------------------------------------------------
// Render commands
enum RenderCommandType
{
    RenderCommandType_FramebufferClear,
    RenderCommandType_GeometryBufferDraw,
    RenderCommandType_ProgramUse,
    RenderCommandType_ProgramUploadUniform,
    RenderCommandType_BindTexture
};

struct RenderCommandHeader
{
    u32 type;
};

struct FramebufferClear
{
    glm::vec3 color;
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
        glm::mat4x4 mat4x4;
        int         integer;
        glm::vec3   vec3;
        glm::vec4   vec4;
        struct
        {
            int* integerArray;
            int  count;
        };
    };
};

struct BindTexture
{
    GLuint unit;
    GLuint id;
};
//  ----------------------------------------------------------------------------

void RendererInit(Renderer* renderer, OpenGL* opengl, PlatformAPI* platform);
void RendererFrameBegin(Renderer* renderer, glm::mat4 viewProj);
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

void DrawLine(Renderer* renderer, glm::vec3 p0, glm::vec3 p1, glm::vec4 color);
void DrawRect(Renderer* renderer, glm::vec2 position, glm::vec2 size, Texture* texture, glm::vec2 texturePosition,
              glm::vec2 textureSize);
void DrawRect(Renderer* renderer, glm::vec2 position, glm::vec2 size, glm::vec4 color);
// TODO: Font scaling
// TODO: Draw breaklines
void DrawText(Renderer* renderer, char* text, glm::vec2 position, glm::vec4 color, f32 scale = 1.0f);