#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb_image.h>

RenderCommandQueue* RendererFrameBegin(OpenGL* opengl)
{
    RenderCommandQueue* queue = &opengl->commandQueue;

    queue->pushBufferBase = opengl->commandQueueBufferMemory;
    queue->pushBufferPtr  = queue->pushBufferBase;
    queue->pushBufferSize = sizeof(opengl->commandQueueBufferMemory);

    return queue;
}

void RendererFrameEnd(OpenGL* opengl)
{
    RenderCommandQueue* queue = &opengl->commandQueue;

    opengl->glEnable(GL_DEPTH_TEST);

    // opengl->glPolygonMode(GL_FRONT_AND_BACK , GL_LINE);
    opengl->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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

            opengl->glClearColor(command->color.r, command->color.g, command->color.b, 1.0f);
            opengl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            break;
        }
        case RenderCommandType_GeometryBufferDraw:
        {
            command += sizeof(GeometryBufferDraw);
            GeometryBufferDraw* command = (GeometryBufferDraw*)payload;
            GeometryBuffer*     buffer  = &command->buffer;

            opengl->glBindVertexArray(buffer->VAO);

            // Indexed
            if (buffer->indexCount > 0)
            {
                opengl->glDrawElements(buffer->primitive, buffer->indexCount, GL_UNSIGNED_INT, 0);
            }
            else
            {
                opengl->glDrawArrays(buffer->primitive, 0, buffer->vertexCount);
            }
            break;
        }
        case RenderCommandType_ProgramUse:
        {
            command += sizeof(ProgramUse);
            ProgramUse* command = (ProgramUse*)payload;

            opengl->glUseProgram(command->program.id);
            break;
        }
        case RenderCommandType_ProgramUploadUniform:
        {
            command += sizeof(ProgramUploadUniform);
            ProgramUploadUniform* command = (ProgramUploadUniform*)payload;

            GLint loc = opengl->glGetUniformLocation(command->programId, command->name);
            if (loc != -1)
            {
                switch (command->type)
                {
                case UniformType_Mat4x4:
                {
                    opengl->glUniformMatrix4fv(loc, 1, GL_FALSE, &command->mat4x4.ptr[0]);
                    break;
                }
                case UniformType_Int:
                {
                    opengl->glUniform1i(loc, command->integer);
                    break;
                }
                case UniformType_IntArray:
                {
                    opengl->glUniform1iv(loc, command->count, command->integerArray);
                    break;
                }
                case UniformType_Vec3:
                {
                    opengl->glUniform3fv(loc, 1, &command->vec3.x);
                    break;
                }
                case UniformType_Vec4:
                {
                    opengl->glUniform4fv(loc, 1, &command->vec4.x);
                    break;
                }
                    InvalidDefaultCase;
                }
            }
            else
            {
                Log("Uniform '%s' not found in program '%d'", command->name, command->programId);
            }

            break;
        }
        }
    }
#pragma warning(pop)
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

inline void PushRenderProgramUse(RenderCommandQueue* queue, GLuint programId)
{
    ProgramUse* command = PushRenderCommand(queue, ProgramUse);
    command->program.id = programId;
}

inline void PushRenderUploadUniformMat4x4(RenderCommandQueue* queue, GLuint programId, const char* name, mat4x4 mat4)
{
    ProgramUploadUniform* command = PushRenderCommand(queue, ProgramUploadUniform);
    sprintf(command->name, "%s", name);
    command->programId = programId;
    command->mat4x4    = mat4;
    command->type      = UniformType_Mat4x4;
}

inline void PushRenderUploadUniformInt(RenderCommandQueue* queue, GLuint programId, const char* name, int integer)
{
    ProgramUploadUniform* command = PushRenderCommand(queue, ProgramUploadUniform);
    sprintf(command->name, "%s", name);
    command->programId = programId;
    command->integer   = integer;
    command->type      = UniformType_Int;
}

inline void PushRenderUploadUniformIntArray(RenderCommandQueue* queue, GLuint programId, const char* name, int* array,
                                            int count)
{
    ProgramUploadUniform* command = PushRenderCommand(queue, ProgramUploadUniform);
    sprintf(command->name, "%s", name);
    command->programId    = programId;
    command->integerArray = array;
    command->count        = count;
    command->type         = UniformType_IntArray;
}

inline void PushRenderUploadUniformVec3(RenderCommandQueue* queue, GLuint programId, const char* name, v3 vec3)
{
    ProgramUploadUniform* command = PushRenderCommand(queue, ProgramUploadUniform);
    sprintf(command->name, "%s", name);
    command->programId = programId;
    command->vec3      = vec3;
    command->type      = UniformType_Vec3;
}

