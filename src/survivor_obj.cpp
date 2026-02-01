#define LINE_BUFFER_CHAR_COUNT 128
#define UNKNOWN_VERTEX_INDEX   0xFFFFFFFF

enum ObjLineType
{
    ObjLineType_Vertex,      // v
    ObjLineType_Normal,      // vn
    ObjLineType_UV,          // vt
    ObjLineType_Face,        // f
    ObjLineType_Comment,     // #
    ObjLineType_Material,    // mtllib
    ObjLineType_UseMaterial, // usemtl
    ObjLineType_UseMap,      // usemap
    ObjLineType_Group,       // g
    ObjLineType_Empty
};

enum MaterialLineType
{
    MaterialLineType_Ambient,          // Ka
    MaterialLineType_Diffuse,          // Kd
    MaterialLineType_Specular,         // Ks
    MaterialLineType_SpecularExponent, // Ns
    MaterialLineType_Disolve,          // d
    MaterialLineType_Illum,            // illum
    MaterialLineType_Comment,          // #
    MaterialLineType_Name,             // newmtl
    MaterialLineType_DiffuseMap,       // map_Kd
    MaterialLineType_Empty
};

internal ObjLineType ObjGetLineType(char* lineBuf)
{
    char attrBuf[64];
    sscanf(lineBuf, "%s ", attrBuf);

    if (strlen(lineBuf) == 0)
    {
        return ObjLineType_Empty;
    }
    else if (strcmp(attrBuf, "v") == 0)
    {
        return ObjLineType_Vertex;
    }
    else if (strcmp(attrBuf, "vn") == 0)
    {
        return ObjLineType_Normal;
    }
    else if (strcmp(attrBuf, "vt") == 0)
    {
        return ObjLineType_UV;
    }
    else if (strcmp(attrBuf, "f") == 0)
    {
        return ObjLineType_Face;
    }
    else if (strcmp(attrBuf, "#") == 0)
    {
        return ObjLineType_Comment;
    }
    else if (strcmp(attrBuf, "mtllib") == 0)
    {
        return ObjLineType_Material;
    }
    else if (strcmp(attrBuf, "usemtl") == 0)
    {
        return ObjLineType_UseMaterial;
    }
    else if (strcmp(attrBuf, "usemap") == 0)
    {
        return ObjLineType_UseMap;
    }
    else if (strcmp(attrBuf, "g") == 0)
    {
        return ObjLineType_Group;
    }
    else
    {
        // Unsupported attribute type
        InvalidCodePath;
    }
}

internal void ObjPrepass(Obj* data, void* objBuffer, size_t size)
{
    u8* beginCursor = (u8*)objBuffer;
    u8* cursor      = beginCursor;
    u64 bytesRead   = 0;

    char lineBuf[LINE_BUFFER_CHAR_COUNT];

    while (bytesRead < size)
    {
        while (*cursor != '\n')
        {
            bytesRead += sizeof(u8);
            cursor++;
        }

        memcpy(lineBuf, beginCursor, sizeof(char) * (cursor - beginCursor));

        ObjLineType lineType = ObjGetLineType(lineBuf);
        switch (lineType)
        {
        case ObjLineType_Vertex:
        {
            data->positionCount++;
            break;
        }
        case ObjLineType_Normal:
        {
            data->normalCount++;
            break;
        }
        case ObjLineType_UV:
        {
            data->uvCount++;
            break;
        }
        case ObjLineType_Face:
        {
            data->faceCount++;
            break;
        }
            DefaultCase;
        }

        // Jump to the next line
        bytesRead += sizeof(u8);
        cursor++;
        beginCursor = cursor;
    }
}

internal MaterialLineType MaterialGetLineType(char* lineBuf)
{
    char attrBuf[64];
    sscanf(lineBuf, "%s ", attrBuf);

    if (strlen(lineBuf) == 0)
    {
        return MaterialLineType_Empty;
    }
    else if (strcmp(attrBuf, "#") == 0)
    {
        return MaterialLineType_Comment;
    }
    else if (strcmp(attrBuf, "Ka") == 0)
    {
        return MaterialLineType_Ambient;
    }
    else if (strcmp(attrBuf, "Kd") == 0)
    {
        return MaterialLineType_Diffuse;
    }
    else if (strcmp(attrBuf, "Ks") == 0)
    {
        return MaterialLineType_Specular;
    }
    else if (strcmp(attrBuf, "Ns") == 0)
    {
        return MaterialLineType_SpecularExponent;
    }
    else if (strcmp(attrBuf, "d") == 0)
    {
        return MaterialLineType_Disolve;
    }
    else if (strcmp(attrBuf, "illum") == 0)
    {
        return MaterialLineType_Illum;
    }
    else if (strcmp(attrBuf, "newmtl") == 0)
    {
        return MaterialLineType_Name;
    }
    else if (strcmp(attrBuf, "map_Kd") == 0)
    {
        return MaterialLineType_DiffuseMap;
    }
    else
    {
        // Unsupported attribute type
        InvalidCodePath;
    }
}

