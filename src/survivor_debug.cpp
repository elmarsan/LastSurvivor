internal void DebugDrawPlane(Renderer* renderer, v3 position, v3 size, v4 color);
internal void DebugDrawCircle(Renderer* renderer, v3 position, f32 radius, v4 color, u32 pointCount = 32);
internal void DebugDrawAABB(Renderer* renderer, v3 worldPosition, f32 yRotation, AABB aabb, v4 color);

internal void DebugDrawPlane(Renderer* renderer, v3 position, v3 size, v4 color)
{
    v3 halfSize = size * 0.5f;
    v3 topRight{ position.x - halfSize.x, position.y, position.z - halfSize.z };
    v3 topLeft{ position.x + halfSize.x, position.y, position.z - halfSize.z };
    v3 bottomRight{ position.x - halfSize.x, position.y, position.z + halfSize.z };
    v3 bottomLeft{ position.x + halfSize.x, position.y, position.z + halfSize.z };

    DrawLine(renderer, topRight, topLeft, color);
    DrawLine(renderer, bottomRight, bottomLeft, color);
    DrawLine(renderer, topRight, bottomRight, color);
    DrawLine(renderer, topLeft, bottomLeft, color);
}

internal void DebugDrawCircle(Renderer* renderer, v3 position, f32 radius, v4 color, u32 pointCount)
{
    f32 angleStep = 360.0f / pointCount;

    for (u32 i = 0; i < pointCount; i++)
    {
        f32 angle0 = angleStep * i;
        f32 angle1 = angleStep * ((i + 1) % pointCount);

        f32 x0 = radius * cos(Radians(angle0));
        f32 z0 = radius * sin(Radians(angle0));
        f32 x1 = radius * cos(Radians(angle1));
        f32 z1 = radius * sin(Radians(angle1));

        v3 p0 = position + v3{ x0, 0.0f, z0 };
        v3 p1 = position + v3{ x1, 0.0f, z1 };

        DrawLine(renderer, p0, p1, color);
    }
}

internal void DebugDrawGridCell(Renderer* renderer, cell_index cell, v4 color)
{
    s32 minCol = -(GRID_COLS / 2);
    s32 minRow = -(GRID_ROWS / 2);
    s32 row    = CELL_ROW(cell);
    s32 col    = CELL_COL(cell);

    v3 cellHalfExtent = { CELL_HALF, 0.0f, CELL_HALF };
    v3 worldPosition{ (col + minRow) + cellHalfExtent.x, 0.02f, (f32) - (row + minCol) - cellHalfExtent.z };

    DebugDrawPlane(renderer, worldPosition, cellHalfExtent * 2.0f, color);
}

internal void DebugDrawAABB(Renderer* renderer, v3 worldPosition, f32 yRotation, AABB aabb, v4 color)
{
    // clang-format off
    v3 vertices[8] = {
        { aabb.min.x, aabb.min.y, aabb.min.z }, // 0
        { aabb.min.x, aabb.min.y, aabb.max.z }, // 1
        { aabb.min.x, aabb.max.y, aabb.min.z }, // 2
        { aabb.min.x, aabb.max.y, aabb.max.z }, // 3
        { aabb.max.x, aabb.min.y, aabb.min.z }, // 4
        { aabb.max.x, aabb.min.y, aabb.max.z }, // 5
        { aabb.max.x, aabb.max.y, aabb.min.z }, // 6
        { aabb.max.x, aabb.max.y, aabb.max.z }, // 7
    };

    u32 indices[24] = {
        0, 1, 1, 3, 3, 2, 2, 0,
        4, 5, 5, 7, 7, 6, 6, 4,
        0, 4, 1, 5, 2, 6, 3, 7
    };
    // clang-format on

    mat4x4 translate = Translate(Identity(), worldPosition);
    mat4x4 rotate    = Rotate(Identity(), yRotation, { 0.0f, 1.0f, 0.0f });
    mat4x4 model     = translate * rotate;

    v3 worldVertices[8];
    for (u32 vertexIndex = 0; vertexIndex < 8; vertexIndex++)
    {
        v4 p  = { vertices[vertexIndex].x, vertices[vertexIndex].y, vertices[vertexIndex].z, 1.0f };
        v4 wp = model * p;

        worldVertices[vertexIndex] = { wp.x, wp.y, wp.z };
    }

    // Draw lines
    for (u32 index = 0; index < 24; index += 2)
    {
        v3 p0 = worldVertices[indices[index]];
        v3 p1 = worldVertices[indices[index + 1]];

        DrawLine(renderer, p0, p1, color);
    }
}

