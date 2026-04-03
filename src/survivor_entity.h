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

struct Entity
{
    EntityType type;
    glm::vec3  position;
    glm::vec3  velocity;
    glm::vec3  size;
    f32        yaw;          // TODO: Replace by v3/quat for rotations?
    Entity*    targetEntity; // TODO: Needed? All enemies will follow player
    AABB       aabb;
    u32        flags;
    s32        health;
    AssetID    assetID;
};

struct EntityManager
{
    Entity entities[MAX_ENTITY_COUNT];
    u32    entityCount;
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

void EntityUpdate(Entity* entity);

inline Entity* EntityGet(EntityManager* manager, u32 index) { return &manager->entities[index]; }

inline Entity* EntityNew(EntityManager* manager, EntityType type)
{
    Assert(manager->entityCount < ArrayCount(manager->entities));

    Entity* entity = &manager->entities[manager->entityCount++];
    entity->type   = type;
    entity->health = maxHealth;

    switch (entity->type)
    {
    case EntityType_Player:
    {
        entity->assetID = AssetID_Stickman;
        break;
    }
    case EntityType_Enemy:
    {
        u32 min         = AssetID_ZombieFemaleA;
        u32 max         = AssetID_ZombieMaleA;
        entity->assetID = (AssetID)(rand() % (max + 1 - min) + min);
        break;
    }
    case EntityType_Obstacle:
    {
        entity->assetID = AssetID_Fence;
        break;
    }
    }

    return entity;
}

inline void WorldRemoveEntity(World* world, Entity* entity)
{
    if (entity->type == EntityType_Obstacle)
    {
        EntityCellCorners cells = EntityGetCellCorners(entity);

        // Corners
        {
            for (u32 cellIndex = 0; cellIndex < ArrayCount(cells.arr); cellIndex++)
            {
                cell_index cornerCellIndex = cells.arr[cellIndex];
                GridCell*  cell            = &world->grid[cornerCellIndex];

                for (u32 entityPtrIndex = 0; entityPtrIndex < ArrayCount(cell->entities); entityPtrIndex++)
                {
                    if (cell->entities[entityPtrIndex] == entity)
                    {
                        cell->entities[entityPtrIndex] = 0;
                        cell->entityCount--;
                        break;
                    }
                }
            }
        }

        // Occupied cells
        {
            u32 beginRow = CELL_ROW(cells.bottomRight);
            u32 endRow   = CELL_ROW(cells.topRight);
            u32 beginCol = CELL_COL(cells.bottomLeft);
            u32 endCol   = CELL_COL(cells.bottomRight);

            for (u32 row = beginRow; row <= endRow; row++)
            {
                for (u32 col = beginCol; col <= endCol; col++)
                {
                    world->grid[CELL_INDEX(row, col)].entityCount--;
                }
            }
        }
    }
}

inline void EntityRemove(EntityManager* manager, Entity* entity)
{
    u32 index = 0;
    for (u32 entityIndex = 0; entityIndex < manager->entityCount; entityIndex++)
    {
        if (entity == &manager->entities[entityIndex])
        {
            index = entityIndex;
            break;
        }
    }

    while (index < manager->entityCount)
    {
        manager->entities[index] = manager->entities[index + 1];
        index++;
    }

    manager->entityCount--;
}

inline void EntityDestroy(EntityManager* manager, Entity* entity, World* world)
{
    WorldRemoveEntity(world, entity);
    EntityRemove(manager, entity);
}

inline void EntitiesRemoveFlag(EntityManager* manager, u32 flag)
{
    for (u32 entityIndex = 0; entityIndex < manager->entityCount; entityIndex++)
    {
        Entity* entity = EntityGet(manager, entityIndex);
        entity->flags &= ~flag;
    }
}
