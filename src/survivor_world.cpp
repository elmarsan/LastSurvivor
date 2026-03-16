internal void WorldComputeNodeEdges(Entity* entities, u32 entityCount, World* world, u32 targetNodeIndex);

v3 WorldMousePicking(Camera* camera, mat4x4 projection, v2u windowDim, v2u mouse)
{
    f32 screenWidth  = (f32)windowDim.w;
    f32 screenHeight = (f32)windowDim.h;
    f32 mouseX       = (f32)mouse.x;
    f32 mouseY       = (f32)mouse.y;

    mat4x4 inverseProjection = Inverse(projection);
    mat4x4 inverseView       = Inverse(CameraView(camera));

    // Viewport -> NDC
    v3 rayNdc = { 0 };
    rayNdc.x  = (2.0f * mouseX) / screenWidth - 1.0f;
    rayNdc.y  = 1.0f - (2.0f * mouseY) / screenHeight;

    // NDC -> Clip
    v4 rayClip{ rayNdc.x, rayNdc.y, -1.0f, 1.0f };

    // Clip -> View
    v4 rayView = inverseProjection * rayClip;
    rayView.z  = -1.0f;
    rayView.w  = 0;

    // View -> World
    v4 rayWorld4 = inverseView * rayView;
    v3 rayWorld{ rayWorld4.x, rayWorld4.y, rayWorld4.z };
    rayWorld = Norm(rayWorld);

    // Intersection with world plane
    v3  camPos = CameraPosition(camera);
    f32 t      = -(camPos.y / rayWorld.y);
    v3  point  = camPos + rayWorld * t;

    return point;
}

// b32 WorldIsPositionInBounds(v3 position)
//{
//     // Left limit
//     if (position.x < -(GRID_ROWS * 0.5))
//     {
//         return false;
//     }
//     // Right limit
//     if (position.x > (GRID_ROWS * 0.5))
//     {
//         return false;
//     }
//     // Top limit
//     if (position.z < -(GRID_COLS * 0.5))
//     {
//         return false;
//     }
//     // Bottom limit
//     if (position.z > (GRID_COLS * 0.5))
//     {
//         return false;
//     }

//    return true;
//}

void GridAppendEntity(GridCell* grid, cell_index cellIndex, Entity* entity)
{
    Assert(WorldIsValidCellIndex(cellIndex));

    b32       duplicated = false;
    GridCell* cell       = &grid[cellIndex];

    for (u32 entityPtrIndex = 0; entityPtrIndex < ArrayCount(cell->entities); entityPtrIndex++)
    {
        if (cell->entities[entityPtrIndex] == entity)
        {
            duplicated = true;
            break;
        }
    }

    if (!duplicated)
    {
        Assert(cell->entityCount < ArrayCount(cell->entities));
        cell->entities[cell->entityCount++] = entity;
    }
}

b32 GridIsValidCellForEntity(GridCell* grid, cell_index cellIndex, Entity* entity)
{
    GridCell* cell = &grid[cellIndex];

    // Cell occupied by 4 entities
    if (cell->entityCount == 4)
    {
        return false;
    }

    u32 verticalOrientedEntityCount   = 0;
    u32 horizontalOrientedEntityCount = 0;

    for (u32 entityPtrIndex = 0; entityPtrIndex < ArrayCount(cell->entities); entityPtrIndex++)
    {
        Entity* entityPtr = cell->entities[entityPtrIndex];
        if (entityPtr)
        {
            // Assert(entityPtr->type == EntityType_Obstacle);
            if (EntityIsVerticalOriented(entityPtr))
            {
                verticalOrientedEntityCount++;
            }
            else
            {
                horizontalOrientedEntityCount++;
            }
        }
    }
    Assert(verticalOrientedEntityCount <= 2);
    Assert(horizontalOrientedEntityCount <= 2);

    b32 newEntityIsVertical = EntityIsVerticalOriented(entity);
    if (newEntityIsVertical && verticalOrientedEntityCount < 2)
    {
        return true;
    }
    else if (!newEntityIsVertical && horizontalOrientedEntityCount < 2)
    {
        return true;
    }

    return false;
}