internal void ObjParseMaterialFile(Obj* obj, char* filename, PlatformFileReadEntire* fileRead,
                                   PlatformFileFree* fileFree)
{
    FileReadResult materialFile = fileRead(filename);
    if (materialFile.contentSize > 0)
    {
        u8* beginCursor = (u8*)materialFile.content;
        u8* cursor      = beginCursor;
        u64 bytesRead   = 0;
        u32 lineCount   = 1;

        char lineBuf[LINE_BUFFER_CHAR_COUNT];

        while (bytesRead < materialFile.contentSize)
        {
            while (*cursor != '\n')
            {
                bytesRead += sizeof(u8);
                cursor++;
            }

            u32 lineCharCount = (u32)((u64)cursor - (u64)beginCursor);

            memcpy(lineBuf, beginCursor, sizeof(char) * (lineCharCount));
            memset(&lineBuf[lineCharCount], 0, sizeof(char) * (LINE_BUFFER_CHAR_COUNT - lineCharCount));

            MaterialLineType lineType = MaterialGetLineType(lineBuf);

            switch (lineType)
            {
            case MaterialLineType_Name:
            {
                // name
                break;
            }
            case MaterialLineType_Ambient:
            {
                v3 ambient{ 0.0f, 0.0f, 0.0f };
                sscanf(lineBuf, "Ka%f %f %f", &ambient.r, &ambient.g, &ambient.b);
                break;
            }
            case MaterialLineType_Diffuse:
            {
                v3 diffuse{ 0.0f, 0.0f, 0.0f };
                sscanf(lineBuf, "Kd%f %f %f", &diffuse.r, &diffuse.g, &diffuse.b);
                break;
            }
            case MaterialLineType_Specular:
            {
                v3 specular{ 0.0f, 0.0f, 0.0f };
                sscanf(lineBuf, "Ks%f %f %f", &specular.r, &specular.g, &specular.b);
                break;
            }

                DefaultCase;
            }

            // Jump to the next line
            bytesRead += sizeof(u8);
            cursor++;
            lineCount++;
            beginCursor = cursor;
        }

        fileFree(materialFile.content);
    }
    else
    {
        InvalidCodePath;
    }
}

Obj ObjReadData(void* objBuf, size_t objBufSize, PlatformFileReadEntire* fileRead, PlatformFileFree* fileFree,
                PlatformLog* logf, Arena* arena)
{
    logf("------------------------------------------------------------");
    logf("Reading obj file");

    Obj result = { 0 };

    ObjPrepass(&result, objBuf, objBufSize);
    result.positions = PushArray(arena, result.positionCount, v3);
    result.normals   = PushArray(arena, result.normalCount, v3);
    result.uvs       = PushArray(arena, result.uvCount, v2);
    result.faces     = PushArray(arena, result.faceCount, ObjFace);

    logf("Vertex count: %d", result.positionCount);
    logf("Normal count: %d", result.normalCount);
    logf("UV     count: %d", result.uvCount);
    logf("Face   count: %d", result.faceCount);

    u8* beginCursor = (u8*)objBuf;
    u8* cursor      = beginCursor;
    u64 bytesRead   = 0;
    u32 lineCount   = 1;

    char     lineBuf[LINE_BUFFER_CHAR_COUNT];
    v3*      vertexPtr = result.positions;
    v3*      normalPtr = result.normals;
    v2*      uvPtr     = result.uvs;
    ObjFace* facePtr   = result.faces;

    while (bytesRead < objBufSize)
    {
        while (*cursor != '\n')
        {
            bytesRead += sizeof(u8);
            cursor++;
        }

        u32 lineCharCount = (u32)((u64)cursor - (u64)beginCursor);

        memcpy(lineBuf, beginCursor, sizeof(char) * (lineCharCount));
        memset(&lineBuf[lineCharCount], 0, sizeof(char) * (LINE_BUFFER_CHAR_COUNT - lineCharCount));

        ObjLineType lineType = ObjGetLineType(lineBuf);

        switch (lineType)
        {
        case ObjLineType_Vertex:
        {
            sscanf(lineBuf, "v%f %f %f", &vertexPtr->x, &vertexPtr->y, &vertexPtr->z);
            vertexPtr++;
            break;
        }
        case ObjLineType_Normal:
        {
            sscanf(lineBuf, "vn%f %f %f", &normalPtr->x, &normalPtr->y, &normalPtr->z);
            normalPtr++;
            break;
        }
        case ObjLineType_UV:
        {
            sscanf(lineBuf, "vt%f %f", &uvPtr->u, &uvPtr->v);
            uvPtr->u = Clamp(uvPtr->u, 0.0f, uvPtr->u);
            uvPtr->v = Clamp(uvPtr->v, 0.0f, uvPtr->v);
            uvPtr++;
            break;
        }
        case ObjLineType_Face:
        {
            // TODO: Handle (x, y, z, w) indices
            // TODO: Handle negative indices
            // TODO: Handle different patterns "v1" "v1/vt1" "v1/vt1/vn1" "v1//vn1"
            // Position
            if (result.normalCount == 0 && result.uvCount == 0)
            {
                facePtr->v0.normal = UNKNOWN_VERTEX_INDEX;
                facePtr->v1.normal = UNKNOWN_VERTEX_INDEX;
                facePtr->v2.normal = UNKNOWN_VERTEX_INDEX;
                facePtr->v0.uv     = UNKNOWN_VERTEX_INDEX;
                facePtr->v1.uv     = UNKNOWN_VERTEX_INDEX;
                facePtr->v2.uv     = UNKNOWN_VERTEX_INDEX;

                sscanf(lineBuf, "f%u %u %u", &facePtr->v0.position, &facePtr->v1.position, &facePtr->v2.position);

                for (u32 cornerIndex = 0; cornerIndex < ArrayCount(facePtr->corners); cornerIndex++)
                {
                    facePtr->corners[cornerIndex].position--;
                }
                facePtr++;
            }
            // Position + uv + normal
            else if (result.normalCount > 0 && result.uvCount > 0)
            {
                sscanf(lineBuf, "f%u/%u/%u %u/%u/%u %u/%u/%u", &facePtr->v0.position, &facePtr->v0.uv,
                       &facePtr->v0.normal, &facePtr->v1.position, &facePtr->v1.uv, &facePtr->v1.normal,
                       &facePtr->v2.position, &facePtr->v2.uv, &facePtr->v2.normal);

                for (u32 cornerIndex = 0; cornerIndex < ArrayCount(facePtr->corners); cornerIndex++)
                {
                    facePtr->corners[cornerIndex].position--;
                    facePtr->corners[cornerIndex].normal--;
                    facePtr->corners[cornerIndex].uv--;
                }
                facePtr++;
            }
            else
            {
                InvalidCodePath;
            }

            break;
        }
        case ObjLineType_Material:
        {
            // TODO: Resolve path properly
            // char materialFileBuf[128];
            // sscanf(lineBuf, "mtllib ./%s", materialFileBuf);
            // ObjParseMaterialFile(&result, materialFileBuf, fileRead, fileFree);

            ObjParseMaterialFile(&result, "../data/fence.mtl", fileRead, fileFree);
            break;
        }
            DefaultCase;
        }

        // Jump to the next line
        bytesRead += sizeof(u8);
        cursor++;
        lineCount++;
        beginCursor = cursor;
    }
    logf("------------------------------------------------------------");

    return result;
}

