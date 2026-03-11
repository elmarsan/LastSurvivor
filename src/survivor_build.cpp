void ObstacleDrag(Entity* obstacle, Camera* camera, mat4x4 projection, v2u windowDim, v2u screenCoordPos)
{
    v3 newPosition = WorldMousePicking(camera, projection, windowDim, screenCoordPos);

    f32 halfWidth = (obstacle->aabb.max.x - obstacle->aabb.min.x) * 0.5f;
    f32 halfDepth = (obstacle->aabb.max.z - obstacle->aabb.min.z) * 0.5f;
    if (EntityIsVerticalOriented(obstacle))
    {
        f32 auxHalfWidth = halfWidth;
        halfWidth        = halfDepth;
        halfDepth        = auxHalfWidth;
    }

    f32 leftLimit   = -(GRID_ROWS * 0.5);
    f32 rightLimit  = -leftLimit;
    f32 topLimit    = -(GRID_COLS * 0.5);
    f32 bottomLimit = -topLimit;

    // Left limit
    if ((newPosition.x - halfWidth) <= leftLimit)
    {
        newPosition.x = leftLimit + halfWidth;
    }
    // Right limits
    if ((newPosition.x + halfWidth) > rightLimit)
    {
        newPosition.x = rightLimit - halfWidth;
    }
    // Top limit
    if ((newPosition.z - halfDepth) < topLimit)
    {
        newPosition.z = topLimit + halfDepth;
    }
    // Bottom limit
    if ((newPosition.z + halfDepth) > bottomLimit)
    {
        newPosition.z = bottomLimit - halfDepth;
    }

    obstacle->position = newPosition;
}

void ObstacleRotate(Entity* obstacle, b32 counterclockwise)
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

SnapCandidate ObstacleFindNearestSnap(GridCell* grid, Entity* obstacle)
{
    EntityWorldCorners worldCorners = EntityGetWorldCorners(obstacle);

    SnapCandidate result = { 0 };

    f32 minSnapDistance = FLT_MAX;

    for (u32 aCornerIndex = 0; aCornerIndex < ArrayCount(worldCorners.corners); aCornerIndex++)
    {
        v3         aCornerPos = worldCorners.corners[aCornerIndex];
        cell_index cell       = WorldPositionToGridCell(aCornerPos);

        if (!WorldIsValidCellIndex(cell))
        {
            continue;
        }

        s32 row = CELL_ROW(cell);
        s32 col = CELL_COL(cell);

        s32 startRow = Max(row - 1, 0);
        s32 endRow   = Min(row + 1, GRID_ROWS - 1);
        s32 startCol = Max(col - 1, 0);
        s32 endCol   = Min(col + 1, GRID_COLS - 1);

        for (s32 r = startRow; r <= endRow; r++)
        {
            for (s32 c = startCol; c <= endCol; c++)
            {
                GridCell* cell = &grid[CELL_INDEX(r, c)];

                for (u32 entityIndex = 0; entityIndex < 4; entityIndex++)
                {
                    Entity* entity = cell->entities[entityIndex];

                    if (!entity || entity == obstacle)
                    {
                        continue;
                    }

                    EntityWorldCorners bWorldCorners = EntityGetWorldCorners(entity);

                    for (u32 bCornerIndex = 0; bCornerIndex < ArrayCount(bWorldCorners.corners); bCornerIndex++)
                    {
                        v3 bCornerPos = bWorldCorners.corners[bCornerIndex];

                        f32 dist = Length(aCornerPos - bCornerPos);
                        if (dist < minSnapDistance)
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

void ObstaclesSnap(GridCell* grid, Entity* a, SnapCandidate* snapCandidate)
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

    // Vertical to vertical
    if (aIsVertical && bIsVertical)
    {
        // Snapping from top
        if (a->position.z < b->position.z)
        {
            a->position.z = b->position.z - aLength;
        }
        // Snapping from bottom
        else
        {
            a->position.z = b->position.z + aLength;
        }

        a->position.x = b->position.x;
    }
    // Horizontal to horizontal
    else if (!aIsVertical && !bIsVertical)
    {
        // Snapping from right
        if (a->position.x > b->position.x)
        {
            a->position.x = b->position.x + aLength;
        }
        // Snapping from left
        else
        {
            a->position.x = b->position.x - aLength;
        }

        a->position.z = b->position.z;
    }
    // (A)Horizontal to B(vertical)
    else if (bIsVertical)
    {
        // Snapping from top
        if (a->position.z < b->position.z)
        {
            a->position.z = b->position.z - bHalfLength - aHalfDepth;
        }
        // Snapping from bottom
        else
        {
            a->position.z = b->position.z + bHalfLength + aHalfDepth;
        }

        b32        snappingToTwoObstacles = false;
        cell_index fromCell               = WorldPositionToGridCell(snapCandidate->to);
        GridCell*  cellInfo               = &grid[fromCell];
        if (cellInfo->entityCount > 1)
        {
            snappingToTwoObstacles = true;
        }

        // Snapping from right
        if (a->position.x > b->position.x)
        {
            a->position.x = b->position.x - bHalfDepth + aHalfLength;
            if (snappingToTwoObstacles)
            {
                a->position.x += bDepth;
            }
        }
        // Snapping from left
        else
        {
            a->position.x = b->position.x + bHalfDepth - aHalfLength;
            if (snappingToTwoObstacles)
            {
                a->position.x -= bDepth;
            }
        }
    }
    // (A)Vertical to (B)horizontal
    else
    {
        // Snapping from top
        if (a->position.z < b->position.z)
        {
            a->position.z = b->position.z - bHalfDepth - aHalfLength;
        }
        // Snapping from bottom
        else
        {
            a->position.z = b->position.z + bHalfDepth + aHalfLength;
        }

        // Snapping from right
        if (a->position.x > b->position.x)
        {
            a->position.x = b->position.x + bHalfLength - aHalfDepth;
        }
        // Snapping from left
        else
        {
            a->position.x = b->position.x - bHalfLength + aHalfDepth;
        }
    }
}

void ObstaclePlace(GridCell* grid, Entity* entity)
{
    Assert(entity->type == EntityType_Obstacle);

    entity->flags &= ~EntityFlag_Positioning;
    EntityCellCorners entityCells = EntityGetCellCorners(entity);

    for (u32 cellIndex = 0; cellIndex < ArrayCount(entityCells.cells); cellIndex++)
    {
        GridAppendEntity(grid, entityCells.cells[cellIndex], entity);
    }
}