void WorldComputeNodes(Entity* entities, u32 entityCount, World* world)
{
    world->nodes.clear();

    for (u32 row = 0; row < GRID_ROWS; row++)
    {
        cell_index firstNotEmpty = GRAPH_EMPTY_NODE;
        cell_index lastNotEmpty  = GRAPH_EMPTY_NODE;

        for (u32 col = 0; col < GRID_COLS; col++)
        {
            cell_index cellIndex = CELL_INDEX(row, col);
            GridCellV2 cell      = world->gridV2[cellIndex];

            // Occupied cells
            if (cell.entityCount)
            {
                // DebugDrawGridCell(state->debug, opengl, cellIndex, blue);
            }

            if (cell.entityCount > 0 && firstNotEmpty == GRAPH_EMPTY_NODE)
            {
                firstNotEmpty = cellIndex;

                // Top-left corner
                if (CELL_COL(firstNotEmpty) > GRID_MIN_COL)
                {
                    cell_index topLeft     = CELL_INDEX(CELL_ROW(firstNotEmpty) + 1, CELL_COL(firstNotEmpty) - 1);
                    cell_index top         = firstNotEmpty + GRID_ROWS;
                    GridCellV2 topLeftCell = world->gridV2[topLeft];
                    GridCellV2 topCell     = world->gridV2[top];

                    if (topCell.entityCount == 0)
                    {
                        // DebugDrawGridCell(state->debug, opengl, topLeft, green);
                        // nodes.push_back(topLeft);
                        world->nodes.push_back(topLeft);
                    }
                }

                // Bottom-left corner
                if (CELL_ROW(firstNotEmpty) > GRID_MIN_ROW && CELL_COL(firstNotEmpty) > GRID_MIN_COL)
                {
                    cell_index bottomLeft     = CELL_INDEX(CELL_ROW(firstNotEmpty) - 1, CELL_COL(firstNotEmpty) - 1);
                    cell_index bottom         = firstNotEmpty - GRID_ROWS;
                    GridCellV2 bottomLeftCell = world->gridV2[bottomLeft];
                    GridCellV2 bottomCell     = world->gridV2[bottom];

                    if (bottomCell.entityCount == 0)
                    {
                        // DebugDrawGridCell(state->debug, opengl, bottomLeft, green);
                        // nodes.push_back(bottomLeft);
                        world->nodes.push_back(bottomLeft);
                    }
                }
            }
            else if (cell.entityCount == 0 && firstNotEmpty != GRAPH_EMPTY_NODE && lastNotEmpty == GRAPH_EMPTY_NODE)
            {
                lastNotEmpty = CELL_INDEX(row, col - 1);

                // Top-right corner
                if (CELL_COL(lastNotEmpty) < GRID_MAX_COL)
                {
                    cell_index topRight     = CELL_INDEX(CELL_ROW(lastNotEmpty) + 1, CELL_COL(lastNotEmpty) + 1);
                    cell_index top          = lastNotEmpty + GRID_ROWS;
                    GridCellV2 topRightCell = world->gridV2[topRight];
                    GridCellV2 topCell      = world->gridV2[top];

                    if (topCell.entityCount == 0)
                    {
                        // DebugDrawGridCell(state->debug, opengl, topRight, green);
                        // nodes.push_back(topRight);
                        world->nodes.push_back(topRight);
                    }
                }

                // Bottom-right corner
                if (CELL_ROW(lastNotEmpty) > GRID_MIN_ROW)
                {
                    cell_index bottomRight     = CELL_INDEX(CELL_ROW(lastNotEmpty) - 1, CELL_COL(lastNotEmpty) + 1);
                    cell_index bottom          = lastNotEmpty - GRID_ROWS;
                    GridCellV2 bottomRightCell = world->gridV2[bottomRight];
                    GridCellV2 bottomCell      = world->gridV2[bottom];

                    if (bottomCell.entityCount == 0)
                    {
                        // DebugDrawGridCell(state->debug, opengl, bottomRight, green);
                        // nodes.push_back(bottomRight);
                        world->nodes.push_back(bottomRight);
                    }
                }

                firstNotEmpty = GRAPH_EMPTY_NODE;
                lastNotEmpty  = GRAPH_EMPTY_NODE;
            }
        }
    }

    world->edges.resize(world->nodes.size());
    for (u32 nodeIndex = 0; nodeIndex < world->nodes.size(); nodeIndex++)
    {
        WorldComputeNodeEdges(entities, entityCount, world, nodeIndex);
    }
}

internal void WorldComputeNodeEdges(Entity* entities, u32 entityCount, World* world, u32 targetNodeIndex)
{
    v3 nodeAPos = WorldGridCellToPosition(world->nodes[targetNodeIndex]);

    for (u32 nodeIndex = 0; nodeIndex < world->nodes.size(); nodeIndex++)
    {
        if (world->nodes[targetNodeIndex] != world->nodes[nodeIndex])
        {
            v3 nodeBPos = WorldGridCellToPosition(world->nodes[nodeIndex]);

            b32 intersectEntity = false;
            for (u32 entityIndex = 0; entityIndex < entityCount; entityIndex++)
            {
                // Entity* entity = EntityGet(state, entityIndex);
                Entity* entity = &entities[entityIndex];
                if (entity->type == EntityType_Obstacle)
                {
                    AABB entityWorldAABB = EntityWorldAABB(entity);
                    // entityWorldAABB      = AABBExpandXZ(entityWorldAABB, CELL_SIZE);

                    if (AABBSegmentIntersection(entityWorldAABB, nodeAPos, nodeBPos))
                    {
                        intersectEntity = true;
                        break;
                    }
                }
            }

            if (!intersectEntity)
            {
                world->edges[targetNodeIndex].push_back(nodeIndex);
                nodeAPos.y = 0.1f;
                nodeBPos.y = 0.1f;
            }
        }
    }
}