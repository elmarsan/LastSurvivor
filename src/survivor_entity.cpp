Entity* EntitySpawn(EntityManager* manager, EntityType type, glm::vec3 position)
{
    Assert(manager->entityCount < ArrayCount(manager->entities));

    // Arena* arena          = &manager->arena;
    Arena* transientArena = &manager->transientArena;

    Entity* entity   = &manager->entities[manager->entityCount++];
    entity->type     = type;
    entity->health   = maxHealth;
    entity->position = position;

    switch (entity->type)
    {
    case EntityType_Player:
    {
        entity->assetID = AssetID_Stickman;
        entity->scale   = glm::vec3{ 1.0f, 1.0f, 1.0f };
        Model* model    = AssetsModelGet(manager->assets, entity->assetID);
        entity->aabb    = model->aabb;
        // TODO: Push player skeleton to permanent arena
        break;
    }
    case EntityType_Enemy:
    {
        u32     min        = AssetID_ZombieFemaleA;
        u32     max        = AssetID_ZombieMaleA;
        AssetID assetID    = (AssetID)(rand() % (max + 1 - min) + min);
        Model*  model      = AssetsModelGet(manager->assets, assetID);
        u32     jointCount = model->skeleton->jointCount;

        entity->assetID = assetID;
        entity->scale   = ZOMBIE_SCALE;
        // TODO: Apply ZOMBIE_SCALE factor to zombie aabb
        entity->aabb                          = AssetsModelGet(manager->assets, AssetID_Stickman)->aabb;
        entity->skeleton                      = PushStruct(transientArena, Skeleton);
        entity->skeleton->joints              = PushArray(transientArena, jointCount, Joint);
        entity->skeleton->jointMatrices       = PushArray(transientArena, jointCount, glm::mat4);
        entity->skeleton->jointIndexBindOrder = PushArray(transientArena, jointCount - 1, u32);
        entity->skeleton->jointCount          = jointCount;

        memcpy(entity->skeleton->joints, model->skeleton->joints, sizeof(Joint) * jointCount);
        memcpy(entity->skeleton->jointIndexBindOrder, model->skeleton->jointIndexBindOrder,
               sizeof(u32) * jointCount - 1);

        break;
    }
    case EntityType_Obstacle:
    {
        entity->assetID = AssetID_Fence;
        entity->scale   = glm::vec3{ 1.0f, 1.0f, 1.0f };
        Model* model    = AssetsModelGet(manager->assets, AssetID_Fence);
        entity->aabb    = model->aabb;
        break;
    }
    }

    return entity;
}

internal void WorldRemoveEntity(World* world, Entity* entity)
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

internal void EntityRemove(EntityManager* manager, Entity* entity)
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

void EntityDestroy(EntityManager* manager, Entity* entity, World* world)
{
    WorldRemoveEntity(world, entity);
    EntityRemove(manager, entity);
}

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