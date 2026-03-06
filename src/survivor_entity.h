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

    v3 corners[4];
};

EntityWorldCorners EntityGetWorldCorners(Entity* entity)
{
    v3 localCorners[4] = {
        { entity->aabb.min.x, 0.0f, entity->aabb.min.z }, // Top-left
        { entity->aabb.max.x, 0.0f, entity->aabb.min.z }, // Top-right
        { entity->aabb.min.x, 0.0f, entity->aabb.max.z }, // Bottom-left
        { entity->aabb.max.x, 0.0f, entity->aabb.max.z }, // Bottom-right
    };

    EntityWorldCorners worldCorners = { 0 };

    for (u32 cornerIndex = 0; cornerIndex < 4; cornerIndex++)
    {
        v3 rotatedCorner = RotateVec3Y(localCorners[cornerIndex], entity->yaw);

        worldCorners.corners[cornerIndex] = {
            entity->position.x + rotatedCorner.x, //
            entity->aabb.min.y,                   //
            entity->position.z + rotatedCorner.z  //
        };
    }

    return worldCorners;
}

union EntityCellCorners
{
    struct
    {
        u32 topLeftCell;
        u32 topRightCell;
        u32 bottomLeftCell;
        u32 bottomRightCell;
    };

    u32 cells[4];
};
// TODO: Get rid of this
EntityCellCorners EntityGetCellCorners(Entity* entity)
{
    EntityWorldCorners worldCorners = EntityGetWorldCorners(entity);

    v3 min = worldCorners.corners[0];
    v3 max = worldCorners.corners[0];
    for (u32 cornerIndex = 1; cornerIndex < 4; cornerIndex++)
    {
        min.x = Min(min.x, worldCorners.corners[cornerIndex].x);
        min.z = Min(min.z, worldCorners.corners[cornerIndex].z);
        max.x = Max(max.x, worldCorners.corners[cornerIndex].x);
        max.z = Max(max.z, worldCorners.corners[cornerIndex].z);
    }

    // TODO: Check valid cell
    cell_index minXCell = WorldPositionToGridCell({ min.x, 0.0f, 0.0f });
    cell_index maxXCell = WorldPositionToGridCell({ max.x, 0.0f, 0.0f });
    cell_index minZCell = WorldPositionToGridCell({ 0.0f, 0.0f, min.z });
    cell_index maxZCell = WorldPositionToGridCell({ 0.0f, 0.0f, max.z });

    u32 minCol = CELL_COL(minXCell);
    u32 maxCol = CELL_COL(maxXCell);
    u32 minRow = CELL_ROW(maxZCell);
    u32 maxRow = CELL_ROW(minZCell);

    // TODO: hack!!!
    if (maxRow > 29)
    {
        maxRow = 29;
    }
    if (minRow < 0)
    {
        minRow = 0;
    }

    EntityCellCorners corners = { 0 };
    corners.topLeftCell       = CELL_INDEX(maxRow, minCol);
    corners.topRightCell      = CELL_INDEX(maxRow, maxCol);
    corners.bottomLeftCell    = CELL_INDEX(minRow, minCol);
    corners.bottomRightCell   = CELL_INDEX(minRow, maxCol);

    return corners;
}

b32 EntitiesIntersect(Entity* a, Entity* b, AABB* penetration);