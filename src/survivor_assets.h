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

    AssetID_Count
};

#define MODEL_COUNT   4
#define TEXTURE_COUNT 3

struct Model
{
    u32*       indices;
    Vertex*    vertexs;
    u32        indicesCount;
    u32        vertexCount;
    AABB       aabb;
    GPUBuffer* gpuBuffer;
};

struct Assets
{
    Arena        arena;
    PlatformAPI* platform;
    Renderer*    renderer;
    Model*       models[MODEL_COUNT];
    Texture*     textures[TEXTURE_COUNT];
};

void AssetLoad(Assets* assets, AssetID id);