#include <windows.h>

#include "../survivor.h"
#include "../platform_core.h"
#include "gltf.h"
#include "obj.h"

struct Export
{
    char* inputFilename;
    char* outputFilename;
};

Export exports[AssetCount] = {
    // Models
    { "ZombieMale_A_joined.gltf", assetFilenames[Model_ZombieMaleA] },
    { "ZombieFemale_A_joined.gltf", assetFilenames[Model_ZombieFemaleA] },
    { "stickman.obj", assetFilenames[Model_Stickman] },
    { "fence.obj", assetFilenames[Model_Fence] },
    { "chainlink_fence.gltf", assetFilenames[Model_ChainlinkFence] },

    // Textures
    { "zcolors.png", assetFilenames[Texture_Zombie] },
    { "crosshairs.png", assetFilenames[Texture_Crosshair] },
    { "WoodPlanksOld0242_7_S.jpg", assetFilenames[Texture_Fence] },
    { "M_Fencing_baseColor.png", assetFilenames[Texture_ChainlinkFence] },

    // Zombie male animations
    { "ZombieMale@attack_left_70f.gltf", assetFilenames[Anim_ZombieMaleAttackLeft] },
    { "ZombieMale@attack_right_70f.gltf", assetFilenames[Anim_ZombieMaleAttackRight] },
    { "ZombieMale@idle_220f.gltf", assetFilenames[Anim_ZombieMaleIdle] },
    { "ZombieMale@idle_alert_120f.gltf", assetFilenames[Anim_ZombieMaleIdleAlert] },
    { "ZombieMale@idle2_220f.gltf", assetFilenames[Anim_ZombieMaleIdle2] },
    { "ZombieMale@running_58f.gltf", assetFilenames[Anim_ZombieMaleRunning] },
    { "ZombieMale@slowWalk_85f.gltf", assetFilenames[Anim_ZombieMaleSlowWalk] },
    { "ZombieMale@walk_64f.gltf", assetFilenames[Anim_ZombieMaleWalk] },
    { "ZombieMale@walk_agressive_64f.gltf", assetFilenames[Anim_ZombieMaleWalkAgressive] },
    { "ZombieMale@walk_limp_60f.gltf", assetFilenames[Anim_ZombieMaleWalkLimp] },
    { "ZombieMale@crawling_forward_90f.gltf", assetFilenames[Anim_ZombieMaleCrawlingForward] },
    { "ZombieMale@crawling_idle_220f.gltf", assetFilenames[Anim_ZombieMaleCrawlingIdle] },

    // Zombie female animations
    { "ZombieFemale@attack_left_70f.gltf", assetFilenames[Anim_ZombieFemaleAttackLeft] },
    { "ZombieFemale@attack_right_70f.gltf", assetFilenames[Anim_ZombieFemaleAttackRight] },
    { "ZombieFemale@idle_220f.gltf", assetFilenames[Anim_ZombieFemaleIdle] },
    { "ZombieFemale@idle_alert_120f.gltf", assetFilenames[Anim_ZombieFemaleIdleAlert] },
    { "ZombieFemale@idle2_220f.gltf", assetFilenames[Anim_ZombieFemaleIdle2] },
    { "ZombieFemale@running_58f.gltf", assetFilenames[Anim_ZombieFemaleRunning] },
    { "ZombieFemale@slowWalk_85f.gltf", assetFilenames[Anim_ZombieFemaleSlowWalk] },
    { "ZombieFemale@walk_64f.gltf", assetFilenames[Anim_ZombieFemaleWalk] },
    { "ZombieFemale@walk_agressive_64f.gltf", assetFilenames[Anim_ZombieFemaleWalkAgressive] },
    { "ZombieFemale@walk_limp_60f.gltf", assetFilenames[Anim_ZombieFemaleWalkLimp] },
    { "ZombieFemale@crawling_forward_90f.gltf", assetFilenames[Anim_ZombieFemaleCrawlingForward] },
    { "ZombieFemale@crawling_idle_220f.gltf", assetFilenames[Anim_ZombieFemaleCrawlingIdle] },
};

void ExportModel(PlatformAPI* platform, Arena* arena, char* filename, Vertex* vertexs, u32* indices, u32 vertexCount,
                 u32 indexCount, Skeleton* skeleton)
{
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
            u32*   jointCount          = PushStruct(arena, u32);
            Joint* joints              = PushArray(arena, skeleton->jointCount, Joint);
            u32*   jointIndexBindOrder = PushArray(arena, skeleton->jointCount - 1, u32);

            *jointCount = skeleton->jointCount;
            for (u32 jointIndex = 0; jointIndex < skeleton->jointCount; jointIndex++)
            {
                Joint* joint = joints + jointIndex;

                memcpy(joint, &skeleton->joints[jointIndex], sizeof(Joint));
            }

            memcpy(jointIndexBindOrder, skeleton->jointIndexBindOrder, sizeof(u32) * skeleton->jointCount - 1);
        }

        u8* beginFileContent = (u8*)header;
        platform->FileWriteEntire(filename, beginFileContent, (arena->ptr) - beginFileContent);
    }
    TemporaryMemoryEnd(tempMemory);
}

