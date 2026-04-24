// internal void WorldComputeNodeEdges(World* world, EntityManager* manager, EntityType nodeType, u32 targetNodeIndex);
// internal f32  WorldGetMovementCost(cell_index from, cell_index to);
// internal f32  WorldHeuristicLength(cell_index from, cell_index to);
// internal void WorldAddTempNode(World* world, EntityManager* entityManager, EntityType nodeType, cell_index
// cellIndex); internal void WorldPopTempNodes(World* world); internal b32  WorldIsCellValidForEntity(World* world,
// cell_index cellIndex, Entity* entity);

glm::vec3 WorldMousePicking(Camera* camera, glm::uvec2 windowDim, glm::uvec2 mouse)
{
    f32 screenWidth  = (f32)windowDim.x;
    f32 screenHeight = (f32)windowDim.y;
    f32 mouseX       = (f32)mouse.x;
    f32 mouseY       = (f32)mouse.y;

    glm::mat4 inverseProjection = glm::inverse(CameraGetProjection(camera, (f32)windowDim.x / (f32)windowDim.y));
    glm::mat4 inverseView       = glm::inverse(CameraGetView(camera));

    // Viewport -> NDC
    glm::vec3 rayNdc;
    rayNdc.x = (2.0f * mouseX) / screenWidth - 1.0f;
    rayNdc.y = 1.0f - (2.0f * mouseY) / screenHeight;

    // NDC -> Clip
    glm::vec4 rayClip{ rayNdc.x, rayNdc.y, -1.0f, 1.0f };

    // Clip -> View
    glm::vec4 rayView = inverseProjection * rayClip;
    rayView.z         = -1.0f;
    rayView.w         = 0;

    // View -> World
    glm::vec4 rayWorld4 = inverseView * rayView;
    glm::vec3 rayWorld{ rayWorld4.x, rayWorld4.y, rayWorld4.z };
    rayWorld = SafeNorm(rayWorld);

    // Intersection with world plane
    f32       t     = -(camera->position.y / rayWorld.y);
    glm::vec3 point = camera->position + rayWorld * t;

    return point;
}

// b32 WorldIsPositionInBounds(glm::vec3 position)
//{
//     // Left limit
//     if (position.x < GRID_LEFT_LIMIT)
//     {
//         return false;
//     }
//     // Right limit
//     if (position.x > GRID_RIGHT_LIMIT)
//     {
//         return false;
//     }
//     // Top limit
//     if (position.z < GRID_TOP_LIMIT)
//     {
//         return false;
//     }
//     // Bottom limit
//     if (position.z > GRID_BOTTOM_LIMIT)
//     {
//         return false;
//     }

//    return true;
//}

// void WorldAddEntity(World* world, EntityManager* entityManager, Entity* entity)
//{
//     Assert(entity->type == EntityType_Obstacle);

//    EntityCellCorners entityCells = EntityGetCellCorners(entity);

//    // Corners (snapping)
//    {
//        for (u32 cellIndex = 0; cellIndex < ArrayCount(entityCells.arr); cellIndex++)
//        {
//            b32        duplicated      = false;
//            cell_index cornerCellIndex = entityCells.arr[cellIndex];
//            GridCell*  cell            = &world->grid[cornerCellIndex];

//            for (u32 entityPtrIndex = 0; entityPtrIndex < ArrayCount(cell->entities); entityPtrIndex++)
//            {
//                if (cell->entities[entityPtrIndex] == entity)
//                {
//                    duplicated = true;
//                    break;
//                }
//            }

//            if (!duplicated)
//            {
//                // Assert(cell->entityCount < ArrayCount(cell->entities));
//                cell->entities[cell->entityCount++] = entity;
//            }
//        }
//    }

//    // Occupied area
//    {
//        u32 beginRow = CELL_ROW(entityCells.bottomRight);
//        u32 endRow   = CELL_ROW(entityCells.topRight);
//        u32 beginCol = CELL_COL(entityCells.bottomLeft);
//        u32 endCol   = CELL_COL(entityCells.bottomRight);
//        for (u32 row = beginRow; row <= endRow; row++)
//        {
//            for (u32 col = beginCol; col <= endCol; col++)
//            {
//                world->grid[CELL_INDEX(row, col)].entityCount++;
//            }
//        }
//    }

//    WorldComputeStaticNodes(world, entityManager);
//}

