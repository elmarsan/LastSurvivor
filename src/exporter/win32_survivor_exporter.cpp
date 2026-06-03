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
    { "parking.glb", assetFilenames[Model_Parking] },

    // Textures
    { "crosshairs.png", assetFilenames[Texture_Crosshair] },

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

Skeleton* GLTF_ExtractSkeleton(GLTFModel* gltfModel, Arena* arena)
{
    u32 jointCount = (u32)gltfModel->skeleton.joints.size() + 1;

    Skeleton* result            = PushStruct(arena, Skeleton);
    result->joints              = PushArray(arena, jointCount, Joint);
    result->jointIndexBindOrder = PushArray(arena, jointCount, u32);
    result->jointCount          = jointCount;

    Joint* root           = result->joints;
    u32    nextJointIndex = 1;

    for (u32 nodeIndex = 0; nodeIndex < gltfModel->nodes.size(); nodeIndex++)
    {
        for (int jointIndex : gltfModel->skeleton.joints)
        {
            // Is joints
            if ((u32)jointIndex == nodeIndex)
            {
                Joint*    joint    = result->joints + nextJointIndex;
                GLTFNode* gltfNode = &gltfModel->nodes[nodeIndex];

                JointInitFromGLTFNode(joint, gltfNode);
                nextJointIndex++;

                // Note: Zombie armature root
                if (gltfNode->parentIndex != EMPTY)
                {
                    GLTFNode* gltfParentNode = &gltfModel->nodes[gltfNode->parentIndex];
                    if (StrEquals(gltfParentNode->name, "rig_CharRoot"))
                    {
                        Assert(gltfParentNode->parentIndex == EMPTY);
                        JointInitFromGLTFNode(root, gltfParentNode);
                    }
                }

                break;
            }
        }
    }

    for (u32 jointIndex = 0; jointIndex < jointCount; jointIndex++)
    {
        Joint*    joint    = result->joints + jointIndex;
        GLTFNode* gltfNode = JointGetMappedGLTFNode(joint, gltfModel);
        Assert(gltfNode);

        std::vector<u32> childrenJoints = GLTFNodeGetJointChildrenIndexes(gltfModel, gltfNode);
        joint->childrenCount            = (u32)childrenJoints.size();
        // Log("%d", joint->childrenCount);
        Assert(joint->childrenCount <= JOINT_MAX_CHILDREN_COUNT);

        u32 nextChildIndex = 0;
        for (u32 childIndex : childrenJoints)
        {
            GLTFNode* gltfChildNode = &gltfModel->nodes[childIndex];
            Joint*    child         = GLTFNodeGetMappedJoint(result, gltfChildNode);
            Assert(child);
            s64 jointIndex = child - result->joints;

            joint->childrenIndexes[nextChildIndex++] = (u32)jointIndex;
        }
    }

    memcpy(result->jointIndexBindOrder, gltfModel->skeleton.joints.data(), sizeof(u32) * jointCount);

    return result;
}

// TODO: Asset pipeline
internal void Exporter_WriteAnimation(Animation* animation, FILE* file)
{
    // Log("Exporting animation %s ...", animation->name);

    AssetFileHeader          header;
    AssetAnimationFileHeader animHeader;

    header.type = AssetType_Animation;
    strcpy(animHeader.name, animation->name);
    animHeader.duration     = animation->duration;
    animHeader.channelCount = animation->channelCount;
    animHeader.samplerCount = animation->samplerCount;

    fwrite(&header, sizeof(header), 1, file);
    fwrite(&animHeader, sizeof(animHeader), 1, file);

    // Samplers
    for (u32 samplerIndex = 0; samplerIndex < animation->samplerCount; samplerIndex++)
    {
        AnimationSampler* inSampler = animation->samplers + samplerIndex;

        fwrite(&inSampler->count, sizeof(inSampler->count), 1, file);
        fwrite(&inSampler->interpolation, sizeof(inSampler->interpolation), 1, file);
        fwrite(inSampler->times, sizeof(f32), inSampler->count, file);
        fwrite(inSampler->transformations, sizeof(glm::vec4), inSampler->count, file);
    }

    // Channels
    fwrite(animation->channels, sizeof(AnimationChannel), animHeader.channelCount, file);
}

