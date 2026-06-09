#pragma once

#define MODEL_COUNT              Texture_Crosshair
#define TEXTURE_COUNT            4
#define ANIMATION_COUNT          AssetCount - TEXTURE_COUNT
#define JOINT_MAX_CHILDREN_COUNT 10
#define MODEL_SCALE              glm::vec3{ 0.015f, 0.015f, 0.015f }

enum Asset
{
    Model_ZombieMaleA,
    Model_ZombieFemaleA,
    Model_Parking,

    Texture_Crosshair,
    Texture_ShotgunSprite,
    Texture_PistolSprite,
    Texture_BloodSprite,

    // TODO: Asset pipeline: group animation into files.
    // Example zombie male with (attackt_left, attackt_right, walk, etc...)
    // Example zombie FEmale with (attackt_left, attackt_right, walk, etc...)
    // Same for special zombies
    Anim_ZombieMaleAttackLeft,
    Anim_ZombieMaleAttackRight,
    Anim_ZombieMaleIdle,
    Anim_ZombieMaleIdleAlert,
    Anim_ZombieMaleIdle2,
    Anim_ZombieMaleRunning,
    Anim_ZombieMaleSlowWalk,
    Anim_ZombieMaleWalk,
    Anim_ZombieMaleWalkAgressive,
    Anim_ZombieMaleWalkLimp,
    Anim_ZombieMaleCrawlingForward,
    Anim_ZombieMaleCrawlingIdle,

    Anim_ZombieFemaleAttackLeft,
    Anim_ZombieFemaleAttackRight,
    Anim_ZombieFemaleIdle,
    Anim_ZombieFemaleIdleAlert,
    Anim_ZombieFemaleIdle2,
    Anim_ZombieFemaleRunning,
    Anim_ZombieFemaleSlowWalk,
    Anim_ZombieFemaleWalk,
    Anim_ZombieFemaleWalkAgressive,
    Anim_ZombieFemaleWalkLimp,
    Anim_ZombieFemaleCrawlingForward,
    Anim_ZombieFemaleCrawlingIdle,

    AssetCount
};

char* assetFilenames[AssetCount] = {
    // Models
    "../data/zombie_Male_A.svv",
    "../data/zombie_Female_A.svv",
    "../data/parking.svv",

    // Textures
    "../data/crosshairs.svv",
    "../data/sprite_shotgun_modified.svv",
    "../data/sprite_pistol_modified.svv",
    "../data/sprite_blood.svv",

    // Zombie male animations
    "../data/zombie_male_attack_left.svv",
    "../data/zombie_male_attack_right.svv",
    "../data/zombie_male_idle.svv",
    "../data/zombie_male_idle_alert.svv",
    "../data/zombie_male_idle_2.svv",
    "../data/zombie_male_running.svv",
    "../data/zombie_male_slow_walk.svv",
    "../data/zombie_male_walk.svv",
    "../data/zombie_male_walk_agressive.svv",
    "../data/zombie_male_walk_limp.svv",
    "../data/zombie_male_crawling_forward.svv",
    "../data/zombie_male_crawling_idle.svv",

    // Zombie female animations
    "../data/zombie_female_attack_left.svv",
    "../data/zombie_female_attack_right.svv",
    "../data/zombie_female_idle.svv",
    "../data/zombie_female_idle_alert.svv",
    "../data/zombie_female_idle_2.svv",
    "../data/zombie_female_running.svv",
    "../data/zombie_female_slow_walk.svv",
    "../data/zombie_female_walk.svv",
    "../data/zombie_female_walk_agressive.svv",
    "../data/zombie_female_walk_limp.svv",
    "../data/zombie_female_crawling_forward.svv",
    "../data/zombie_female_crawling_idle.svv",
};

struct Joint
{
    char      name[256];
    u32       childrenIndexes[JOINT_MAX_CHILDREN_COUNT];
    u32       childrenCount;
    int       parentIndex;
    glm::mat4 inverseBindMatrix;
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
};