// b32 WorldIsValidEntityPosition(World* world, Entity* entity)
//{
//     EntityCellCorners cells = EntityGetCellCorners(entity);
//     for (u32 cellIndex = 0; cellIndex < ArrayCount(cells.arr); cellIndex++)
//     {
//         cell_index cell = cells.arr[cellIndex];
//         if (!WorldIsValidCellIndex(cell) || !WorldIsCellValidForEntity(world, cell, entity))
//         {
//             return false;
//         }
//     }

//    return true;
//}

// void WorldComputeStaticNodes(World* world, EntityManager* manager)
//{
//     WorldPopTempNodes(world);

//    world->nodes.clear();
//    world->edges.clear();

//    for (u32 row = 0; row < GRID_ROWS; row++)
//    {
//        cell_index firstNotEmpty = CELL_EMPTY;
//        cell_index lastNotEmpty  = CELL_EMPTY;

//        for (u32 col = 0; col < GRID_COLS; col++)
//        {
//            cell_index cellIndex = CELL_INDEX(row, col);
//            GridCell   cell      = world->grid[cellIndex];

//            if (cell.entityCount > 0 && firstNotEmpty == CELL_EMPTY)
//            {
//                firstNotEmpty = cellIndex;

//                // Top-left corner
//                if (CELL_COL(firstNotEmpty) > GRID_MIN_COL)
//                {
//                    cell_index topLeft     = CELL_INDEX(CELL_ROW(firstNotEmpty) + 1, CELL_COL(firstNotEmpty) - 1);
//                    cell_index top         = firstNotEmpty + GRID_ROWS;
//                    GridCell   topLeftCell = world->grid[topLeft];
//                    GridCell   topCell     = world->grid[top];

//                    if (topCell.entityCount == 0)
//                    {
//                        world->nodes.push_back(topLeft);
//                    }
//                }

//                // Bottom-left corner
//                if (CELL_ROW(firstNotEmpty) > GRID_MIN_ROW && CELL_COL(firstNotEmpty) > GRID_MIN_COL)
//                {
//                    cell_index bottomLeft     = CELL_INDEX(CELL_ROW(firstNotEmpty) - 1, CELL_COL(firstNotEmpty) - 1);
//                    cell_index bottom         = firstNotEmpty - GRID_ROWS;
//                    GridCell   bottomLeftCell = world->grid[bottomLeft];
//                    GridCell   bottomCell     = world->grid[bottom];

//                    if (bottomCell.entityCount == 0)
//                    {
//                        world->nodes.push_back(bottomLeft);
//                    }
//                }
//            }
//            else if (cell.entityCount == 0 && firstNotEmpty != CELL_EMPTY && lastNotEmpty == CELL_EMPTY)
//            {
//                lastNotEmpty = CELL_INDEX(row, col - 1);

//                // Top-right corner
//                if (CELL_COL(lastNotEmpty) < GRID_MAX_COL)
//                {
//                    cell_index topRight     = CELL_INDEX(CELL_ROW(lastNotEmpty) + 1, CELL_COL(lastNotEmpty) + 1);
//                    cell_index top          = lastNotEmpty + GRID_ROWS;
//                    GridCell   topRightCell = world->grid[topRight];
//                    GridCell   topCell      = world->grid[top];

//                    if (topCell.entityCount == 0)
//                    {
//                        world->nodes.push_back(topRight);
//                    }
//                }

//                // Bottom-right corner
//                if (CELL_ROW(lastNotEmpty) > GRID_MIN_ROW)
//                {
//                    cell_index bottomRight     = CELL_INDEX(CELL_ROW(lastNotEmpty) - 1, CELL_COL(lastNotEmpty) + 1);
//                    cell_index bottom          = lastNotEmpty - GRID_ROWS;
//                    GridCell   bottomRightCell = world->grid[bottomRight];
//                    GridCell   bottomCell      = world->grid[bottom];

//                    if (bottomCell.entityCount == 0)
//                    {
//                        world->nodes.push_back(bottomRight);
//                    }
//                }

//                firstNotEmpty = CELL_EMPTY;
//                lastNotEmpty  = CELL_EMPTY;
//            }
//        }
//    }

//    world->edges.resize(world->nodes.size());
//    for (u32 nodeIndex = 0; nodeIndex < world->nodes.size(); nodeIndex++)
//    {
//        WorldComputeNodeEdges(world, manager, EntityType_Obstacle, nodeIndex);
//    }
//}

