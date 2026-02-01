#pragma once

#define OBJ_INVALID_INDEX 0xFFFFFFFF

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

struct Obj
{
    u32      positionCount; // v
    u32      normalCount;   // vn
    u32      uvCount;       // vt
    u32      faceCount;     // f
    v3*      positions;
    v3*      normals;
    v2*      uvs;
    ObjFace* faces;
};

struct ObjMaterial
{
    v3    ambient;          // Ka
    v3    diffuse;          // Kd
    v3    specular;         // Ks
    f32   specularExponent; // Ns
    f32   disolve;          // d
    u32   illumModel;       // illum
    char* diffuseMap;
};

Obj  ObjReadData(void* objBuf, size_t objBufSize, PlatformFileReadEntire* fileRead, PlatformFileFree* fileFree,
                 PlatformLog* logf, Arena* arena);
void ObjInitGeometryBuffer(Obj* data, Arena* arena, OpenGL* opengl, GeometryBuffer* buffer);