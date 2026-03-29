EntityWorldCorners EntityGetWorldCorners(Entity* entity)
{
    glm::vec3 localCorners[4] = {
        { entity->aabb.min.x, 0.0f, entity->aabb.min.z }, // Top-left
        { entity->aabb.max.x, 0.0f, entity->aabb.min.z }, // Top-right
        { entity->aabb.min.x, 0.0f, entity->aabb.max.z }, // Bottom-left
        { entity->aabb.max.x, 0.0f, entity->aabb.max.z }, // Bottom-right
    };

    // EntityWorldCorners worldCorners = { 0 };
    EntityWorldCorners worldCorners{};

    for (u32 cornerIndex = 0; cornerIndex < ArrayCount(localCorners); cornerIndex++)
    {
        glm::vec3 rotatedCorner = glm::rotateY(localCorners[cornerIndex], entity->yaw);

        worldCorners.arr[cornerIndex] = {
            entity->position.x + rotatedCorner.x, //
            entity->aabb.min.y,                   //
            entity->position.z + rotatedCorner.z  //
        };
    }

    return worldCorners;
}

EntityCellCorners EntityGetCellCorners(Entity* entity)
{
    EntityWorldCorners worldCorners = EntityGetWorldCorners(entity);

    glm::vec3 min = worldCorners.arr[0];
    glm::vec3 max = worldCorners.arr[0];
    for (u32 cornerIndex = 1; cornerIndex < 4; cornerIndex++)
    {
        min.x = Min(min.x, worldCorners.arr[cornerIndex].x);
        min.z = Min(min.z, worldCorners.arr[cornerIndex].z);
        max.x = Max(max.x, worldCorners.arr[cornerIndex].x);
        max.z = Max(max.z, worldCorners.arr[cornerIndex].z);
    }

    if (max.x >= GRID_RIGHT_LIMIT)
    {
        max.x = GRID_RIGHT_LIMIT - CELL_SIZE;
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

    if (maxRow > GRID_MAX_ROW)
    {
        maxRow = GRID_MAX_ROW;
    }
    if (minRow < GRID_MIN_ROW)
    {
        minRow = GRID_MIN_ROW;
    }

    EntityCellCorners corners = { 0 };
    corners.topLeft           = CELL_INDEX(maxRow, minCol);
    corners.topRight          = CELL_INDEX(maxRow, maxCol);
    corners.bottomLeft        = CELL_INDEX(minRow, minCol);
    corners.bottomRight       = CELL_INDEX(minRow, maxCol);

    return corners;
}

internal AABB EntityWorldAABB(Entity* entity)
{
    AABB result;

    if (entity->type == EntityType_Obstacle)
    {
        if (EntityIsVerticalOriented(entity))
        {
            f32 halfWidth = (entity->aabb.max.x - entity->aabb.min.x) * 0.5f;
            f32 halfDepth = (entity->aabb.max.z - entity->aabb.min.z) * 0.5f;

            result.min.x = entity->position.x - halfDepth;
            result.max.x = entity->position.x + halfDepth;
            result.min.z = entity->position.z - halfWidth;
            result.max.z = entity->position.z + halfWidth;
        }
        else if (EntityIsHorizontalOriented(entity))
        {
            result.min.x = entity->position.x + entity->aabb.min.x;
            result.max.x = entity->position.x + entity->aabb.max.x;
            result.min.z = entity->position.z + entity->aabb.min.z;
            result.max.z = entity->position.z + entity->aabb.max.z;
        }
        else
        {
            Assert(0);
        }

        result.min.y = entity->aabb.min.y;
        result.max.y = entity->aabb.max.y;
    }
    else
    {
        EntityWorldCorners corners = EntityGetWorldCorners(entity);

        result.min = corners.arr[0];
        result.max = corners.arr[0];

        for (u32 cornerIndex = 1; cornerIndex < ArrayCount(corners.arr); cornerIndex++)
        {
            glm::vec3 p = corners.arr[cornerIndex];

            result.min.x = Min(result.min.x, p.x);
            result.min.z = Min(result.min.z, p.z);
            result.max.x = Max(result.max.x, p.x);
            result.max.z = Max(result.max.z, p.z);
        }

        result.min.y = entity->aabb.min.y;
        result.max.y = entity->aabb.max.y;
    }

    return result;
}

b32 EntitiesIntersect(Entity* a, Entity* b, AABB* intersection = 0)
{
    AABB aWorldAABB = EntityWorldAABB(a);
    AABB bWorldAABB = EntityWorldAABB(b);

    if (a->type == EntityType_Obstacle && b->type == EntityType_Obstacle)
    {
        return AABBIntersectionXZ(aWorldAABB, bWorldAABB);
    }

    return AABBIntersection(aWorldAABB, bWorldAABB, intersection);
}

// typedef void (*EntityUpdateFunc)(GameState* state, f32 delta, Entity* entity);
//
// internal void PlayerUpdate(GameState* state, f32 delta, PlatformAPI platform, Entity* entity)
//{
// }
//  internal void EnemyUpdate(GameState* state, Entity* entity)
//{
//      //
//      Assert(0);
//  }
//  internal void ObstacleUpdate(GameState* state, Entity* entity)
//{
//      //
//      Assert(0);
//  }
//
// void EntityUpdate(GameState* state, f32 delta, Entity* entity)
//{
//     local_persist EntityUpdateFunc updateTable[EntityType_Count] = {
//         PlayerUpdate,
//         // EnemyUpdate,
//         // ObstacleUpdate
//     };
//     updateTable[entity->type](state, entity);
// }