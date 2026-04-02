enum AssetType
{
    AssetType_Model   = 1,
    AssetType_Font    = 2,
    AssetType_Sfx     = 3,
    AssetType_Texture = 4,
};

struct AssetFileHeader
{
    u8  type;
    u8  _reserved1;
    u8  _reserved2;
    u8  _reserved3;
    u64 _reserved4;
};

struct AssetModelFileHeader
{
    u32 vertexCount;
    u32 indicesCount;
};

// clang-format off
internal char* assetFilenames[AssetID_Count] = {
    "../data/stickman.svv",
    "../data/fence.svv",
    "../data/ZombieFemale_A.svv",
    "../data/ZombieMale_A.svv",
    "../data/zcolors.png",
    "../data/crosshairs.png",
    "../data/WoodPlanksOld0242_7_S.jpg"
};
// clang-format on

void AssetLoad(Assets* assets, AssetID id)
{
    PlatformAPI* platform = assets->platform;
    Arena*       arena    = &assets->arena;
    Renderer*    renderer = assets->renderer;

    char* assetFilename = assetFilenames[id];
    platform->Logf("Loading asset '%s'", assetFilename);

    if (id >= AssetID_ZombieTexture && id <= AssetID_FenceTexture)
    {
        assets->textures[id - AssetID_ZombieTexture] = PushStruct(arena, Texture);
        TextureInit(renderer, assets->textures[id - AssetID_ZombieTexture], assetFilename);
    }
    else
    {
        FileReadResult file = platform->FileReadEntire(assetFilename);
        if (file.content)
        {
            Stream stream = StreamInit((u8*)file.content);

            AssetFileHeader header;
            StreamRead(&stream, &header, sizeof(header), 1);

            switch (header.type)
            {
            case AssetType_Model:
            {
                Assert(assets->models[id] == 0);
                assets->models[id] = PushStruct(arena, Model);
                Model* model       = assets->models[id];

                AssetModelFileHeader modelHeader;
                StreamRead(&stream, &modelHeader, sizeof(modelHeader), 1);

                model->vertexCount  = modelHeader.vertexCount;
                model->indicesCount = modelHeader.indicesCount;
                model->vertexs      = PushArray(arena, modelHeader.vertexCount, Vertex);
                model->indices      = PushArray(arena, modelHeader.indicesCount, u32);

                StreamRead(&stream, model->vertexs, sizeof(Vertex), modelHeader.vertexCount);
                StreamRead(&stream, model->indices, sizeof(u32), modelHeader.indicesCount);

                for (u32 vertexIndex = 0; vertexIndex < model->vertexCount; vertexIndex++)
                {
                    glm::vec3 position = model->vertexs[vertexIndex].position;

                    model->aabb.min.x = Min(model->aabb.min.x, position.x);
                    model->aabb.min.y = Min(model->aabb.min.y, position.y);
                    model->aabb.min.z = Min(model->aabb.min.z, position.z);

                    model->aabb.max.x = Max(model->aabb.max.x, position.x);
                    model->aabb.max.y = Max(model->aabb.max.y, position.y);
                    model->aabb.max.z = Max(model->aabb.max.z, position.z);
                }

                // TODO: Remove fence simetric hack
                if (id == AssetID_Fence)
                {
                    model->aabb.max   = model->aabb.max;
                    model->aabb.min   = -model->aabb.max;
                    model->aabb.min.y = model->aabb.min.y;
                }

                model->gpuBuffer = PushStruct(arena, GPUBuffer);

                GPUBufferInit(renderer, model->gpuBuffer);
                GPUBufferVBOAlloc(renderer, model->gpuBuffer, model->vertexs, sizeof(Vertex) * model->vertexCount,
                                  sizeof(Vertex), GL_STATIC_DRAW);
                GPUBufferEBOAlloc(renderer, model->gpuBuffer, model->indices, sizeof(u32) * model->indicesCount,
                                  sizeof(u32), GL_STATIC_DRAW);

                GPUBufferVertexAttrib(renderer, model->gpuBuffer, 0, 3, GL_FLOAT, sizeof(Vertex),
                                      offsetof(Vertex, position));
                GPUBufferVertexAttrib(renderer, model->gpuBuffer, 1, 3, GL_FLOAT, sizeof(Vertex),
                                      offsetof(Vertex, normal));
                GPUBufferVertexAttrib(renderer, model->gpuBuffer, 2, 2, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, uv));

                break;
            }
            case AssetType_Font:
            {
                Assert(0);
            }
            case AssetType_Sfx:
            {
                Assert(0);
            }
                InvalidDefaultCase;
            }

            platform->FileFree(file.content);
        }
        else
        {
            platform->Logf("Unable to load asset");
            Assert(0);
        }
    }
}

internal void AssetExportModel(Assets* assets, AssetID id, Vertex* vertexs, u32* indices, u32 vertexCount,
                               u32 indexCount)
{
    PlatformAPI* platform = assets->platform;
    Arena*       arena    = &assets->arena;

    char* assetFilename = assetFilenames[id];
    platform->Logf("Exporting model '%s'", assetFilename);

    // TODO: Temp arenas
    AssetFileHeader*      header      = PushStruct(arena, AssetFileHeader);
    AssetModelFileHeader* modelHeader = PushStruct(arena, AssetModelFileHeader);
    Vertex*               vertexs2    = PushArray(arena, vertexCount, Vertex);
    u32*                  indices2    = PushArray(arena, indexCount, u32);

    header->type              = AssetType_Model;
    modelHeader->vertexCount  = vertexCount;
    modelHeader->indicesCount = indexCount;
    memcpy(vertexs2, vertexs, sizeof(Vertex) * vertexCount);
    memcpy(indices2, indices, sizeof(u32) * indexCount);

    u8* beginFileContent = (u8*)header;
    platform->FileWriteEntire(assetFilename, beginFileContent, (arena->ptr) - beginFileContent);
}

internal void AssetExportOBJ(Obj* obj, PlatformAPI* platform, char* filename)
{
    FILE* file = fopen(filename, "wb");
    Assert(file);

    AssetFileHeader      header;
    AssetModelFileHeader modelHeader;
    header.type              = AssetType_Model;
    modelHeader.vertexCount  = obj->vertexCount;
    modelHeader.indicesCount = obj->indexCount;

    fwrite(&header, sizeof(header), 1, file);
    fwrite(&modelHeader, sizeof(modelHeader), 1, file);
    fwrite(obj->vertexs, sizeof(Vertex), obj->vertexCount, file);
    fwrite(obj->indices, sizeof(u32), obj->indexCount, file);

    fclose(file);
}

internal void AssetExportGLTF(GLTFModel* gltf, PlatformAPI* platform, char* filename)
{
    Assert(gltf->meshes.size() == 1);
    Assert(gltf->meshes[0].primitives.size() == 1);

    GLTFMeshPrimitive* primitive = &gltf->meshes[0].primitives[0];

    FILE* file = fopen(filename, "wb");
    Assert(file);

    AssetFileHeader      header;
    AssetModelFileHeader modelHeader;
    header.type              = AssetType_Model;
    modelHeader.vertexCount  = (u32)primitive->vertexs.size();
    modelHeader.indicesCount = (u32)primitive->indices.size();

    fwrite(&header, sizeof(header), 1, file);
    fwrite(&modelHeader, sizeof(modelHeader), 1, file);
    fwrite(primitive->vertexs.data(), sizeof(Vertex), primitive->vertexs.size(), file);
    fwrite(primitive->indices.data(), sizeof(u32), primitive->indices.size(), file);

    fclose(file);
}