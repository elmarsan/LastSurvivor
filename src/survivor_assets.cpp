internal void Assets_ReadAnimation(PlatformAPI* platform, Stream* stream, Animation* animation, Arena* arena)
{
    AssetFileHeader          header;
    AssetAnimationFileHeader animHeader;
    StreamRead(stream, &header, sizeof(header), 1);
    StreamRead(stream, &animHeader, sizeof(animHeader), 1);

    strcpy(animation->name, animHeader.name);
    platform->Logf("Reading animation %s ...", animation->name);

    animation->duration     = animHeader.duration;
    animation->channelCount = animHeader.channelCount;
    animation->samplerCount = animHeader.samplerCount;
    animation->samplers     = PushArray(arena, animation->samplerCount, AnimationSampler);
    animation->channels     = PushArray(arena, animation->channelCount, AnimationChannel);

    for (u32 samplerIndex = 0; samplerIndex < animation->samplerCount; samplerIndex++)
    {
        AnimationSampler* sampler = animation->samplers + samplerIndex;

        // Note: read in the following order
        // 1. Sampler count
        // 2. Interpolation type (step or linear)
        // 3. Sampler times
        // 3. Sampler transformations
        StreamRead(stream, &sampler->count, sizeof(sampler->count), 1);

        sampler->times           = PushArray(arena, sampler->count, f32);
        sampler->transformations = PushArray(arena, sampler->count, glm::vec4);

        StreamRead(stream, &sampler->interpolation, sizeof(sampler->interpolation), 1);
        StreamRead(stream, sampler->times, sizeof(f32), sampler->count);
        StreamRead(stream, sampler->transformations, sizeof(glm::vec4), sampler->count);
    }

    StreamRead(stream, animation->channels, sizeof(AnimationChannel), animation->channelCount);
}

void Assets_Init(Assets* assets, Arena* baseArena, Renderer* renderer, PlatformAPI* platform)
{
    SubArena(&assets->arena, baseArena, Megabytes(20));
    assets->platform = platform;
    assets->renderer = renderer;
}

