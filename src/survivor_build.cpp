void BuildDragObstacle(Entity* obstacle, Camera* camera, glm::mat4 projection, glm::uvec2 windowDim,
                       glm::uvec2 screenCoordPos)
{
    glm::vec3 newPosition = WorldMousePicking(camera, projection, windowDim, screenCoordPos);

    f32 halfWidth = (obstacle->aabb.max.x - obstacle->aabb.min.x) * 0.5f;
    f32 halfDepth = (obstacle->aabb.max.z - obstacle->aabb.min.z) * 0.5f;
    if (EntityIsVerticalOriented(obstacle))
    {
        f32 auxHalfWidth = halfWidth;
        halfWidth        = halfDepth;
        halfDepth        = auxHalfWidth;
    }

    // Left limit
    if ((newPosition.x - halfWidth) <= GRID_LEFT_LIMIT)
    {
        newPosition.x = GRID_LEFT_LIMIT + halfWidth;
    }
    // Right limits
    if ((newPosition.x + halfWidth) > GRID_RIGHT_LIMIT)
    {
        newPosition.x = GRID_RIGHT_LIMIT - halfWidth;
    }
    // Top limit
    if ((newPosition.z - halfDepth) < GRID_TOP_LIMIT)
    {
        newPosition.z = GRID_TOP_LIMIT + halfDepth;
    }
    // Bottom limit
    if ((newPosition.z + halfDepth) > GRID_BOTTOM_LIMIT)
    {
        newPosition.z = GRID_BOTTOM_LIMIT - halfDepth;
    }

    obstacle->position = newPosition;
}

void BuildRotateObstacle(Entity* obstacle, b32 counterclockwise)
{
    Assert(obstacle->type == EntityType_Obstacle);

    f32 yawStep = Radians(90.0f);

    // Rotate left
    if (counterclockwise)
    {
        obstacle->yaw -= -yawStep;
    }
    // Rotate right
    else
    {
        obstacle->yaw += yawStep;
    }

    obstacle->yaw = fmodf(obstacle->yaw, 2.0f * Pi);
    if (obstacle->yaw < 0.0f)
    {
        obstacle->yaw += (2.0f * Pi);
    }
}

SnapCandidate BuildFindSnapCandidate(World* world, Entity* obstacle)
{
    Assert(obstacle->type == EntityType_Obstacle);

    SnapCandidate result = { 0 };

    EntityWorldCorners worldCorners    = EntityGetWorldCorners(obstacle);
    f32                minSnapDistance = FLT_MAX;

    for (u32 aCornerIndex = 0; aCornerIndex < ArrayCount(worldCorners.arr); aCornerIndex++)
    {
        glm::vec3  aCornerPos = worldCorners.arr[aCornerIndex];
        cell_index cellIndex  = WorldPositionToGridCell(aCornerPos);
        if (!WorldIsValidCellIndex(cellIndex))
        {
            continue;
        }

        s32 row = CELL_ROW(cellIndex);
        s32 col = CELL_COL(cellIndex);

        s32 startRow = Max(row - 1, GRID_MIN_ROW);
        s32 endRow   = Min(row + 1, GRID_MAX_ROW);
        s32 startCol = Max(col - 1, GRID_MIN_COL);
        s32 endCol   = Min(col + 1, GRID_MAX_COL);

        for (s32 r = startRow; r <= endRow; r++)
        {
            for (s32 c = startCol; c <= endCol; c++)
            {
                GridCell* cell = &world->grid[CELL_INDEX(r, c)];

                for (u32 entityIndex = 0; entityIndex < 4; entityIndex++)
                {
                    Entity* entity = cell->entities[entityIndex];
                    if (!entity || entity == obstacle)
                    {
                        continue;
                    }

                    EntityWorldCorners bWorldCorners = EntityGetWorldCorners(entity);

                    for (u32 bCornerIndex = 0; bCornerIndex < ArrayCount(bWorldCorners.arr); bCornerIndex++)
                    {
                        glm::vec3 bCornerPos = bWorldCorners.arr[bCornerIndex];

                        f32 dist = glm::length(aCornerPos - bCornerPos);
                        if (dist <= CELL_SIZE && dist < minSnapDistance)
                        {
                            minSnapDistance = dist;
                            result.entity   = entity;
                            result.from     = aCornerPos;
                            result.to       = bCornerPos;
                        }
                    }
                }
            }
        }
    }

    return result;
}

