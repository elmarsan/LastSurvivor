#pragma once

#define MAX_ENTITY_COUNT 128

// TODO: Tweak values
global_variable f32 playerMaxSpeed         = 8.3f;
global_variable f32 playerFrictionForce    = 20.0f;
global_variable f32 playerMoveAcceleration = 40.0f;
global_variable f32 knockbackForce         = 17.0f;
global_variable f32 rotationSpeed          = 0.05f;
global_variable f32 enemyHitRadius         = 0.75f;
global_variable f32 enemyMaxSpeed          = 1.0f;
global_variable f32 enemyAcceleration      = 20.0f;
global_variable s32 maxHealth              = 100;
global_variable f32 enemyStrikingRange     = 0.5f;
global_variable f32 enemyJumpRange         = 0.2f;
global_variable f32 capsuleRadius          = 0.8f;

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
    EntityFlag_InvalidPosition = (1 << 3),
    EntityFlag_InAttack        = (1 << 4),
    EntityFlag_Hitting         = (1 << 5),
    EntityFlag_Idle            = (1 << 6),
    EntityFlag_Climbing        = (1 << 7),
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
    glm::vec3       rotation;
    glm::vec3       direction;
    Entity*         targetEntity; // TODO: Needed? All enemies will follow player
    AABB            aabb;
    u32             flags;
    s32             health;
    u32             assetID;
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
    return (entity->rotation.y == (Pi / 2.0f) || entity->rotation.y == (3 * Pi / 2.0f));
}

inline b32 EntityIsHorizontalOriented(Entity* entity)
{
    return (entity->rotation.y == Pi) || (entity->rotation.y == (2 * Pi)) || (entity->rotation.y == 0.0f);
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
// void           EntityDestroy(EntityManager* manager, Entity* entity, World* world);

inline void EntitiesRemoveFlag(EntityManager* manager, u32 flag)
{
    for (u32 entityIndex = 0; entityIndex < manager->entityCount; entityIndex++)
    {
        Entity* entity = EntityGet(manager, entityIndex);
        entity->flags &= ~flag;
    }
}
