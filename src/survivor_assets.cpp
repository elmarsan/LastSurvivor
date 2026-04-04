enum AssetType
{
    AssetType_Model     = 1,
    AssetType_Font      = 2,
    AssetType_Sfx       = 3,
    AssetType_Texture   = 4,
    AssetType_Animation = 5,
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
    b32 skinned;
};

struct AssetAnimationFileHeader
{
    char name[256];
    f32  duration;
    u32  channelCount;
    u32  samplerCount;
};

// clang-format off
internal char* assetFilenames[AssetID_Count] = {
    "../data/stickman.svv",
    "../data/fence.svv",
    "../data/zombie_Female_A.svv",
    "../data/zombie_Male_A.svv",
    "../data/zombie_diffuse.svv",
    "../data/crosshairs.svv",
    "../data/fence_diffuse.svv",
	"../data/zombie_male_attack_left.svv"
};
// clang-format on

void AssetsInit(Assets* assets, Arena* baseArena, Renderer* renderer, PlatformAPI* platform)
{
    SubArena(&assets->arena, baseArena, Megabytes(10));
    assets->platform = platform;
    assets->renderer = renderer;
}

void AssetsLoad(Assets* assets, AssetID id)
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

                if (modelHeader.skinned)
                {
                    u32 jointCount;
                    StreamRead(&stream, &jointCount, sizeof(jointCount), 1);

                    model->skeleton             = PushStruct(arena, Skeleton);
                    model->skeleton->joints     = PushArray(arena, jointCount, Joint);
                    model->skeleton->jointCount = jointCount;

                    StreamRead(&stream, model->skeleton->joints, sizeof(Joint), jointCount);
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
            case AssetType_Animation:
            {
                u32 animationId = id - AssetID_ZombieAttackLeftAnimation;
                Assert(assets->animations[animationId] == 0);

                assets->animations[animationId] = PushStruct(arena, Animation);
                Animation* animation            = assets->animations[animationId];

                AssetAnimationFileHeader header;
                StreamRead(&stream, &header, sizeof(header), 1);

                strcpy(animation->name, header.name);
                animation->duration     = header.duration;
                animation->channelCount = header.channelCount;
                animation->samplerCount = header.samplerCount;

                animation->samplers = PushArray(arena, animation->samplerCount, AnimationSampler);
                animation->channels = PushArray(arena, animation->channelCount, AnimationChannel);

                for (u32 samplerIndex = 0; samplerIndex < animation->samplerCount; samplerIndex++)
                {
                    AnimationSampler* sampler = animation->samplers + samplerIndex;

                    StreamRead(&stream, &sampler->count, sizeof(sampler->count), 1);

                    f32*       times           = PushArray(arena, sampler->count, f32);
                    glm::vec4* transformations = PushArray(arena, sampler->count, glm::vec4);

                    StreamRead(&stream, times, sizeof(f32), sampler->count);
                    StreamRead(&stream, transformations, sizeof(glm::vec4), sampler->count);

                    sampler->times           = times;
                    sampler->transformations = transformations;
                }

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

Model* AssetsModelGet(Assets* assets, AssetID id)
{
    if (id > AssetID_ZombieMaleA)
    {
        assets->platform->Logf("Invalid model id");
        Assert(0);
    }

    return assets->models[id];
}

Texture* AssetsTextureGet(Assets* assets, AssetID id)
{
    if (id < AssetID_ZombieTexture || id > AssetID_FenceTexture)
    {
        assets->platform->Logf("Invalid texture id");
        Assert(0);
    }

    return assets->textures[id - AssetID_ZombieTexture];
}

void AssetExportModel(Assets* assets, AssetID id, Vertex* vertexs, u32* indices, u32 vertexCount, u32 indexCount,
                      Skeleton* skeleton)
{
    PlatformAPI* platform = assets->platform;
    Arena*       arena    = &assets->arena;

    char* assetFilename = assetFilenames[id];
    platform->Logf("Exporting model '%s'", assetFilename);

    TemporaryMemory tempMemory = TemporaryMemoryBegin(arena);
    {
        AssetFileHeader*      header      = PushStruct(arena, AssetFileHeader);
        AssetModelFileHeader* modelHeader = PushStruct(arena, AssetModelFileHeader);
        Vertex*               vertexs2    = PushArray(arena, vertexCount, Vertex);
        u32*                  indices2    = PushArray(arena, indexCount, u32);

        header->type              = AssetType_Model;
        modelHeader->vertexCount  = vertexCount;
        modelHeader->indicesCount = indexCount;
        modelHeader->skinned      = skeleton ? true : false;

        memcpy(vertexs2, vertexs, sizeof(Vertex) * vertexCount);
        memcpy(indices2, indices, sizeof(u32) * indexCount);

        if (skeleton)
        {
            u32*   jointCount = PushStruct(arena, u32);
            Joint* joints     = PushArray(arena, skeleton->jointCount, Joint);

            *jointCount = skeleton->jointCount;
            for (u32 jointIndex = 0; jointIndex < skeleton->jointCount; jointIndex++)
            {
                Joint* joint = joints + jointIndex;

                memcpy(joint, &skeleton->joints[jointIndex], sizeof(Joint));
            }
        }

        u8* beginFileContent = (u8*)header;
        platform->FileWriteEntire(assetFilename, beginFileContent, (arena->ptr) - beginFileContent);
    }
    TemporaryMemoryEnd(tempMemory);
}

void AssetExportAnimation(Assets* assets, AssetID id, Animation* animation)
{
    Arena*       arena    = &assets->arena;
    PlatformAPI* platform = assets->platform;

    char* filename = assetFilenames[id];
    platform->Logf("Exporting animation '%s'", filename);

    TemporaryMemory tempMemory = TemporaryMemoryBegin(arena);
    {
        u32 channelCount = animation->channelCount;
        u32 samplerCount = animation->samplerCount;

        AssetFileHeader*          header     = PushStruct(arena, AssetFileHeader);
        AssetAnimationFileHeader* animHeader = PushStruct(arena, AssetAnimationFileHeader);

        header->type = AssetType_Animation;
        strcpy(animHeader->name, animation->name);
        animHeader->duration     = animation->duration;
        animHeader->channelCount = channelCount;
        animHeader->samplerCount = samplerCount;

        // Samplers
        for (u32 samplerIndex = 0; samplerIndex < animation->samplerCount; samplerIndex++)
        {
            AnimationSampler* inSampler = animation->samplers + samplerIndex;

            u32*       count           = PushStruct(arena, u32);
            f32*       times           = PushArray(arena, inSampler->count, f32);
            glm::vec4* transformations = PushArray(arena, inSampler->count, glm::vec4);

            *count = inSampler->count;
            memcpy(times, inSampler->times, sizeof(f32) * inSampler->count);
            memcpy(transformations, inSampler->transformations, sizeof(glm::vec4) * inSampler->count);
        }

        // Channels
        AnimationChannel* channels = PushArray(arena, channelCount, AnimationChannel);
        memcpy(channels, animation->channels, sizeof(AnimationChannel) * channelCount);

        u8* beginFileContent = (u8*)header;
        platform->FileWriteEntire(filename, beginFileContent, (arena->ptr) - beginFileContent);
    }
    TemporaryMemoryEnd(tempMemory);
}