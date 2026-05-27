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

void DebugUpdateAndRender(Debug* debug, GameInput* input, PlatformAPI* platform)
{
    GameState*     state         = debug->state;
    Renderer*      renderer      = state->renderer;
    glm::uvec2     windowDim     = platform->WindowGetDimension();
    EntityManager* entityManager = state->entityManager;
    Entity*        player        = &entityManager->entities[0];
    Mouse*         mouse         = &input->keyboard.mouse;
    Assets*        assets        = state->assets;

#if 1
    // Debug hide cursor
    if (ButtonIsPressed(input->debug.f4))
    {
        platform->CursorHide();
    }
    // Debug show cursor
    if (ButtonIsPressed(input->debug.f5))
    {
        platform->CursorShow();
    }
#endif

    // Debug animations
    if (ButtonIsPressed(input->debug.f3))
    {
        Entity* enemy = EntityGet(entityManager, 1);

        enemy->animation.time = 0.0f;
        if (enemy->animation.current)
        {
            enemy->animation.current = 0;
        }
        else
        {
            if (enemy->assetID == Model_ZombieMaleA)
            {
                // enemy->animation.current = AssetsAnimationGet(assets, Anim_ZombieMaleAttackLeft);
                // enemy->animation.current = AssetsAnimationGet(assets, Anim_ZombieMaleRunning);
                // enemy->animation.current = AssetsAnimationGet(assets, Anim_ZombieMaleWalkLimp);

                // enemy->animation.current = AssetsAnimationGet(assets, Anim_ZombieMaleCrawlingIdle);
                enemy->animation.current = AssetsAnimationGet(assets, Anim_ZombieMaleCrawlingForward);
            }
            else if (enemy->assetID == Model_ZombieFemaleA)
            {
                // enemy->animation.current = AssetsAnimationGet(assets, Anim_ZombieFemaleWalk);
                enemy->animation.current = AssetsAnimationGet(assets, Anim_ZombieFemaleAttackLeft);
                // enemy->animation.current = AssetsAnimationGet(assets, Anim_ZombieFemaleIdle);
                // enemy->animation.current = AssetsAnimationGet(assets, Anim_ZombieFemaleCrawlingIdle);
                // enemy->animation.current = AssetsAnimationGet(assets, Anim_ZombieFemaleCrawlingForward);
            }
            else
            {
                Assert(0);
            }
        }
    }

#if DEBUG_COORDINATES
    char coordBuffer[64];
    sprintf(coordBuffer, "(%d,%d)", (u32)player->position.x, (u32)player->position.z);
    f32       scale    = 0.7f;
    glm::vec2 textSize = UI_GetTextSize(state->ui, coordBuffer, scale);
    DrawText(renderer, coordBuffer, { 0.0f, 8.0f }, color_white, scale);
#endif

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
            DrawLine(renderer, { (f32)col, y, (f32)minRow }, { (f32)col, y, (f32)maxRow }, color_black);
        }
        // Horizontal lines
        for (s32 row = minRow; row <= maxRow; row++)
        {
            DrawLine(renderer, { (f32)minCol, y, (f32)row }, { (f32)maxCol, y, (f32)row }, color_black);
        }

        //        // Debug graph
        //        {
        //            // Nodes and edges
        //            for (u32 nodeIndex = 0; nodeIndex < world->nodes.size(); nodeIndex++)
        //            {
        //                cell_index nodeCell = world->nodes[nodeIndex];

        //                if (nodeCell != CELL_EMPTY)
        //                {
        //                    glm::vec3 nodePos = WorldGridCellToPosition(nodeCell);
        //                    DebugDrawGridCell(renderer, nodeCell, color_green);

        //// Select cell edges
        // #if 1
        //                     if (debug->selectedCellIndex != CELL_EMPTY)
        //                     {
        //                         if (debug->selectedCellIndex == nodeCell)
        //                         {
        //                             for (auto edgeIndex : world->edges[nodeIndex])
        //                             {
        //                                 cell_index dstCell    = world->nodes[edgeIndex];
        //                                 glm::vec3  dstNodePos = WorldGridCellToPosition(dstCell);
        //                                 nodePos.y             = 0.1f;
        //                                 dstNodePos.y          = 0.1f;
        //                                 DrawLine(renderer, nodePos, dstNodePos, color_magenta);
        //                             }
        //                         }
        //                     }
        // #endif
        //// All edges
        // #if 0
        //                         for (auto edgeIndex : world->edges[nodeIndex])
        //                         {
        //                             cell_index dstCell    = world->nodes[edgeIndex];
        //                             glm::vec3         dstNodePos = WorldGridCellToPosition(dstCell);
        //                             nodePos.y             = 0.1f;
        //                             dstNodePos.y          = 0.1f;
        //							DrawLine(renderer, nodePos, dstNodePos, color_magenta);
        //                         }
        // #endif
        //                 }
        //             }

        //            // Occupied cells
        //            for (cell_index cellIndex = 0; cellIndex < GRID_CELLS; cellIndex++)
        //            {
        //                if (world->grid[cellIndex].entityCount > 0)
        //                {
        //                    // DebugDrawGridCell(renderer, cellIndex, color_blue);
        //                }
        //            }
        //        }
    }

    // Debug entities
    for (u32 entityIndex = 0; entityIndex < entityManager->entityCount; entityIndex++)
    {
        Entity* entity = EntityGet(entityManager, entityIndex);

        // Entity rotation
        {
            if (entity->type == EntityType_Enemy)
            {
                glm::vec3 lookAt{ sinf(entity->rotation.y), 0.0f, cosf(entity->rotation.y) };
                glm::vec3 p0{ entity->position.x, entity->aabb.max.y, entity->position.z };
                glm::vec3 p1 = p0 + (lookAt * 1.5f);

                DrawLine(renderer, p0, p1, color_blue);
            }
        }

// World collider
#if DEBUG_COLLIDERS
        if (entity->type == EntityType_Collider)
        {
            DebugDrawAABB(renderer, entity->position, entity->rotation.y, entity->aabb, color_yellow);
        }
#endif

        // Entity Y orientation
        // if (entity->type == EntityType_Enemy || entity->type == EntityType_Player)
        if (entity->type == EntityType_Enemy)
        {
            glm::vec4 color = entity->type == EntityType_Enemy ? color_red : color_blue;

            // f32 radius = enemyHitRadius;
            f32 radius = capsuleRadius;

            glm::vec3 start = { entity->position.x, 0.0f, entity->position.z };
            glm::vec3 end   = { entity->position.x, 2.2f, entity->position.z };

            DebugDrawCircle(renderer, start, radius, color);
            DebugDrawCircle(renderer, end, radius, color);

            // Left
            DrawLine(renderer, { start.x - radius, 0.0f, start.z }, { start.x - radius, 2.0f, start.z }, color);
            // Right
            DrawLine(renderer, { start.x + radius, 0.0f, start.z }, { start.x + radius, 2.0f, start.z }, color);
            // Top
            DrawLine(renderer, { start.x, 0.0f, start.z - radius }, { start.x, 2.0f, start.z - radius }, color);
            // Bottom
            DrawLine(renderer, { start.x, 0.0f, start.z + radius }, { start.x, 2.0f, start.z + radius }, color);
        }
    }
}