void DebugDraw(Debug* debug, GameInput* input, PlatformAPI* platform)
{
    GameState*     state         = debug->state;
    Renderer*      renderer      = state->renderer;
    World*         world         = state->world;
    v2u            windowDim     = platform->WindowGetDimension();
    Camera*        camera        = state->camera;
    mat4x4         projection    = Perspective(Radians(45.0f), (f32)windowDim.w / (f32)windowDim.h, 0.1f, 100.0f);
    mat4x4         view          = CameraView(camera);
    EntityManager* entityManager = state->entityManager;
    Entity*        player        = &entityManager->entities[0];
    Mouse*         mouse         = &input->mouse;

    if (ButtonIsPressed(mouse->middle))
    {
        v3 mousePoint            = WorldMousePicking(camera, projection, windowDim, mouse->pos);
        debug->selectedCellIndex = WorldPositionToGridCell(mousePoint);
    }

    // Debug grid
    {
        f32 y = 0.01f;

        s32 minCol = -(GRID_COLS / 2);
        s32 maxCol = minCol + GRID_COLS;
        s32 minRow = -(GRID_ROWS / 2);
        s32 maxRow = minRow + GRID_ROWS;

        // Vertical lines
        for (s32 col = minCol; col <= maxCol; col++)
        {
            DrawLine(renderer, { (f32)col, y, (f32)minRow }, { (f32)col, y, (f32)maxRow }, black);
        }
        // Horizontal lines
        for (s32 row = minRow; row <= maxRow; row++)
        {
            DrawLine(renderer, { (f32)minCol, y, (f32)row }, { (f32)maxCol, y, (f32)row }, black);
        }

        // Debug selected cell
        if (debug->selectedCellIndex != CELL_EMPTY)
        {
            DebugDrawGridCell(renderer, debug->selectedCellIndex, magenta);
        }

        // Debug graph
        {
            // Nodes and edges
            for (u32 nodeIndex = 0; nodeIndex < world->nodes.size(); nodeIndex++)
            {
                cell_index nodeCell = world->nodes[nodeIndex];

                if (nodeCell != CELL_EMPTY)
                {
                    v3 nodePos = WorldGridCellToPosition(nodeCell);
                    DebugDrawGridCell(renderer, nodeCell, green);

// Select cell edges
#if 1
                    if (debug->selectedCellIndex != CELL_EMPTY)
                    {
                        if (debug->selectedCellIndex == nodeCell)
                        {
                            for (auto edgeIndex : world->edges[nodeIndex])
                            {
                                cell_index dstCell    = world->nodes[edgeIndex];
                                v3         dstNodePos = WorldGridCellToPosition(dstCell);
                                nodePos.y             = 0.1f;
                                dstNodePos.y          = 0.1f;
                                DrawLine(renderer, nodePos, dstNodePos, magenta);
                            }
                        }
                    }
#endif
// All edges
#if 0
                        for (auto edgeIndex : world->edges[nodeIndex])
                        {
                            cell_index dstCell    = world->nodes[edgeIndex];
                            v3         dstNodePos = WorldGridCellToPosition(dstCell);
                            nodePos.y             = 0.1f;
                            dstNodePos.y          = 0.1f;                            
							DrawLine(renderer, nodePos, dstNodePos, magenta);
                        }
#endif
                }
            }

            // Occupied cells
            for (cell_index cellIndex = 0; cellIndex < GRID_CELLS; cellIndex++)
            {
                if (world->grid[cellIndex].entityCount > 0)
                {
                    DebugDrawGridCell(renderer, cellIndex, blue);
                }
            }

            cell_index playerCellIndex = WorldPositionToGridCell(player->position);
            cell_index nodeIndex       = 468;

            v3 playerCellCenter = WorldGridCellToPosition(playerCellIndex);
            v3 nodeCenter       = WorldGridCellToPosition(nodeIndex);

            DrawLine(renderer, playerCellCenter, nodeCenter, red);
        }
    }

    // Shooting
    //  {
    //      if (ButtonIsDown(mouse->left))
    //      {
    //          v3 end = WorldMousePicking(camera, projection, windowDim, mouse->pos);

    //          // DebugDrawLine(state->debug, opengl, player->position, end, green);
    //          DrawLine(renderer, player->position, end, green);
    //      }
    //  }

    // Debug entities
    for (u32 entityIndex = 0; entityIndex < entityManager->entityCount; entityIndex++)
    {
        Entity* entity = EntityGet(entityManager, entityIndex);

        // Entity rotation
        {
            v3 lookAt{ sinf(entity->yaw), 0.0f, cosf(entity->yaw) };
            v3 p0{ entity->position.x, entity->aabb.max.y, entity->position.z };
            v3 p1 = p0 + (lookAt * 1.5f);

            DrawLine(renderer, p0, p1, blue);
        }

        // Entity AABB
        DebugDrawAABB(renderer, entity->position, entity->yaw, entity->aabb, red);

        // Entity Y orientation
        if (entity->type == EntityType_Obstacle)
        {
            // Snap points
#if 1
            if (state->mode == GameMode_Build)
            {
                EntityWorldCorners worldCorners = EntityGetWorldCorners(entity);
                for (u32 cornerIndex = 0; cornerIndex < ArrayCount(worldCorners.arr); cornerIndex++)
                {
                    v3 snapPoint = worldCorners.arr[cornerIndex];
                    snapPoint.y  = 0.0f;

                    DebugDrawPlane(renderer, snapPoint, { CELL_SIZE, 0.0f, CELL_SIZE }, yellow);
                }
            }
#endif
        }
        else if (entity->type == EntityType_Enemy)
        {
            // Enemy hitbox
            DebugDrawCircle(renderer, entity->position, enemyHitRadius, red);

#if 1
            // Path finding
            {
                WorldUpdate(world, entityManager);

                cell_index              startCell = WorldPositionToGridCell(entity->position);
                cell_index              dstCell   = WorldPositionToGridCell(entity->targetEntity->position);
                std::vector<cell_index> path      = WorldFindBestPath(world, state->entityManager, startCell, dstCell);
                if (!path.empty())
                {
                    for (u32 i = 0; i < path.size() - 1; i++)
                    {
                        v3 start = WorldGridCellToPosition(path[i]);
                        v3 end   = WorldGridCellToPosition(path[i + 1]);

                        start.y = 0.01f;
                        end.y   = 0.01f;

                        DrawLine(renderer, start, end, yellow);
                    }
                }
            }
#endif
        }
    }
}