void BuildSnapObstacles(World* world, Entity* a, SnapCandidate* snapCandidate)
{
    Entity* b = snapCandidate->entity;

    b32 aIsVertical = EntityIsVerticalOriented(a);
    b32 bIsVertical = EntityIsVerticalOriented(b);

    f32 aLength = (a->aabb.max.x - a->aabb.min.x);
    f32 aDepth  = (a->aabb.max.z - a->aabb.min.z);
    f32 bDepth  = (b->aabb.max.z - b->aabb.min.z);
    f32 bLength = (b->aabb.max.x - b->aabb.min.x);

    f32 aHalfLength = aLength * 0.5f;
    f32 bHalfLength = bLength * 0.5f;
    f32 aHalfDepth  = aDepth * 0.5f;
    f32 bHalfDepth  = bDepth * 0.5f;

    glm::vec3 oldPosition = a->position;
    glm::vec3 newPosition = a->position;

    // Vertical to vertical
    if (aIsVertical && bIsVertical)
    {
        // Snapping from top
        if (a->position.z < b->position.z)
        {
            newPosition.z = b->position.z - aLength;
        }
        // Snapping from bottom
        else
        {
            newPosition.z = b->position.z + aLength;
        }

        newPosition.x = b->position.x;
    }
    // Horizontal to horizontal
    else if (!aIsVertical && !bIsVertical)
    {
        // Snapping from right
        if (a->position.x > b->position.x)
        {
            newPosition.x = b->position.x + aLength;
        }
        // Snapping from left
        else
        {
            newPosition.x = b->position.x - aLength;
        }

        newPosition.z = b->position.z;
    }
    // (A)Horizontal to B(vertical)
    else if (bIsVertical)
    {
        // Snapping from top
        if (a->position.z < b->position.z)
        {
            newPosition.z = b->position.z - bHalfLength - aHalfDepth;
        }
        // Snapping from bottom
        else
        {
            newPosition.z = b->position.z + bHalfLength + aHalfDepth;
        }

        b32        snappingToTwoObstacles = false;
        cell_index fromCell               = WorldPositionToGridCell(snapCandidate->to);
        GridCell*  cellInfo               = &world->grid[fromCell];
        if (cellInfo->entityCount > 1)
        {
            snappingToTwoObstacles = true;
        }

        // Snapping from right
        if (a->position.x > b->position.x)
        {
            newPosition.x = b->position.x - bHalfDepth + aHalfLength;
            if (snappingToTwoObstacles)
            {
                newPosition.x += bDepth;
            }
        }
        // Snapping from left
        else
        {
            newPosition.x = b->position.x + bHalfDepth - aHalfLength;
            if (snappingToTwoObstacles)
            {
                newPosition.x -= bDepth;
            }
        }
    }
    // (A)Vertical to (B)horizontal
    else
    {
        // Snapping from top
        if (a->position.z < b->position.z)
        {
            newPosition.z = b->position.z - bHalfDepth - aHalfLength;
        }
        // Snapping from bottom
        else
        {
            newPosition.z = b->position.z + bHalfDepth + aHalfLength;
        }

        // Snapping from right
        if (a->position.x > b->position.x)
        {
            newPosition.x = b->position.x + bHalfLength - aHalfDepth;
        }
        // Snapping from left
        else
        {
            newPosition.x = b->position.x - bHalfLength + aHalfDepth;
        }
    }

    // Revert snap if obstacle end ups outside grid bounds
    {
        a->position                = newPosition;
        EntityWorldCorners corners = EntityGetWorldCorners(a);

        for (u32 cornerIndex = 0; cornerIndex < ArrayCount(corners.arr); cornerIndex++)
        {
            glm::vec3 cornerPos = corners.arr[cornerIndex];

            if (!(cornerPos.x >= GRID_LEFT_LIMIT && cornerPos.x <= GRID_RIGHT_LIMIT) ||
                !(cornerPos.z <= GRID_BOTTOM_LIMIT && cornerPos.z >= GRID_TOP_LIMIT))
            {
                a->position = oldPosition;
                break;
            }
        }
    }
}

void BuildPlaceObstacle(World* world, EntityManager* entityManager, Entity* entity)
{
    Assert(entity->type == EntityType_Obstacle);
    entity->flags &= ~EntityFlag_Positioning;
    WorldAddEntity(world, entityManager, entity);
}

b32 BuildIsObstacleValidPosition(World* world, EntityManager* entityManager, Entity* entity)
{
    Assert(entity->type == EntityType_Obstacle);

    if (!WorldIsValidEntityPosition(world, entity))
    {
        return false;
    }

    // Check for overlapping with other obstacles
    EntityCellCorners entityCorners = EntityGetCellCorners(entity);
    for (u32 entityIndex = 0; entityIndex < entityManager->entityCount; entityIndex++)
    {
        Entity* placedEntity = EntityGet(entityManager, entityIndex);
        if (placedEntity != entity && placedEntity->type == EntityType_Obstacle)
        {
            AABB intersection;
            if (EntitiesIntersect(entity, placedEntity, &intersection))
            {
                return false;
            }
        }
    }

    return true;
}