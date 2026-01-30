#define LINE_BUFFER_CHAR_COUNT 128

struct ObjFileInfo
{
    u32 vertexCount;
    u32 indexCount;
};

internal ObjFileInfo ObjGetFileInfo(FileReadResult file)
{
    ObjFileInfo result = {};

    u8* beginCursor = (u8*)file.content;
    u8* cursor      = beginCursor;
    u64 bytesRead   = 0;

    char buffer[LINE_BUFFER_CHAR_COUNT];

    while (bytesRead < file.contentSize)
    {
        while (*cursor != '\n')
        {
            bytesRead += sizeof(u8);
            cursor++;
        }

        memcpy(buffer, beginCursor, sizeof(char) * (cursor - beginCursor));

        char* attributeType = strtok(buffer, " ");

        if (attributeType)
        {
            if (strcmp(attributeType, "v") == 0)
            {
                result.vertexCount++;
            }
            else if (strcmp(attributeType, "f") == 0)
            {
                // Note: assuming always triangles
                result.indexCount += 3;
            }
            else if (strcmp(attributeType, "#") == 0)
            {
                // Skip code comment
            }
            else
            {
                // Unsupported attribute type
                InvalidCodePath;
            }
        }

        // Jump to the next line
        bytesRead += sizeof(u8);
        cursor++;
        beginCursor = cursor;
    }

    return result;
}

void ObjParseFile(FileReadResult file, PlatformLog* logf, OpenGL* opengl, Arena* arena, GeometryBuffer* geometryBuffer)
{
    if (file.contentSize > 0)
    {
        ObjFileInfo fileInfo = ObjGetFileInfo(file);

        u32* indices   = PushArray(arena, fileInfo.indexCount, u32);
        v3*  vertexs   = PushArray(arena, fileInfo.vertexCount, v3);
        u32* indexPtr  = indices;
        v3*  vertexPtr = vertexs;

        logf("Reading obj file");

        u8* beginCursor = (u8*)file.content;
        u8* cursor      = beginCursor;
        u64 bytesRead   = 0;
        u32 lineCount   = 1;

        char buffer[LINE_BUFFER_CHAR_COUNT];

        while (bytesRead < file.contentSize)
        {
            while (*cursor != '\n')
            {
                bytesRead += sizeof(u8);
                cursor++;
            }

            u32 lineCharCount = (u32)((u64)cursor - (u64)beginCursor);

            memcpy(buffer, beginCursor, sizeof(char) * (cursor - beginCursor));
            memset(&buffer[lineCharCount], 0, sizeof(char) * (LINE_BUFFER_CHAR_COUNT - lineCharCount));

            char* lineToken = strtok(buffer, " ");
            if (lineToken)
            {
                if (strcmp(lineToken, "v") == 0)
                {
                    f32* data = &vertexPtr->x;

                    while (lineToken)
                    {
                        lineToken = strtok(NULL, " ");
                        if (lineToken)
                        {
                            *data++ = (f32)atof(lineToken);
                        }
                    }

                    vertexPtr++;
                }
                else if (strcmp(lineToken, "f") == 0)
                {
                    while (lineToken)
                    {
                        lineToken = strtok(NULL, " ");
                        if (lineToken)
                        {
                            *indexPtr++ = (u32)atoi(lineToken) - 1;
                            int x       = 10;
                        }
                    }
                }
                else if (strcmp(lineToken, "#") == 0)
                {
                    // Skip code comment
                }
                else
                {
                    // Unsupported attribute type
                    logf("Obj: unsupported attribute type: %s", lineToken);
                    InvalidCodePath;
                }
            }
            else
            {
                // Skip empty line
            }

            // Jump to the next line
            bytesRead += sizeof(u8);
            cursor++;
            lineCount++;
            beginCursor = cursor;
        }

        GeometryBufferInit(opengl, geometryBuffer, GL_TRIANGLES);
        GeometryBufferVBOAlloc(opengl, geometryBuffer, vertexs, sizeof(v3) * fileInfo.vertexCount, sizeof(v3),
                               GL_STATIC_DRAW);
        GeometryBufferEBOAlloc(opengl, geometryBuffer, indices, sizeof(u32) * fileInfo.indexCount, sizeof(u32),
                               GL_STATIC_DRAW);
        GeometryBufferVertexAttrib(opengl, geometryBuffer, 0, 3, GL_FLOAT, 0, 0);

        logf("File read");
    }
}