internal u32 FindVertexIndex(ObjIndex* index, ObjIndex* indices, u32 indexCount)
{
    for (u32 i = 0; i < indexCount; i++)
    {
        if (indices[i].position == index->position && indices[i].uv == index->uv && indices[i].normal == index->normal)
        {
            return i;
        }
    }
    return UNKNOWN_VERTEX_INDEX;
}

void ObjInitGeometryBuffer(Obj* data, Arena* arena, OpenGL* opengl, GeometryBuffer* buffer)
{
    u32 maxVertices = data->faceCount * 3;
    u32 maxIndices  = data->faceCount * 3;

    Vertex*   vertexs       = PushArray(arena, maxVertices, Vertex);
    u32*      indices       = PushArray(arena, maxIndices, u32);
    ObjIndex* uniqueIndices = PushArray(arena, maxVertices, ObjIndex);

    u32     vertexCount     = 0;
    u32     indexCount      = 0;
    u32     buildIndexCount = 0;
    u32*    indexPtr        = indices;
    Vertex* vertexPtr       = vertexs;

    for (u32 faceIndex = 0; faceIndex < data->faceCount; faceIndex++)
    {
        ObjFace* face = &data->faces[faceIndex];

        for (u32 cornerIndex = 0; cornerIndex < 3; cornerIndex++)
        {
            ObjIndex* corner = &face->corners[cornerIndex];

            u32 vertexIndex = FindVertexIndex(corner, uniqueIndices, vertexCount);

            if (vertexIndex != UNKNOWN_VERTEX_INDEX)
            {
                *indexPtr++ = vertexIndex;
                indexCount++;
            }
            else
            {
                vertexPtr->position = data->positions[corner->position];

                if (corner->normal != UNKNOWN_VERTEX_INDEX)
                {
                    vertexPtr->normal = data->normals[corner->normal];
                }
                if (corner->uv != UNKNOWN_VERTEX_INDEX)
                {
                    vertexPtr->uv = data->uvs[corner->uv];
                }

                *indexPtr++                = vertexCount;
                uniqueIndices[vertexCount] = *corner;

                vertexCount++;
                vertexPtr++;
                indexCount++;
            }
        }
    }

    GeometryBufferInit(opengl, buffer, GL_TRIANGLES);
    GeometryBufferVBOAlloc(opengl, buffer, vertexs, sizeof(Vertex) * vertexCount, sizeof(v3), GL_STATIC_DRAW);
    GeometryBufferEBOAlloc(opengl, buffer, indices, sizeof(u32) * indexCount, sizeof(u32), GL_STATIC_DRAW);

    GeometryBufferVertexAttrib(opengl, buffer, 0, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, position));
    GeometryBufferVertexAttrib(opengl, buffer, 1, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, normal));
    GeometryBufferVertexAttrib(opengl, buffer, 2, 2, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, uv));
}
