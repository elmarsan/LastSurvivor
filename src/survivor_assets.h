#pragma once

enum AssetID
{
    AssetID_Stickman,
    AssetID_Fence,
    AssetID_ZombieFemaleA,
    AssetID_ZombieMaleA,

    AssetID_ZombieTexture,
    AssetID_CrosshairTexture,
    AssetID_FenceTexture,

    AssetID_ZombieAttackLeftAnimation,

    AssetID_Count
};

#define MODEL_COUNT              4
#define TEXTURE_COUNT            3
#define JOINT_MAX_CHILDREN_COUNT 4

struct Joint
{
    u32       childrenIndexes[JOINT_MAX_CHILDREN_COUNT];
    u32       childrenCount;
    char      name[256];
    glm::vec3 bindTranslation;
    glm::quat bindRotation;
    glm::vec3 bindScale;
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
};

struct Skeleton
{
    u32        jointCount;
    Joint*     joints;
    glm::mat4* jointMatrices;
};

enum AnimationChannelPath
{
    AnimationChannelPath_Translation,
    AnimationChannelPath_Rotation,
    AnimationChannelPath_Scale
};

struct AnimationChannel
{
    u32                  samplerIndex;
    u32                  jointIndex;
    AnimationChannelPath path;
};

struct AnimationSampler
{
    u32        count;
    f32*       times;
    glm::vec4* transformations;
};

struct Animation
{
    char              name[256];
    f32               duration;
    u32               channelCount;
    u32               samplerCount;
    AnimationSampler* samplers;
    AnimationChannel* channels;
};

struct Model
{
    u32*       indices;
    Vertex*    vertexs;
    u32        indicesCount;
    u32        vertexCount;
    b32        skinned;
    AABB       aabb;
    GPUBuffer* gpuBuffer;
    Skeleton*  skeleton;
};

struct Assets
{
    Arena        arena;
    PlatformAPI* platform;
    Renderer*    renderer;
    Model*       models[MODEL_COUNT];
    Texture*     textures[TEXTURE_COUNT];
    Animation*   animations[1];
};

void     AssetsInit(Assets* assets, Arena* baseArena, PlatformAPI* platform);
void     AssetsLoad(Assets* assets, AssetID id);
Model*   AssetsModelGet(Assets* assets, AssetID id);
Texture* AssetsTextureGet(Assets* assets, AssetID id);
void     AssetExportModel(Assets* assets, AssetID id, Vertex* vertexs, u32* indices, u32 vertexCount, u32 indexCount,
                          Skeleton* skeleton);
void     AssetExportAnimation(Assets* assets, AssetID id, Animation* animation);