// void WorldUpdate(World* world, EntityManager* entityManager)
//{
//     WorldPopTempNodes(world);

//    for (u32 entityIndex = 0; entityIndex < entityManager->entityCount; entityIndex++)
//    {
//        Entity* entity = EntityGet(entityManager, entityIndex);
//        if (entity->health > 0 && (entity->type == EntityType_Enemy || entity->type == EntityType_Player))
//        {
//            WorldAddTempNode(world, entityManager, entity->type, WorldPositionToGridCell(entity->position));
//        }
//    }
//}

//// https://www.redblobgames.com/pathfinding/a-star/implementation.html
//// A* algorithm
//// Returns best path from start to goal in reverse order (start → ... → goal).
// std::vector<cell_index> WorldFindBestPath(World* world, EntityManager* entityManager, cell_index start, cell_index
// goal)
//{
//     std::unordered_map<cell_index, cell_index> cameFromMap{};
//     std::unordered_map<cell_index, f32>        costMap{};

//    u32 startNodeIndex = CELL_EMPTY;
//    for (u32 nodeIndex = 0; nodeIndex < world->nodes.size(); nodeIndex++)
//    {
//        if (world->nodes[nodeIndex] == start)
//        {
//            startNodeIndex = nodeIndex;
//            break;
//        }
//    }
//    Assert(startNodeIndex != CELL_EMPTY);

//    PriorityQueue<u32, f32> frontier;
//    frontier.put(startNodeIndex, 0);

//    cell_index startCell = world->nodes[startNodeIndex];

//    cameFromMap[start] = startCell;
//    costMap[start]     = 0;

//    while (!frontier.empty())
//    {
//        u32 currentIndex = frontier.get();

//        cell_index currentCell = world->nodes[currentIndex];

//        if (currentCell == goal)
//        {
//            break;
//        }

//        for (u32 edgeIndex = 0; edgeIndex < world->edges[currentIndex].size(); edgeIndex++)
//        {
//            u32 edgeNodeIndex = world->edges[currentIndex][edgeIndex];

//            cell_index nextCell = world->nodes[edgeNodeIndex];

//            f32 new_cost = costMap[currentCell] + WorldGetMovementCost(currentCell, nextCell);
//            if (costMap.find(nextCell) == costMap.end() || new_cost < costMap[nextCell])
//            {
//                costMap[nextCell] = new_cost;
//                f32 priority      = new_cost + WorldHeuristicLength(nextCell, goal);
//                frontier.put(edgeNodeIndex, priority);
//                cameFromMap[nextCell] = currentCell;
//            }
//        }
//    }

//    // Build path
//    std::vector<cell_index> path;
//    cell_index              current = goal;
//    if (cameFromMap.find(goal) == cameFromMap.end())
//    {
//        return path; // no path can be found
//    }
//    while (current != start)
//    {
//        path.push_back(current);
//        current = cameFromMap[current];
//    }
//    path.push_back(start);
//    return path;
//}

// internal void WorldComputeNodeEdges(World* world, EntityManager* manager, EntityType nodeType, u32 targetNodeIndex)
//{
//     glm::vec3 nodeAPos = WorldGridCellToPosition(world->nodes[targetNodeIndex]);

//    for (u32 nodeIndex = 0; nodeIndex < world->nodes.size(); nodeIndex++)
//    {
//        if (world->nodes[targetNodeIndex] != world->nodes[nodeIndex])
//        {
//            glm::vec3 nodeBPos = WorldGridCellToPosition(world->nodes[nodeIndex]);

//            b32 intersectEntity = false;
//            for (u32 entityIndex = 0; entityIndex < manager->entityCount; entityIndex++)
//            {
//                Entity* entity = &manager->entities[entityIndex];
//                if (entity->type == EntityType_Obstacle)
//                {
//                    AABB entityWorldAABB = EntityWorldAABB(entity);
//                    if (nodeType == EntityType_Enemy)
//                    {
//                        entityWorldAABB = AABBExpandXZ(entityWorldAABB, CELL_SIZE);
//                    }

//                    if (AABBSegmentIntersection(entityWorldAABB, nodeAPos, nodeBPos))
//                    {
//                        intersectEntity = true;
//                        break;
//                    }
//                }
//            }

