internal void DebugDrawPlane(Renderer* renderer, glm::vec3 position, glm::vec3 size, glm::vec4 color);
internal void DebugDrawCircle(Renderer* renderer, glm::vec3 position, f32 radius, glm::vec4 color, u32 pointCount = 32);
internal void DebugDrawAABB(Renderer* renderer, glm::vec3 worldPosition, f32 yRotation, AABB aabb, glm::vec4 color);

internal void DebugDrawPlane(Renderer* renderer, glm::vec3 position, glm::vec3 size, glm::vec4 color)
{
    glm::vec3 halfSize = size * 0.5f;
    glm::vec3 topRight{ position.x - halfSize.x, position.y, position.z - halfSize.z };
    glm::vec3 topLeft{ position.x + halfSize.x, position.y, position.z - halfSize.z };
    glm::vec3 bottomRight{ position.x - halfSize.x, position.y, position.z + halfSize.z };
    glm::vec3 bottomLeft{ position.x + halfSize.x, position.y, position.z + halfSize.z };

    DrawLine(renderer, topRight, topLeft, color);
    DrawLine(renderer, bottomRight, bottomLeft, color);
    DrawLine(renderer, topRight, bottomRight, color);
    DrawLine(renderer, topLeft, bottomLeft, color);
}

internal void DebugDrawCircle(Renderer* renderer, glm::vec3 position, f32 radius, glm::vec4 color, u32 pointCount)
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

        glm::vec3 p0 = position + glm::vec3{ x0, 0.0f, z0 };
        glm::vec3 p1 = position + glm::vec3{ x1, 0.0f, z1 };

        DrawLine(renderer, p0, p1, color);
    }
}

internal void DebugDrawGridCell(Renderer* renderer, cell_index cell, glm::vec4 color)
{
    s32 minCol = -(GRID_COLS / 2);
    s32 minRow = -(GRID_ROWS / 2);
    s32 row    = CELL_ROW(cell);
    s32 col    = CELL_COL(cell);

    glm::vec3 cellHalfExtent = { CELL_HALF, 0.0f, CELL_HALF };
    glm::vec3 worldPosition{ (col + minRow) + cellHalfExtent.x, 0.02f, (f32) - (row + minCol) - cellHalfExtent.z };

    DebugDrawPlane(renderer, worldPosition, cellHalfExtent * 2.0f, color);
}

internal void DebugDrawAABB(Renderer* renderer, glm::vec3 worldPosition, f32 yRotation, AABB aabb, glm::vec4 color)
{
    // clang-format off
    glm::vec3 vertices[8] = {
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

    glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, worldPosition);
    glm::mat4 rotate    = glm::rotate(glm::mat4{ 1.0f }, yRotation, { 0.0f, 1.0f, 0.0f });
    glm::mat4 model     = translate * rotate;

    glm::vec3 worldVertices[8];
    for (u32 vertexIndex = 0; vertexIndex < 8; vertexIndex++)
    {
        glm::vec4 p  = { vertices[vertexIndex].x, vertices[vertexIndex].y, vertices[vertexIndex].z, 1.0f };
        glm::vec4 wp = model * p;

        worldVertices[vertexIndex] = { wp.x, wp.y, wp.z };
    }

    // Draw lines
    for (u32 index = 0; index < 24; index += 2)
    {
        glm::vec3 p0 = worldVertices[indices[index]];
        glm::vec3 p1 = worldVertices[indices[index + 1]];

        DrawLine(renderer, p0, p1, color);
    }
}

void DebugDraw(Debug* debug, GameInput* input, PlatformAPI* platform)
{
    GameState*     state         = debug->state;
    Renderer*      renderer      = state->renderer;
    World*         world         = state->world;
    glm::uvec2     windowDim     = platform->WindowGetDimension();
    Camera*        camera        = state->camera;
    glm::mat4      projection    = glm::perspective(Radians(45.0f), (f32)windowDim.x / (f32)windowDim.y, 0.1f, 100.0f);
    glm::mat4      view          = CameraView(camera);
    EntityManager* entityManager = state->entityManager;
    Entity*        player        = &entityManager->entities[0];
    Mouse*         mouse         = &input->mouse;

    if (ButtonIsPressed(mouse->middle))
    {
        glm::vec3 mousePoint     = WorldMousePicking(camera, projection, windowDim, mouse->pos);
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
                    glm::vec3 nodePos = WorldGridCellToPosition(nodeCell);
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
                                glm::vec3  dstNodePos = WorldGridCellToPosition(dstCell);
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
                            glm::vec3         dstNodePos = WorldGridCellToPosition(dstCell);
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

            glm::vec3 playerCellCenter = WorldGridCellToPosition(playerCellIndex);
            glm::vec3 nodeCenter       = WorldGridCellToPosition(nodeIndex);

            DrawLine(renderer, playerCellCenter, nodeCenter, red);
        }
    }

    // Shooting
    //  {
    //      if (ButtonIsDown(mouse->left))
    //      {
    //          glm::vec3 end = WorldMousePicking(camera, projection, windowDim, mouse->pos);

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
            glm::vec3 lookAt{ sinf(entity->rotation.y), 0.0f, cosf(entity->rotation.y) };
            glm::vec3 p0{ entity->position.x, entity->aabb.max.y, entity->position.z };
            glm::vec3 p1 = p0 + (lookAt * 1.5f);

            DrawLine(renderer, p0, p1, blue);
        }

        // Entity AABB
        DebugDrawAABB(renderer, entity->position, entity->rotation.y, entity->aabb, red);

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
                    glm::vec3 snapPoint = worldCorners.arr[cornerIndex];
                    snapPoint.y         = 0.0f;

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
                        glm::vec3 start = WorldGridCellToPosition(path[i]);
                        glm::vec3 end   = WorldGridCellToPosition(path[i + 1]);

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