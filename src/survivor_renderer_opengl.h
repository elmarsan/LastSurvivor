#pragma once

#include <gl/glcorearb.h>

struct RenderCommandQueue
{
    u8* pushBufferBase;
    u8* pushBufferPtr;
    u32 pushBufferSize;
};

struct OpenGL
{
    RenderCommandQueue commandQueue;
    u8                 commandQueueBufferMemory[65536];

    PFNGLENABLEPROC                  glEnable;
    PFNGLDISABLEPROC                 glDisable;
    PFNGLCLEARCOLORPROC              glClearColor;
    PFNGLCLEARPROC                   glClear;
    PFNGLDRAWARRAYSPROC              glDrawArrays;
    PFNGLDRAWELEMENTSPROC            glDrawElements;
    PFNGLLINEWIDTHPROC               glLineWidth;
    PFNGLGENTEXTURESPROC             glGenTextures;
    PFNGLBINDTEXTUREPROC             glBindTexture;
    PFNGLTEXIMAGE2DPROC              glTexImage2D;
    PFNGLTEXPARAMETERIPROC           glTexParameteri;
    PFNGLTEXPARAMETERIVPROC          glTexParameteriv;
    PFNGLBLENDFUNCPROC               glBlendFunc;
    PFNGLCREATEPROGRAMPROC           glCreateProgram;
    PFNGLCREATESHADERPROC            glCreateShader;
    PFNGLATTACHSHADERPROC            glAttachShader;
    PFNGLDELETESHADERPROC            glDeleteShader;
    PFNGLLINKPROGRAMPROC             glLinkProgram;
    PFNGLDELETEPROGRAMPROC           glDeleteProgram;
    PFNGLSHADERSOURCEPROC            glShaderSource;
    PFNGLUSEPROGRAMPROC              glUseProgram;
    PFNGLGETSHADERIVPROC             glGetShaderiv;
    PFNGLGETSHADERINFOLOGPROC        glGetShaderInfoLog;
    PFNGLCOMPILESHADERPROC           glCompileShader;
    PFNGLGETPROGRAMIVPROC            glGetProgramiv;
    PFNGLGETPROGRAMINFOLOGPROC       glGetProgramInfoLog;
    PFNGLGENBUFFERSPROC              glGenBuffers;
    PFNGLGENVERTEXARRAYSPROC         glGenVertexArrays;
    PFNGLBINDBUFFERPROC              glBindBuffer;
    PFNGLBINDVERTEXARRAYPROC         glBindVertexArray;
    PFNGLBUFFERDATAPROC              glBufferData;
    PFNGLBUFFERSUBDATAPROC           glBufferSubData;
    PFNGLDELETEBUFFERSPROC           glDeleteBuffers;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
    PFNGLVERTEXATTRIBPOINTERPROC     glVertexAttribPointer;
    PFNGLVERTEXATTRIBIPOINTERPROC    glVertexAttribIPointer;
    PFNGLDELETEVERTEXARRAYSPROC      glDeleteVertexArrays;
    PFNGLACTIVETEXTUREPROC           glActiveTexture;
    PFNGLGENERATEMIPMAPPROC          glGenerateMipmap;
    PFNGLGETUNIFORMLOCATIONPROC      glGetUniformLocation;
    PFNGLUNIFORMMATRIX4FVPROC        glUniformMatrix4fv;
    PFNGLUNIFORM1IPROC               glUniform1i;
    PFNGLUNIFORM1UIPROC              glUniform1ui;
    PFNGLUNIFORM1FVPROC              glUniform1fv;
    PFNGLUNIFORM3FVPROC              glUniform3fv;
    PFNGLUNIFORM4FVPROC              glUniform4fv;
    PFNGLUNIFORM1IVPROC              glUniform1iv;
};

struct GeometryBuffer
{
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    GLenum primitive;
    u32    vertexCount;
    u32    indexCount;
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
    GeometryBuffer buffer;
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
// ----------------------------------------------------------------------------

RenderCommandQueue* RendererFrameBegin(OpenGL* opengl);
void                RendererFrameEnd(OpenGL* opengl);

void GeometryBufferInit(OpenGL* opengl, GeometryBuffer* buffer, GLenum primitive);
void GeometryBufferVBOAlloc(OpenGL* opengl, GeometryBuffer* buffer, void* data, size_t size, size_t vertexSize,
                            GLenum usage);
void GeometryBufferVBOSubdata(OpenGL* opengl, GeometryBuffer* buffer, void* data, size_t size);
void GeometryBufferEBOAlloc(OpenGL* opengl, GeometryBuffer* buffer, void* data, size_t size, size_t indexSize,
                            GLenum usage);
void GeometryBufferEBOSubdata(OpenGL* opengl, GeometryBuffer* buffer, void* data, size_t size);
void GeometryBufferVertexAttrib(OpenGL* opengl, GeometryBuffer* buffer, u32 index, u32 componentCount, GLenum type,
                                size_t stride, size_t offset);

void ProgramInit(OpenGL* opengl, Program* program);
void ProgramAttachShader(OpenGL* opengl, Program* program, char* source, size_t length, GLenum type);
void ProgramBuild(OpenGL* opengl, Program* program);

void TextureAlloc(OpenGL* opengl, Texture* texture, void* imageBuffer, size_t size);