void AssetsInit(Assets* assets, Arena* baseArena, Renderer* renderer, PlatformAPI* platform)
{
    SubArena(&assets->arena, baseArena, Megabytes(10));
    assets->platform = platform;
    assets->renderer = renderer;
}

void AssetsLoad(Assets* assets, u32 id)
{
    PlatformAPI* platform = assets->platform;
    Arena*       arena    = &assets->arena;
    Renderer*    renderer = assets->renderer;

    char* assetFilename = assetFilenames[id];
    platform->Logf("Loading asset '%s'", assetFilename);

    if (id == Texture_Zombie || id == Texture_Crosshair || id == Texture_Fence)
    {
        u32 textureId               = id - Texture_Zombie;
        assets->textures[textureId] = PushStruct(arena, Texture);
        TextureInit(renderer, assets->textures[textureId], assetFilename);
    }
    else
    {
        FileReadResult file = platform->FileReadEntire(assetFilename);
        if (file.content)
        {
            Stream stream = StreamInit((u8*)file.content);

            AssetFileHeader assetHeader;
            StreamRead(&stream, &assetHeader, sizeof(assetHeader), 1);

            switch (assetHeader.type)
            {
            case AssetType_Model:
            {
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
                if (id == Model_Fence)
                {
                    model->aabb.max   = model->aabb.max;
                    model->aabb.min   = -model->aabb.max;
                    model->aabb.min.y = model->aabb.min.y;
                }

                if (modelHeader.skinned)
                {
                    u32 jointCount;
                    StreamRead(&stream, &jointCount, sizeof(jointCount), 1);

                    model->skeleton                      = PushStruct(arena, Skeleton);
                    model->skeleton->joints              = PushArray(arena, jointCount, Joint);
                    model->skeleton->jointMatrices       = PushArray(arena, jointCount, glm::mat4);
                    model->skeleton->jointIndexBindOrder = PushArray(arena, jointCount - 1, u32);
                    model->skeleton->jointCount          = jointCount;

                    StreamRead(&stream, model->skeleton->joints, sizeof(Joint), jointCount);
                    StreamRead(&stream, model->skeleton->jointIndexBindOrder, sizeof(u32), jointCount - 1);
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
                GPUBufferVertexAttrib(renderer, model->gpuBuffer, 3, 4, GL_UNSIGNED_INT, sizeof(Vertex),
                                      offsetof(Vertex, joints));
                GPUBufferVertexAttrib(renderer, model->gpuBuffer, 4, 4, GL_FLOAT, sizeof(Vertex),
                                      offsetof(Vertex, weights));

                break;
            }
            case AssetType_Animation:
            {
                u32 animationId = id - Anim_ZombieMaleAttackLeft;

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

                StreamRead(&stream, animation->channels, sizeof(AnimationChannel), animation->channelCount);

                break;
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

Model* AssetsModelGet(Assets* assets, u32 id)
{
    Assert(id >= 0 && id <= Model_Fence);
    return assets->models[id];
}

Texture* AssetsTextureGet(Assets* assets, u32 id)
{
    Assert(id >= Texture_Zombie && id <= Texture_Fence);
    return assets->textures[id - Texture_Zombie];
}

Animation* AssetsAnimationGet(Assets* assets, u32 id)
{
    Assert(id >= 0 && id < AssetCount);
    return assets->animations[id - Anim_ZombieMaleAttackLeft];
}

internal void SkeletonComputeJointMatrices(Skeleton* skeleton, u32 fromJointIndex, glm::mat4 parent)
{
    Joint* joint = skeleton->joints + fromJointIndex;

    glm::mat4 local = glm::translate(glm::mat4(1.0f), joint->translation) * glm::mat4_cast(joint->rotation) *
                      glm::scale(glm::mat4(1.0f), joint->scale);

    glm::mat4 global                        = parent * local;
    skeleton->jointMatrices[fromJointIndex] = global * joint->inverseBindMatrix;

    for (u32 childrenIndex = 0; childrenIndex < joint->childrenCount; childrenIndex++)
    {
        SkeletonComputeJointMatrices(skeleton, joint->childrenIndexes[childrenIndex], global);
    }
}

void SkeletonUpdatePose(Skeleton* skeleton) { SkeletonComputeJointMatrices(skeleton, 0, glm::mat4{ 1.0f }); }

void SkeletonApplyAnimation(Skeleton* skeleton, Animation* animation, f32 time)
{
    for (u32 channelIndex = 0; channelIndex < animation->channelCount; channelIndex++)
    {
        AnimationChannel* channel = animation->channels + channelIndex;
        AnimationSampler* sampler = animation->samplers + channel->samplerIndex;
        Joint*            joint   = skeleton->joints + channel->jointIndex;

        for (u32 timeIndex = 0; timeIndex + 1 < sampler->count; timeIndex++)
        {
            f32 t0 = sampler->times[timeIndex];
            f32 t1 = sampler->times[timeIndex + 1];

            if (time >= t0 && time <= t1)
            {
                f32 a = (time - t0) / (t1 - t0);

                glm::vec4 v0 = sampler->transformations[timeIndex];
                glm::vec4 v1 = sampler->transformations[timeIndex + 1];

                if (channel->path == AnimationChannelPath_Rotation)
                {
                    glm::quat q0{ v0.x, v0.y, v0.z, v0.w };
                    glm::quat q1{ v1.x, v1.y, v1.z, v1.w };
                    joint->rotation = glm::normalize(glm::slerp(q0, q1, a));
                }
                else if (channel->path == AnimationChannelPath_Translation)
                {
                    joint->translation = glm::mix(v0, v1, a);
                }
                else if (channel->path == AnimationChannelPath_Scale)
                {
                    joint->scale = glm::mix(v0, v1, a);
                }

                break;
            }
        }
    }
}