#pragma once

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
    char name[64];
    char diffuseMap[256];
    v3   ambient;
    v3   diffuse;
    v3   specular;
    f32  specularExponent;
    f32  disolve;
    u32  illumModel;
};

struct Obj
{
    u32          positionCount;
    u32          normalCount;
    u32          uvCount;
    u32          faceCount;
    u32          materialCount;
    v3*          positions;
    v3*          normals;
    v2*          uvs;
    ObjFace*     faces;
    ObjMaterial* materials;
    AABB         aabb;
};

Obj  ObjReadData(void* objBuf, size_t objBufSize, PlatformAPI* platform, Arena* arena);
void ObjInitGeometryBuffer(Obj* obj, Arena* arena, Renderer* renderer, GPUBuffer* buffer);