//            if (!intersectEntity)
//            {
//                world->edges[targetNodeIndex].push_back(nodeIndex);
//            }
//        }
//    }
//}

// internal void WorldAddTempNode(World* world, EntityManager* entityManager, EntityType nodeType, cell_index cellIndex)
//{
//     u32 newNodeIndex = (u32)world->nodes.size();

//    world->nodes.push_back(cellIndex);
//    world->edges.push_back({});

//    WorldComputeNodeEdges(world, entityManager, nodeType, newNodeIndex);

//    // Bidirectional edges
//    std::vector<u32>& edges = world->edges[newNodeIndex];
//    for (u32 edgeIndex = 0; edgeIndex < edges.size(); edgeIndex++)
//    {
//        u32 nodeIndex = edges[edgeIndex];
//        Assert(nodeIndex != newNodeIndex);

//        world->edges[nodeIndex].push_back(newNodeIndex);
//    }

//    world->tempNodeCount++;
//}

// internal void WorldPopTempNodes(World* world)
//{
//     while (world->tempNodeCount)
//     {
//         u32 removedNodeIndex = (u32)world->nodes.size() - 1;
//         world->nodes.pop_back();
//         world->edges.pop_back();

//        for (u32 edgesIndex = 0; edgesIndex < world->edges.size(); edgesIndex++)
//        {
//            std::vector<u32>& edges = world->edges[edgesIndex];

//            for (auto it = edges.begin(); it != edges.end();)
//            {
//                if (*it == removedNodeIndex)
//                {
//                    it = edges.erase(it);
//                }
//                else
//                {
//                    it++;
//                }
//            }
//        }

//        world->tempNodeCount--;
//    }
//}

// internal f32 WorldGetMovementCost(cell_index from, cell_index to)
//{
//     f32 cost = 1.0f;

//    u32 fromCol = CELL_COL(from);
//    u32 fromRow = CELL_ROW(from);
//    u32 toCol   = CELL_COL(to);
//    u32 toRow   = CELL_ROW(to);

//    // Bottom-right diagonal
//    if ((fromCol + 1 == toCol) && (fromRow - 1) == toRow)
//    {
//        cost += 1.0f;
//    }
//    // Bottom-left diagonal
//    if ((fromCol - 1 == toCol) && (fromRow - 1) == toRow)
//    {
//        cost += 1.0f;
//    }
//    // Top-left diagonal
//    if ((fromCol - 1 == toCol) && (fromRow + 1) == toRow)
//    {
//        cost += 1.0f;
//    }
//    // Top-right diagonal
//    if ((fromCol + 1 == toCol) && (fromRow + 1) == toRow)
//    {
//        cost += 1.0f;
//    }

//    return cost;
//}

// internal f32 WorldHeuristicLength(cell_index from, cell_index to)
//{
//     glm::vec3 fromPos = WorldGridCellToPosition(from);
//     glm::vec3 toPos   = WorldGridCellToPosition(to);

//    return glm::length(toPos - fromPos);
//}

// internal b32 WorldIsCellValidForEntity(World* world, cell_index cellIndex, Entity* entity)
//{
//     GridCell* cell = &world->grid[cellIndex];

//    // Cell occupied by 4 entities
//    if (cell->entityCount == 4)
//    {
//        return false;
//    }

//    u32 verticalOrientedEntityCount   = 0;
//    u32 horizontalOrientedEntityCount = 0;

//    for (u32 entityPtrIndex = 0; entityPtrIndex < ArrayCount(cell->entities); entityPtrIndex++)
//    {
//        Entity* entityPtr = cell->entities[entityPtrIndex];
//        if (entityPtr)
//        {
//            if (EntityIsVerticalOriented(entityPtr))
//            {
//                verticalOrientedEntityCount++;
//            }
//            else
//            {
//                horizontalOrientedEntityCount++;
//            }
//        }
//    }
//    Assert(verticalOrientedEntityCount <= 2);
//    Assert(horizontalOrientedEntityCount <= 2);

//    b32 newEntityIsVertical = EntityIsVerticalOriented(entity);
//    if (newEntityIsVertical && verticalOrientedEntityCount < 2)
//    {
//        return true;
//    }
//    else if (!newEntityIsVertical && horizontalOrientedEntityCount < 2)
//    {
//        return true;
//    }

//    return false;
//}