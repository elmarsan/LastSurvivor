#pragma once

#define MAX_ENTITY_COUNT 128

// TODO: Tweak values
global_variable f32 maxSpeed          = 8.3f;
global_variable f32 frictionForce     = 20.0f;
global_variable f32 moveAcceleration  = 40.0f;
global_variable f32 knockbackForce    = 17.0f;
global_variable f32 rotationSpeed     = 0.05f;
global_variable f32 enemyHitRadius    = 0.75f;
global_variable f32 enemyMaxSpeed     = 1.0f;
global_variable f32 enemyAcceleration = 10.0f;
global_variable s32 maxHealth         = 100;

enum EntityType
{
    EntityType_Player,
    EntityType_Enemy,
    EntityType_Obstacle,
    EntityType_Count
};

enum EntityFlag
{
    EntityFlag_InKnockback     = (1 << 0),
    EntityFlag_Positioning     = (1 << 1),
    EntityFlag_Snapping        = (1 << 2),
    EntityFlag_InvalidPosition = (1 << 3)
};

struct ActiveAnimation
{
    Animation* current;
    f32        time;
};

struct Entity
{
    EntityType      type;
    glm::vec3       position;
    glm::vec3       velocity;
    glm::vec3       scale;
    f32             yaw;          // TODO: Replace by v3/quat for rotations?
    Entity*         targetEntity; // TODO: Needed? All enemies will follow player
    AABB            aabb;
    u32             flags;
    s32             health;
    AssetID         assetID;
    Skeleton*       skeleton;
    ActiveAnimation animation;
};

struct EntityManager
{
    Arena   arena;
    Arena   transientArena;
    Assets* assets;
    Entity  entities[MAX_ENTITY_COUNT];
    u32     entityCount;
};

union EntityWorldCorners
{
    struct
    {
        glm::vec3 topLeft;
        glm::vec3 topRight;
        glm::vec3 bottomLeft;
        glm::vec3 bottomRight;
    };

    glm::vec3 arr[4];
};

union EntityCellCorners
{
    struct
    {
        u32 topLeft;
        u32 topRight;
        u32 bottomLeft;
        u32 bottomRight;
    };

    u32 arr[4];
};

EntityWorldCorners EntityGetWorldCorners(Entity* entity);
EntityCellCorners  EntityGetCellCorners(Entity* entity);
b32                EntitiesIntersect(Entity* a, Entity* b, AABB* penetration);

inline b32 EntityIsVerticalOriented(Entity* entity)
{
    return (entity->yaw == (Pi / 2.0f) || entity->yaw == (3 * Pi / 2.0f));
}

inline b32 EntityIsHorizontalOriented(Entity* entity)
{
    return (entity->yaw == Pi) || (entity->yaw == (2 * Pi)) || (entity->yaw == 0.0f);
}

inline void EntityManagerInit(EntityManager* manager, Arena* arena, Assets* assets)
{
    SubArena(&manager->arena, arena, Megabytes(1));
    SubArena(&manager->transientArena, arena, Megabytes(1));

    manager->assets = assets;
}

inline void EntityManagerFreeTransient(EntityManager* manager)
{
#if BUILD_TYPE_DEBUG
    u32 enemyCount = 0;
    for (u32 entityIndex = 0; entityIndex < manager->entityCount; entityIndex++)
    {
        if (manager->entities[entityIndex].type == EntityType_Enemy)
        {
            enemyCount++;
        }
    }

    Assert(enemyCount == 0);
#endif

    ArenaClear(&manager->transientArena);
}

inline Entity* EntityGet(EntityManager* manager, u32 index) { return &manager->entities[index]; }
Entity*        EntitySpawn(EntityManager* manager, EntityType type, glm::vec3 position);
void           EntityDestroy(EntityManager* manager, Entity* entity, World* world);

inline void EntitiesRemoveFlag(EntityManager* manager, u32 flag)
{
    for (u32 entityIndex = 0; entityIndex < manager->entityCount; entityIndex++)
    {
        Entity* entity = EntityGet(manager, entityIndex);
        entity->flags &= ~flag;
    }
}
