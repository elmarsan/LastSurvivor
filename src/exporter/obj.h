#pragma once

#define LINE_BUFFER_CHAR_COUNT 128
#define UNKNOWN_VERTEX_INDEX   0xFFFFFFFF

// Note: Limited to triangulated faces

struct ObjIndex
{
    u32 position;
    u32 uv;
    u32 normal;
};

union ObjFace
{
    struct
    {
        ObjIndex v0;
        ObjIndex v1;
        ObjIndex v2;
    };

    ObjIndex corners[3];
};

struct ObjMaterial
{
    char      name[64];
    char      diffuseMap[256];
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    f32       specularExponent;
    f32       disolve;
    u32       illumModel;
};

struct Obj
{
    u32          positionCount;
    u32          normalCount;
    u32          uvCount;
    u32          faceCount;
    u32          materialCount;
    glm::vec3*   positions;
    glm::vec3*   normals;
    glm::vec2*   uvs;
    ObjFace*     faces;
    ObjMaterial* materials;
    AABB         aabb;
    // GPU buffers
    std::vector<Vertex> vertexs;
    std::vector<u32>    indices;
};

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

ObjLineType ObjGetLineType(char* lineBuf)
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

void ObjPrepass(Obj* data, void* objBuffer, size_t size)
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

MaterialLineType MaterialGetLineType(char* lineBuf)
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

void ObjParseMaterialFile(ObjMaterial* material, char* filename, PlatformAPI* platform)
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
                sscanf(lineBuf, "Ka %f %f %f", &material->ambient.x, &material->ambient.y, &material->ambient.z);
                break;
            }
            case MaterialLineType_Diffuse:
            {
                sscanf(lineBuf, "Kd %f %f %f", &material->diffuse.x, &material->diffuse.y, &material->diffuse.z);
                break;
            }
            case MaterialLineType_Specular:
            {
                sscanf(lineBuf, "Ks %f %f %f", &material->specular.x, &material->specular.y, &material->specular.z);
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

u32 FindVertexIndex(ObjIndex* index, ObjIndex* indices, u32 indexCount)
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

void ObjComputeGPUBuffers(Obj* obj)
{
    u32 maxVertices = obj->faceCount * 3;

    obj->vertexs.clear();
    obj->indices.clear();
    obj->vertexs.reserve(maxVertices);
    obj->indices.reserve(maxVertices);

    std::vector<ObjIndex> uniqueIndices;
    uniqueIndices.reserve(maxVertices);

    for (u32 faceIndex = 0; faceIndex < obj->faceCount; faceIndex++)
    {
        ObjFace* face = &obj->faces[faceIndex];

        for (u32 cornerIndex = 0; cornerIndex < 3; cornerIndex++)
        {
            ObjIndex* corner = &face->corners[cornerIndex];

            u32 vertexIndex = FindVertexIndex(corner, uniqueIndices.data(), (u32)uniqueIndices.size());

            if (vertexIndex != UNKNOWN_VERTEX_INDEX)
            {
                obj->indices.push_back(vertexIndex);
            }
            else
            {
                Vertex vertex   = {};
                vertex.position = obj->positions[corner->position];

                if (corner->normal != UNKNOWN_VERTEX_INDEX)
                {
                    vertex.normal = obj->normals[corner->normal];
                }
                if (corner->uv != UNKNOWN_VERTEX_INDEX)
                {
                    vertex.uv = obj->uvs[corner->uv];
                }

                u32 newIndex = (u32)obj->vertexs.size();

                obj->vertexs.push_back(vertex);
                obj->indices.push_back(newIndex);
                uniqueIndices.push_back(*corner);
            }
        }
    }
}

Obj ObjParse(char* filename, PlatformAPI* platform, Arena* arena)
{
    platform->Logf("------------------------------------------------------------");
    platform->Logf("Reading .obj file: '%s'", filename);

    Obj result = { 0 };

    FileReadResult objFile = platform->FileReadEntire(filename);
    if (objFile.content)
    {
        ObjPrepass(&result, objFile.content, objFile.contentSize);
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

        u8* beginCursor   = (u8*)objFile.content;
        u8* cursor        = beginCursor;
        u64 bytesRead     = 0;
        u32 lineCount     = 1;
        u32 materialIndex = 0;

        char       lineBuf[LINE_BUFFER_CHAR_COUNT];
        glm::vec3* vertexPtr = result.positions;
        glm::vec3* normalPtr = result.normals;
        glm::vec2* uvPtr     = result.uvs;
        ObjFace*   facePtr   = result.faces;

        while (bytesRead < objFile.contentSize)
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
                char   materialFilename[128];
                char   materialFilepath[256];
                size_t directoryLen = GetParentPathLength(filename);

                sscanf(lineBuf, "mtllib %s", materialFilename);
                memcpy(materialFilepath, filename, directoryLen);
                strcpy(materialFilepath + directoryLen, materialFilename);

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

        platform->FileFree(objFile.content);
    }
    else
    {
        platform->Logf("Not found");
    }
    platform->Logf("------------------------------------------------------------");

    ObjComputeGPUBuffers(&result);

    return result;
}