struct Skeleton
{
    u32    jointCount;
    Joint* joints;
    // Final skinning matrices used by the GPU
    // Converts vertices from bind pose into animated pose
    glm::mat4* jointSkinMatrices;
    // Bone transforms in model space (hierarchy result)
    glm::mat4* jointGlobalMatrices;
    // Mapping used to match joint order with mesh skinning order
    u32* jointIndexBindOrder;
};

enum AnimationChannelPath : u32
{
    AnimationChannelPath_Translation,
    AnimationChannelPath_Rotation,
    AnimationChannelPath_Scale
};

enum AnimationSamplerInterpolation : u32
{
    AnimationSamplerInterpolation_Linear,
    AnimationSamplerInterpolation_Step
};

struct AnimationChannel
{
    u32                  samplerIndex;
    u32                  jointIndex;
    AnimationChannelPath path;
};

struct AnimationSampler
{
    u32                           count;
    AnimationSamplerInterpolation interpolation;
    f32*                          times;
    glm::vec4*                    transformations;
};

struct Animation
{
    u32               id;
    char              name[256];
    f32               duration;
    u32               channelCount;
    u32               samplerCount;
    AnimationSampler* samplers;
    AnimationChannel* channels;
};

struct Mesh
{
    u32        indicesCount;
    u32        vertexCount;
    int        materialIndex;
    u32*       indices;
    Vertex*    vertexs;
    GPUBuffer* gpuBuffer;
};

struct Material
{
    int baseColorIndex;
};

struct Model
{
    glm::vec3  localTranslation;
    glm::quat  localRotation;
    glm::vec3  localScale;
    Mesh*      meshes;
    u32        meshCount;
    Texture*   textures;
    u32        textureCount;
    Skeleton*  skeleton;
    Material*  materials;
    u32        materialCount;
    Animation* animations;
    u32        animationCount;
};

struct Assets
{
    Arena        arena;
    PlatformAPI* platform;
    Renderer*    renderer;
    Model*       models[MODEL_COUNT];
    Texture*     textures[TEXTURE_COUNT];
    Animation*   animations[ANIMATION_COUNT];
};

enum AssetType
{
    AssetType_Model     = 1,
    AssetType_Font      = 2,
    AssetType_Sfx       = 3,
    AssetType_Texture   = 4,
    AssetType_Animation = 5,
};

struct WorldCollider
{
    glm::vec3 position;
    AABB      aabb;
};

struct Sprite2D
{
    Texture*   texture;
    u32        frameCount;
    glm::vec2* positions;
    glm::vec2* sizes;
    s32        frameIndex;
    b32        playingAnimation;
    glm::vec2  drawSize;
    f32        timer;
    f32        animSpeedSeconds;
};

#pragma pack(push, 1)
struct AssetFileHeader
{
    u8  type;
    u8  reserved[3];
    u64 reserved4;
};

struct AssetModelFileHeader
{
    u32       meshCount;
    b32       skinned;
    u32       textureCount;
    u32       materialCount;
    u32       animationCount;
    glm::vec3 localTranslation;
    glm::quat localRotation;
    glm::vec3 localScale;
};

struct AssetMeshHeader
{
    char name[64];
    u32  vertexCount;
    u32  indicesCount;
    int  materialIndex;
};

struct AssetTextureHeader
{
    char name[64];
    u32  size;
};

struct AssetAnimationFileHeader
{
    char name[256];
    f32  duration;
    u32  channelCount;
    u32  samplerCount;
};
#pragma pack(pop)

void       Assets_Init(Assets* assets, Arena* baseArena, PlatformAPI* platform);
void       Assets_Load(Assets* assets, Asset id);
Model*     Assets_GetModel(Assets* assets, Asset id);
Animation* Assets_GetAnimation(Assets* assets, Asset id);
Texture*   Assets_GetTexture(Assets* assets, Asset id);

void Skeleton_UpdatePose(Skeleton* skeleton);
void Skeleton_ApplyAnimation(Skeleton* skeleton, Animation* animation, f32 time);