#define LINE_BUFFER_CHAR_COUNT 128
#define UNKNOWN_VERTEX_INDEX   0xFFFFFFFF

enum ObjLineType
{
    ObjLineType_Vertex,        // v
    ObjLineType_Normal,        // vn
    ObjLineType_UV,            // vt
    ObjLineType_Face,          // f
    ObjLineType_Comment,       // #
    ObjLineType_Material,      // mtllib
    ObjLineType_UseMaterial,   // usemtl
    ObjLineType_UseMap,        // usemap
    ObjLineType_Group,         // g
    ObjLineType_Object,        // o
    ObjLineType_SmoothShading, // s
    ObjLineType_Empty
};

enum MaterialLineType
{
    MaterialLineType_Ambient,          // Ka
    MaterialLineType_Diffuse,          // Kd
    MaterialLineType_Specular,         // Ks
    MaterialLineType_Emmisive,         // Ke
    MaterialLineType_SpecularExponent, // Ns
    MaterialLineType_Disolve,          // d
    MaterialLineType_Illum,            // illum
    MaterialLineType_Comment,          // #
    MaterialLineType_Name,             // newmtl
    MaterialLineType_DiffuseMap,       // map_Kd
    MaterialLineType_Refraction,       // Ni
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
    else if (strcmp(attrBuf, "o") == 0)
    {
        return ObjLineType_Object;
    }
    else if (strcmp(attrBuf, "s") == 0)
    {
        return ObjLineType_SmoothShading;
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
        case ObjLineType_Material:
        {
            data->materialCount++;
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
    else if (strcmp(attrBuf, "Ke") == 0)
    {
        return MaterialLineType_Emmisive;
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
    else if (strcmp(attrBuf, "Ni") == 0)
    {
        return MaterialLineType_Refraction;
    }
    else
    {
        // Unsupported attribute type
        InvalidCodePath;
    }
}

internal void ObjParseMaterialFile(ObjMaterial* material, char* filename, PlatformAPI* platform)
{
    FileReadResult materialFile = platform->FileReadEntire(filename);
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
                char materialFileBuf[64];
                sscanf(lineBuf, "newmtl %[^\n]", materialFileBuf);
                sprintf(material->name, "%s", materialFileBuf);
                break;
            }
            case MaterialLineType_Ambient:
            {
                sscanf(lineBuf, "Ka %f %f %f", &material->ambient.r, &material->ambient.g, &material->ambient.b);
                break;
            }
            case MaterialLineType_Diffuse:
            {
                sscanf(lineBuf, "Kd %f %f %f", &material->diffuse.r, &material->diffuse.g, &material->diffuse.b);
                break;
            }
            case MaterialLineType_Specular:
            {
                sscanf(lineBuf, "Ks %f %f %f", &material->specular.r, &material->specular.g, &material->specular.b);
                break;
            }
            case MaterialLineType_SpecularExponent:
            {
                sscanf(lineBuf, "Ns %f", &material->specularExponent);
                break;
            }
            case MaterialLineType_Disolve:
            {
                sscanf(lineBuf, "d %f", &material->disolve);
                break;
            }
            case MaterialLineType_DiffuseMap:
            {
                sscanf(lineBuf, "map_Kd %s", &material->diffuseMap);
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

        platform->FileFree(materialFile.content);
    }
    else
    {
        InvalidCodePath;
    }
}

Obj ObjReadData(void* objBuf, size_t objBufSize, PlatformAPI* platform, Arena* arena)
{
    platform->Logf("------------------------------------------------------------");
    platform->Logf("Reading obj file");

    Obj result = { 0 };

    ObjPrepass(&result, objBuf, objBufSize);
    result.positions = PushArray(arena, result.positionCount, glm::vec3);
    result.normals   = PushArray(arena, result.normalCount, glm::vec3);
    result.uvs       = PushArray(arena, result.uvCount, glm::vec2);
    result.faces     = PushArray(arena, result.faceCount, ObjFace);
    result.materials = PushArray(arena, result.materialCount, ObjMaterial);

    platform->Logf("Vertex   count: %d", result.positionCount);
    platform->Logf("Normal   count: %d", result.normalCount);
    platform->Logf("UV       count: %d", result.uvCount);
    platform->Logf("Face     count: %d", result.faceCount);
    platform->Logf("Material count: %d", result.materialCount);

    u8* beginCursor   = (u8*)objBuf;
    u8* cursor        = beginCursor;
    u64 bytesRead     = 0;
    u32 lineCount     = 1;
    u32 materialIndex = 0;

    char       lineBuf[LINE_BUFFER_CHAR_COUNT];
    glm::vec3* vertexPtr = result.positions;
    glm::vec3* normalPtr = result.normals;
    glm::vec2* uvPtr     = result.uvs;
    ObjFace*   facePtr   = result.faces;

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
            sscanf(lineBuf, "v %f %f %f", &vertexPtr->x, &vertexPtr->y, &vertexPtr->z);

            result.aabb.min.x = Min(result.aabb.min.x, vertexPtr->x);
            result.aabb.min.y = Min(result.aabb.min.y, vertexPtr->y);
            result.aabb.min.z = Min(result.aabb.min.z, vertexPtr->z);

            result.aabb.max.x = Max(result.aabb.max.x, vertexPtr->x);
            result.aabb.max.y = Max(result.aabb.max.y, vertexPtr->y);
            result.aabb.max.z = Max(result.aabb.max.z, vertexPtr->z);

            vertexPtr++;
            break;
        }
        case ObjLineType_Normal:
        {
            sscanf(lineBuf, "vn %f %f %f", &normalPtr->x, &normalPtr->y, &normalPtr->z);
            normalPtr++;
            break;
        }
        case ObjLineType_UV:
        {
            sscanf(lineBuf, "vt %f %f", &uvPtr->x, &uvPtr->y);
            uvPtr->x = glm::clamp(uvPtr->x, 0.0f, uvPtr->x);
            uvPtr->y = glm::clamp(uvPtr->y, 0.0f, uvPtr->y);
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

                sscanf(lineBuf, "f %u %u %u", &facePtr->v0.position, &facePtr->v1.position, &facePtr->v2.position);

                for (u32 cornerIndex = 0; cornerIndex < ArrayCount(facePtr->corners); cornerIndex++)
                {
                    facePtr->corners[cornerIndex].position--;
                }
                facePtr++;
            }
            // Position + uv + normal
            else if (result.normalCount > 0 && result.uvCount > 0)
            {
                sscanf(lineBuf, "f %u/%u/%u %u/%u/%u %u/%u/%u", &facePtr->v0.position, &facePtr->v0.uv,
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
            //  TODO: Resolve path properly
            char materialFileBuf[128];
            sscanf(lineBuf, "mtllib %s", materialFileBuf);

            char materialFilepath[256];
            sprintf(materialFilepath, "%s", "../data/");
            strcat(materialFilepath, materialFileBuf);

            ObjMaterial* material = &result.materials[materialIndex++];
            ObjParseMaterialFile(material, materialFilepath, platform);
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
    platform->Logf("------------------------------------------------------------");

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

void ObjInitGeometryBuffer(Obj* obj, Arena* arena, Renderer* renderer, GPUBuffer* buffer)
{
    OpenGL* gl = renderer->gl;

    u32 maxVertices = obj->faceCount * 3;
    u32 maxIndices  = obj->faceCount * 3;

    Vertex*   vertexs       = PushArray(arena, maxVertices, Vertex);
    u32*      indices       = PushArray(arena, maxIndices, u32);
    ObjIndex* uniqueIndices = PushArray(arena, maxVertices, ObjIndex);

    u32     vertexCount = 0;
    u32     indexCount  = 0;
    u32*    indexPtr    = indices;
    Vertex* vertexPtr   = vertexs;

    for (u32 faceIndex = 0; faceIndex < obj->faceCount; faceIndex++)
    {
        ObjFace* face = &obj->faces[faceIndex];

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
                vertexPtr->position = obj->positions[corner->position];

                if (corner->normal != UNKNOWN_VERTEX_INDEX)
                {
                    vertexPtr->normal = obj->normals[corner->normal];
                }
                if (corner->uv != UNKNOWN_VERTEX_INDEX)
                {
                    vertexPtr->uv = obj->uvs[corner->uv];
                }

                *indexPtr++                = vertexCount;
                uniqueIndices[vertexCount] = *corner;

                vertexCount++;
                vertexPtr++;
                indexCount++;
            }
        }
    }

    size_t vertexSize = sizeof(Vertex);
    size_t indexSize  = sizeof(u32);

    GPUBufferInit(renderer, buffer);
    GPUBufferVBOAlloc(renderer, buffer, vertexs, vertexSize * vertexCount, vertexSize, GL_STATIC_DRAW);
    GPUBufferEBOAlloc(renderer, buffer, indices, indexSize * indexCount, indexSize, GL_STATIC_DRAW);

    GPUBufferVertexAttrib(renderer, buffer, 0, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, position));
    GPUBufferVertexAttrib(renderer, buffer, 1, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, normal));
    GPUBufferVertexAttrib(renderer, buffer, 2, 2, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, uv));
}
