#pragma once

enum EntityType
{
    EntityType_Player,
    EntityType_Enemy,
    EntityType_Obstacle
};

enum EntityFlag
{
    EntityFlag_InKnockback     = (1 << 0),
    EntityFlag_Positioning     = (1 << 1),
    EntityFlag_Snapping        = (1 << 2),
    EntityFlag_InvalidPosition = (1 << 3),
};

struct Entity
{
    EntityType type;
    v3         position;
    v3         velocity;
    v3         size;
    f32        yaw;          // TODO: Replace by v3/quat for rotations?
    Entity*    targetEntity; // TODO: Needed? All enemies will follow player
    AABB       aabb;
    u32        flags;
    u32        index;
};

union EntityWorldCorners
{
    struct
    {
        v3 topLeft;
        v3 topRight;
        v3 bottomLeft;
        v3 bottomRight;
    };

    v3 arr[4];
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