int main(int argc, char** argv)
{
    size_t chunkSize = (size_t)Megabytes(16);
    void*  chunk     = VirtualAlloc(NULL, chunkSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!chunk)
    {
        Log("VirtualAlloc error: %d", GetLastError());
    }

    Arena arena;
    ArenaInit(&arena, chunkSize, chunk);

    PlatformAPI platform;
    platform.Logf            = LogMsg;
    platform.FileReadEntire  = FileReadEntire;
    platform.FileFree        = FileFree;
    platform.FileWriteEntire = FileWriteEntire;

    char zombieMaleAFilename[256]   = "../data/original/";
    char zombieFemaleAFilename[256] = "../data/original/";
    strcat(zombieMaleAFilename, exports[Model_ZombieMaleA].inputFilename);
    strcat(zombieFemaleAFilename, exports[Model_ZombieFemaleA].inputFilename);

    GLTFModel zombieMaleA   = GLTFParse(zombieMaleAFilename, &platform);
    GLTFModel zombieFemaleA = GLTFParse(zombieFemaleAFilename, &platform);

    for (u32 exportIndex = 0; exportIndex < ArrayCount(exports); exportIndex++)
    {
        Export* exportAsset        = exports + exportIndex;
        char*   extension          = GetFilenameExtension(exportAsset->inputFilename);
        char    assetFilepath[256] = "../data/original/";
        strcat(assetFilepath, exportAsset->inputFilename);

        platform.Logf("Exporting '%s' from '%s'", exportAsset->outputFilename, exportAsset->inputFilename);

        // Export animation
        if (exportIndex >= MODEL_COUNT + TEXTURE_COUNT)
        {
            if (StrEquals(extension, "gltf"))
            {
                GLTFModel* model = &zombieMaleA;
                if (exportIndex >= Anim_ZombieFemaleAttackLeft)
                {
                    model = &zombieFemaleA;
                }

                TemporaryMemory tempMemory = TemporaryMemoryBegin(&arena);
                {
                    std::vector<GLTFAnimation> gltfAnimations = GLTFParseAnimations(assetFilepath, &platform);
                    Assert(gltfAnimations.size() == 1);
                    Animation* animation = GLTFConvertAnimation(model, &gltfAnimations[0], &arena);

                    u32 channelCount = animation->channelCount;
                    u32 samplerCount = animation->samplerCount;

                    AssetFileHeader*          header     = PushStruct(&arena, AssetFileHeader);
                    AssetAnimationFileHeader* animHeader = PushStruct(&arena, AssetAnimationFileHeader);

                    header->type = AssetType_Animation;
                    strcpy(animHeader->name, animation->name);
                    animHeader->duration     = animation->duration;
                    animHeader->channelCount = channelCount;
                    animHeader->samplerCount = samplerCount;

                    // Samplers
                    for (u32 samplerIndex = 0; samplerIndex < animation->samplerCount; samplerIndex++)
                    {
                        AnimationSampler* inSampler = animation->samplers + samplerIndex;

                        u32*       count           = PushStruct(&arena, u32);
                        f32*       times           = PushArray(&arena, inSampler->count, f32);
                        glm::vec4* transformations = PushArray(&arena, inSampler->count, glm::vec4);

                        *count = inSampler->count;
                        memcpy(times, inSampler->times, sizeof(f32) * inSampler->count);
                        memcpy(transformations, inSampler->transformations, sizeof(glm::vec4) * inSampler->count);
                    }

                    // Channels
                    AnimationChannel* channels = PushArray(&arena, channelCount, AnimationChannel);
                    memcpy(channels, animation->channels, sizeof(AnimationChannel) * channelCount);

                    u8* beginFileContent = (u8*)header;
                    platform.FileWriteEntire(exportAsset->outputFilename, beginFileContent,
                                             arena.ptr - beginFileContent);
                }
                TemporaryMemoryEnd(tempMemory);
            }
            else
            {
                InvalidCodePath;
            }
        }
        // Export model
        else if (exportIndex < MODEL_COUNT)
        {
            if (StrEquals(extension, "gltf"))
            {
                GLTFModel model = GLTFParse(assetFilepath, &platform);
                Assert(model.meshes.size() == 1);
                Assert(model.meshes[0].primitives.size() == 1);
                GLTFMeshPrimitive* primitive = &model.meshes[0].primitives[0];
                Skeleton*          skeleton  = GLTFConvertSkeleton(&model, &arena);

                ExportModel(&platform, &arena, exportAsset->outputFilename, primitive->vertexs.data(),
                            primitive->indices.data(), (u32)primitive->vertexs.size(), (u32)primitive->indices.size(),
                            skeleton);
            }
            else if (StrEquals(extension, "obj"))
            {
                Obj obj = ObjParse(assetFilepath, &platform, &arena);
                ExportModel(&platform, &arena, exportAsset->outputFilename, obj.vertexs.data(), obj.indices.data(),
                            (u32)obj.vertexs.size(), (u32)obj.indices.size(), 0);
            }
            else
            {
                platform.Logf("Invalid file extension: '%s'", extension);
                InvalidCodePath;
            }
        }
        // Export texture
        else
        {
            // TODO
        }
    }
}