// TODO: Load zombie texture only once
void Assets_Load(Assets* assets, Asset id)
{
    PlatformAPI* platform = assets->platform;
    Arena*       arena    = &assets->arena;
    Renderer*    renderer = assets->renderer;

    char* assetFilename = assetFilenames[id];
    platform->Logf("Loading asset '%s'", assetFilename);

    if (id >= Texture_Crosshair && id < Anim_ZombieMaleAttackLeft)
    {
        u32 textureId               = id - Texture_Crosshair;
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

                model->textureCount     = modelHeader.textureCount;
                model->meshCount        = modelHeader.meshCount;
                model->materialCount    = modelHeader.materialCount;
                model->animationCount   = modelHeader.animationCount;
                model->localTranslation = modelHeader.localTranslation;
                model->localRotation    = modelHeader.localRotation;
                model->localScale       = modelHeader.localScale;

                // Meshes
                model->meshes = PushArray(arena, model->meshCount, Mesh);
                for (u32 meshIndex = 0; meshIndex < model->meshCount; meshIndex++)
                {
                    AssetMeshHeader meshHeader;
                    StreamRead(&stream, &meshHeader, sizeof(meshHeader), 1);

                    Mesh* mesh = model->meshes + meshIndex;

                    mesh->vertexCount   = meshHeader.vertexCount;
                    mesh->indicesCount  = meshHeader.indicesCount;
                    mesh->materialIndex = meshHeader.materialIndex;
                    mesh->vertexs       = PushArray(arena, mesh->vertexCount, Vertex);
                    mesh->indices       = PushArray(arena, mesh->indicesCount, u32);

                    StreamRead(&stream, mesh->vertexs, sizeof(Vertex), mesh->vertexCount);
                    StreamRead(&stream, mesh->indices, sizeof(u32), mesh->indicesCount);

                    mesh->gpuBuffer = PushStruct(arena, GPUBuffer);

                    GPUBufferInit(renderer, mesh->gpuBuffer);
                    GPUBufferVBOAlloc(renderer, mesh->gpuBuffer, mesh->vertexs, sizeof(Vertex) * mesh->vertexCount,
                                      sizeof(Vertex), GL_STATIC_DRAW);
                    GPUBufferEBOAlloc(renderer, mesh->gpuBuffer, mesh->indices, sizeof(u32) * mesh->indicesCount,
                                      sizeof(u32), GL_STATIC_DRAW);

                    GPUBufferVertexAttrib(renderer, mesh->gpuBuffer, 0, 3, GL_FLOAT, sizeof(Vertex),
                                          offsetof(Vertex, position));
                    GPUBufferVertexAttrib(renderer, mesh->gpuBuffer, 1, 3, GL_FLOAT, sizeof(Vertex),
                                          offsetof(Vertex, normal));
                    GPUBufferVertexAttrib(renderer, mesh->gpuBuffer, 2, 2, GL_FLOAT, sizeof(Vertex),
                                          offsetof(Vertex, uv));
                    if (modelHeader.skinned)
                    {
                        GPUBufferVertexAttrib(renderer, mesh->gpuBuffer, 3, 4, GL_UNSIGNED_INT, sizeof(Vertex),
                                              offsetof(Vertex, joints));
                        GPUBufferVertexAttrib(renderer, mesh->gpuBuffer, 4, 4, GL_FLOAT, sizeof(Vertex),
                                              offsetof(Vertex, weights));
                    }
                }

                // Skeleton
                if (modelHeader.skinned)
                {
                    u32 jointCount;
                    StreamRead(&stream, &jointCount, sizeof(jointCount), 1);

                    model->skeleton                      = PushStruct(arena, Skeleton);
                    model->skeleton->joints              = PushArray(arena, jointCount, Joint);
                    model->skeleton->jointSkinMatrices   = PushArray(arena, jointCount, glm::mat4);
                    model->skeleton->jointGlobalMatrices = PushArray(arena, jointCount, glm::mat4);
                    model->skeleton->jointIndexBindOrder = PushArray(arena, jointCount, u32);

                    model->skeleton->jointCount = jointCount;

                    StreamRead(&stream, model->skeleton->joints, sizeof(Joint), jointCount);
                    StreamRead(&stream, model->skeleton->jointIndexBindOrder, sizeof(u32), jointCount);
                }

                // Textures
                if (model->textureCount > 0)
                {
                    model->textures = PushArray(arena, model->textureCount, Texture);

                    for (u32 textureIndex = 0; textureIndex < model->textureCount; textureIndex++)
                    {
                        AssetTextureHeader textureHeader;
                        StreamRead(&stream, &textureHeader, sizeof(textureHeader), 1);

                        Texture* texture = model->textures + textureIndex;
                        strcpy(texture->name, textureHeader.name);
                        TextureAlloc(renderer, texture, (void*)stream.ptr, textureHeader.size);
                        StreamSkip(&stream, textureHeader.size);
                    }
                }

                // Materials
                if (model->materialCount > 0)
                {
                    model->materials = PushArray(arena, model->materialCount, Material);
                    StreamRead(&stream, model->materials, sizeof(Material), model->materialCount);
                }

                // Animations
                if (model->animationCount > 0)
                {
                    model->animations = PushArray(arena, model->animationCount, Animation);

                    for (u32 animationIndex = 0; animationIndex < modelHeader.animationCount; animationIndex++)
                    {
                        Animation* animation = model->animations + animationIndex;
                        Assets_ReadAnimation(platform, &stream, animation, arena);
                        int x = 10;
                    }
                }

                break;
            }
            case AssetType_Animation:
            {
                u32 animationId = id - Anim_ZombieMaleAttackLeft;

                assets->animations[animationId] = PushStruct(arena, Animation);
                Animation* animation            = assets->animations[animationId];
                // Assets_ReadAnimation(platform, &stream, animation, arena);

                // TODO: Duplicated snippet
                // AssetFileHeader          header;
                AssetAnimationFileHeader animHeader;
                // StreamRead(stream, &header, sizeof(header), 1);
                StreamRead(&stream, &animHeader, sizeof(animHeader), 1);

                strcpy(animation->name, animHeader.name);
                platform->Logf("Reading animation %s ...", animation->name);

                animation->duration     = animHeader.duration;
                animation->channelCount = animHeader.channelCount;
                animation->samplerCount = animHeader.samplerCount;
                animation->samplers     = PushArray(arena, animation->samplerCount, AnimationSampler);
                animation->channels     = PushArray(arena, animation->channelCount, AnimationChannel);

                for (u32 samplerIndex = 0; samplerIndex < animation->samplerCount; samplerIndex++)
                {
                    AnimationSampler* sampler = animation->samplers + samplerIndex;

                    // Note: read in the following order
                    // 1. Sampler count
                    // 2. Interpolation type (step or linear)
                    // 3. Sampler times
                    // 3. Sampler transformations
                    StreamRead(&stream, &sampler->count, sizeof(sampler->count), 1);

                    sampler->times           = PushArray(arena, sampler->count, f32);
                    sampler->transformations = PushArray(arena, sampler->count, glm::vec4);

                    StreamRead(&stream, &sampler->interpolation, sizeof(sampler->interpolation), 1);
                    StreamRead(&stream, sampler->times, sizeof(f32), sampler->count);
                    StreamRead(&stream, sampler->transformations, sizeof(glm::vec4), sampler->count);
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

Model* Assets_GetModel(Assets* assets, Asset id)
{
    Assert(id >= 0 && id <= MODEL_COUNT - 1);
    return assets->models[id];
}

Texture* Assets_GetTexture(Assets* assets, Asset id)
{
    Assert(id >= Texture_Crosshair && id < Anim_ZombieMaleAttackLeft);
    return assets->textures[id - Texture_Crosshair];
}

Animation* Assets_GetAnimation(Assets* assets, Asset id)
{
    Assert(id >= 0 && id < AssetCount);
    return assets->animations[id - Anim_ZombieMaleAttackLeft];
}

internal void SkeletonComputeJointMatrices(Skeleton* skeleton, u32 fromJointIndex, glm::mat4 parent)
{
    Joint* joint = skeleton->joints + fromJointIndex;

    // Local transform (joint space relative to parent joint)
    glm::mat4 local = glm::translate(glm::mat4(1.0f), joint->translation) * glm::mat4_cast(joint->rotation) *
                      glm::scale(glm::mat4(1.0f), joint->scale);

    // Global transform (joint space -> model space, in bind/animated pose)
    glm::mat4 global = parent * local;

    // Skinning matrix:
    // transforms from model space into current joint space AND applies animation
    skeleton->jointSkinMatrices[fromJointIndex] = global * joint->inverseBindMatrix;

    // IMPORTANT: store world matrix for debug
    skeleton->jointGlobalMatrices[fromJointIndex] = global;

    for (u32 childrenIndex = 0; childrenIndex < joint->childrenCount; childrenIndex++)
    {
        SkeletonComputeJointMatrices(skeleton, joint->childrenIndexes[childrenIndex], global);
    }
}

// Note: This might ignore some joints (it's ok for zombies)
// TODO: Compute joint matrices from every joint with no parent
void Skeleton_UpdatePose(Skeleton* skeleton) { SkeletonComputeJointMatrices(skeleton, 0, glm::mat4{ 1.0f }); }

void Skeleton_ApplyAnimation(Skeleton* skeleton, Animation* animation, f32 time)
{
    for (u32 channelIndex = 0; channelIndex < animation->channelCount; channelIndex++)
    {
        AnimationChannel* channel = animation->channels + channelIndex;
        AnimationSampler* sampler = animation->samplers + channel->samplerIndex;
        Joint*            joint   = skeleton->joints + channel->jointIndex;

        if (channel->jointIndex >= skeleton->jointCount)
        {
            continue;
        }

        for (u32 timeIndex = 0; timeIndex + 1 < sampler->count; timeIndex++)
        {
            f32 t0 = sampler->times[timeIndex];
            f32 t1 = sampler->times[timeIndex + 1];

            if (time >= t0 && time <= t1)
            {
                switch (sampler->interpolation)
                {
                case AnimationSamplerInterpolation_Linear:
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
                case AnimationSamplerInterpolation_Step:
                {
                    glm::vec4 v0 = sampler->transformations[timeIndex];

                    if (channel->path == AnimationChannelPath_Rotation)
                    {
                        joint->rotation = glm::quat{ v0.x, v0.y, v0.z, v0.w };
                    }
                    else if (channel->path == AnimationChannelPath_Translation)
                    {
                        joint->translation = v0;
                    }
                    else if (channel->path == AnimationChannelPath_Scale)
                    {
                        joint->scale = v0;
                    }

                    break;
                }
                }

                break;
            }
        }
    }
}