void Exporter_GLTFModel(Arena* arena, char* filename, GLTFModel* gltfModel)
{
    FILE* file = fopen(filename, "wb");
    if (file)
    {
        // Extract meshes
        std::vector<GLTFMeshPrimitive> primitives{};
        for (u32 meshIndex = 0; meshIndex < gltfModel->meshes.size(); meshIndex++)
        {
            GLTFMesh* gltfMesh = &gltfModel->meshes[meshIndex];

            for (u32 primitiveIndex = 0; primitiveIndex < gltfMesh->primitives.size(); primitiveIndex++)
            {
                GLTFMeshPrimitive& gltfPrimitive = gltfMesh->primitives[primitiveIndex];
                primitives.push_back(gltfPrimitive);
            }
        }

        // Model transform
        glm::vec3 localTranslation{ 0.0f, 0.0f, 0.0f };
        glm::quat localRotation{ 0.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3 localScale{ 1.0f, 1.0f, 1.0f };

        for (u32 nodeIndex = 0; nodeIndex < gltfModel->nodes.size(); nodeIndex++)
        {
            GLTFNode* gltfNode = &gltfModel->nodes[nodeIndex];
            if (gltfNode->meshIndex == EMPTY && gltfNode->parentIndex == EMPTY)
            {
                localTranslation = gltfNode->translation;
                localRotation    = gltfNode->rotation;
                localScale       = gltfNode->scale;
            }
        }

        AssetFileHeader      header;
        AssetModelFileHeader modelHeader;

        header.type                  = AssetType_Model;
        modelHeader.meshCount        = (u32)primitives.size();
        modelHeader.skinned          = !gltfModel->skeleton.joints.empty();
        modelHeader.textureCount     = (u32)gltfModel->textures.size();
        modelHeader.materialCount    = (u32)gltfModel->materials.size();
        modelHeader.animationCount   = (u32)gltfModel->animations.size();
        modelHeader.localTranslation = localTranslation;
        modelHeader.localRotation    = localRotation;
        modelHeader.localScale       = localScale;

        // Headers
        fwrite(&header, sizeof(header), 1, file);
        fwrite(&modelHeader, sizeof(modelHeader), 1, file);

        // Meshes
        for (u32 primitiveIndex = 0; primitiveIndex < primitives.size(); primitiveIndex++)
        {
            GLTFMeshPrimitive* primitive = &primitives[primitiveIndex];

            AssetMeshHeader meshHeader;
            // meshHeader.name
            meshHeader.vertexCount   = (u32)primitive->vertexs.size();
            meshHeader.indicesCount  = (u32)primitive->indices.size();
            meshHeader.materialIndex = primitive->materialIndex;

            fwrite(&meshHeader, sizeof(meshHeader), 1, file);
            fwrite(primitive->vertexs.data(), sizeof(Vertex), meshHeader.vertexCount, file);
            fwrite(primitive->indices.data(), sizeof(u32), meshHeader.indicesCount, file);
        }

        // Skeleton
        if (modelHeader.skinned)
        {
            Skeleton* skeleton = GLTF_ExtractSkeleton(gltfModel, arena);

            fwrite(&skeleton->jointCount, sizeof(skeleton->jointCount), 1, file);
            fwrite(skeleton->joints, sizeof(Joint), skeleton->jointCount, file);
            fwrite(skeleton->jointIndexBindOrder, sizeof(u32), skeleton->jointCount, file);
        }

        // Textures
        for (u32 textureIndex = 0; textureIndex < modelHeader.textureCount; textureIndex++)
        {
            GLTFTexture* gltfTexture = &gltfModel->textures[textureIndex];
            u32          textureSize = (u32)gltfTexture->data.size();

            AssetTextureHeader textureHeader;
            textureHeader.size = textureSize;
            strcpy(textureHeader.name, gltfTexture->name);

            fwrite(&textureHeader, sizeof(textureHeader), 1, file);
            fwrite(gltfTexture->data.data(), sizeof(u8), textureSize, file);
        }

        // Materials
        for (u32 materialIndex = 0; materialIndex < modelHeader.materialCount; materialIndex++)
        {
            GLTFMaterial* gltfMaterial = &gltfModel->materials[materialIndex];

            Material material;
            material.baseColorIndex = gltfMaterial->baseColorIndex;

            fwrite(&material, sizeof(material), 1, file);
        }

        // Animations
        if (modelHeader.animationCount > 0)
        {
            for (u32 animationIndex = 0; animationIndex < modelHeader.animationCount; animationIndex++)
            {
                GLTFAnimation* gltfAnimation = &gltfModel->animations[animationIndex];
                Animation*     animation     = GLTF_ConvertAnimation(gltfModel, gltfAnimation, arena);

                Exporter_WriteAnimation(animation, file);
            }
        }

        fclose(file);

        // Log("Asset exported");
    }
    else
    {
        Assert(0);
    }
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

        Log("Exporting '%s' from '%s'", exportAsset->outputFilename, exportAsset->inputFilename);

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

                FILE* file = fopen(exportAsset->outputFilename, "wb");
                if (file)
                {
                    std::vector<GLTFAnimation> gltfAnimations = GLTFParseAnimations(assetFilepath, &platform);
                    Assert(gltfAnimations.size() == 1);
                    Animation* animation = GLTF_ConvertAnimation(model, &gltfAnimations[0], &arena);

                    Exporter_WriteAnimation(animation, file);
                    fclose(file);
                }
                else
                {
                    Log("Unable to open output file");
                    Assert(0);
                }
            }
            else
            {
                InvalidCodePath;
            }
        }
        // Export model
        else if (exportIndex < MODEL_COUNT)
        {
            if (StrEquals(extension, "gltf") || StrEquals(extension, "glb"))
            {
                GLTFModel gltfModel = GLTFParse(assetFilepath, &platform);
                Exporter_GLTFModel(&arena, exportAsset->outputFilename, &gltfModel);
            }
            else
            {
                Log("Invalid file extension: '%s'", extension);
                InvalidCodePath;
            }
        }
        // Export texture
        else
        {
            // TODO
        }
    }

    Log("Exporter finished");
}