inline void PushRenderUploadUniformVec4(RenderCommandQueue* queue, GLuint programId, const char* name, v4 vec4)
{
    ProgramUploadUniform* command = PushRenderCommand(queue, ProgramUploadUniform);
    sprintf(command->name, "%s", name);
    command->programId = programId;
    command->vec4      = vec4;
    command->type      = UniformType_Vec4;
}

inline void PushRenderDrawBuffer(RenderCommandQueue* queue, GeometryBuffer* buffer)
{
    GeometryBufferDraw* command = PushRenderCommand(queue, GeometryBufferDraw);
    command->buffer             = *buffer;
}

void GeometryBufferInit(OpenGL* opengl, GeometryBuffer* buffer, GLenum primitive)
{
    opengl->glGenVertexArrays(1, &buffer->VAO);

    buffer->VBO         = 0;
    buffer->EBO         = 0;
    buffer->vertexCount = 0;
    buffer->indexCount  = 0;
    buffer->primitive   = primitive;
}

void GeometryBufferVBOAlloc(OpenGL* opengl, GeometryBuffer* buffer, void* data, size_t size, size_t vertexSize,
                            GLenum usage)
{
    buffer->vertexCount = (u32)(size / vertexSize);

    opengl->glGenBuffers(1, &buffer->VBO);
    opengl->glBindVertexArray(buffer->VAO);
    opengl->glBindBuffer(GL_ARRAY_BUFFER, buffer->VBO);
    opengl->glBufferData(GL_ARRAY_BUFFER, (GLsizei)size, data, usage);
}

void GeometryBufferVBOSubdata(OpenGL* opengl, GeometryBuffer* buffer, void* data, size_t size)
{
    opengl->glBindVertexArray(buffer->VAO);
    opengl->glBindBuffer(GL_ARRAY_BUFFER, buffer->VBO);
    opengl->glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

void GeometryBufferEBOAlloc(OpenGL* opengl, GeometryBuffer* buffer, void* data, size_t size, size_t indexSize,
                            GLenum usage)
{
    buffer->indexCount = (u32)(size / indexSize);

    opengl->glGenBuffers(1, &buffer->EBO);
    opengl->glBindVertexArray(buffer->VAO);
    opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->EBO);
    opengl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei)size, data, usage);
}

void GeometryBufferEBOSubdata(OpenGL* opengl, GeometryBuffer* buffer, void* data, size_t size)
{
    opengl->glBindVertexArray(buffer->VAO);
    opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->EBO);
    opengl->glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size, data);
}

void GeometryBufferVertexAttrib(OpenGL* opengl, GeometryBuffer* buffer, u32 index, u32 componentCount, GLenum type,
                                size_t stride, size_t offset)
{
    opengl->glBindVertexArray(buffer->VAO);
    if (type == GL_BYTE || type == GL_UNSIGNED_BYTE || type == GL_SHORT || type == GL_UNSIGNED_SHORT ||
        type == GL_INT || type == GL_UNSIGNED_INT)
    {
        opengl->glVertexAttribIPointer((GLuint)index, (GLint)componentCount, type, (GLsizei)stride, (void*)offset);
    }
    else
    {
        opengl->glVertexAttribPointer((GLuint)index, (GLint)componentCount, type, false, (GLsizei)stride,
                                      (void*)offset);
    }
    opengl->glEnableVertexAttribArray((GLuint)index);
}

void ProgramInit(OpenGL* opengl, Program* program) { program->id = opengl->glCreateProgram(); }

void ProgramAttachShader(OpenGL* opengl, Program* program, char* source, size_t length, GLenum type)
{
    GLuint shader     = opengl->glCreateShader(type);
    GLint  sourceSize = (GLint)length;
    opengl->glShaderSource(shader, 1, &source, &sourceSize);
    opengl->glCompileShader(shader);

    GLint ok;
    opengl->glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char infoBuffer[512];
        opengl->glGetShaderInfoLog(shader, sizeof(infoBuffer), NULL, infoBuffer);
        Log("OpenGL compiling shader: '%s'", infoBuffer);
        Assert(0);
    }

    opengl->glAttachShader(program->id, shader);
}

void ProgramBuild(OpenGL* opengl, Program* program)
{
    opengl->glLinkProgram(program->id);

    GLint ok;
    opengl->glGetProgramiv(program->id, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char infoBuffer[512];
        opengl->glGetProgramInfoLog(program->id, sizeof(infoBuffer), NULL, infoBuffer);
        Log("OpenGL linking program: '%s'", infoBuffer);
        Assert(0);
    }
}

void TextureAlloc(OpenGL* opengl, Texture* texture, void* imageBuffer, size_t size)
{
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

        opengl->glGenTextures(1, &texture->id);
        opengl->glBindTexture(GL_TEXTURE_2D, texture->id);
        opengl->glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, (GLsizei)width, (GLsizei)height, 0, format, type,
                             pixels);
        opengl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        opengl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        Assert(0);
    }
}