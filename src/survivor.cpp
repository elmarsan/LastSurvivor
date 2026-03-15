#include "survivor.h"

#include "survivor_renderer_opengl.cpp"
#include "survivor_debug_geometry.cpp"
#include "survivor_debug.cpp"
#include "survivor_obj.cpp"
#include "survivor_world.cpp"
#include "survivor_entity.cpp"
#include "survivor_build.cpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

global_variable u32 rectIndices[]   = { 0, 1, 2, 0, 2, 3 };
global_variable u32 rectVertexCount = 4;
global_variable u32 rectIndexCount  = 6;
global_variable v4  green{ 0.2f, 1.0f, 0.0f, 1.0f };
global_variable v4  red{ 1.0f, 0.0f, 0.0f, 1.0f };
global_variable v4  blue{ 0.2f, 0.4f, 1.0f, 1.0f };
global_variable v4  white{ 1.0f, 1.0f, 1.0f, 1.0f };
global_variable v4  black{ 0.0f, 0.0f, 0.0f, 1.0f };
global_variable v4  magenta{ 1.0f, 0.0f, 1.0f, 1.0f };
global_variable v4  yellow{ 1.0f, 1.0f, 0.0f, 1.0f };
global_variable v4  orange{ 0.87f, 0.39f, 0.04f, 1.0f };

// TODO: Tweak values
global_variable f32 maxSpeed          = 8.3f;
global_variable f32 frictionForce     = 20.0f;
global_variable f32 moveAcceleration  = 40.0f;
global_variable f32 knockbackForce    = 17.0f;
global_variable f32 rotationSpeed     = 0.05f;
global_variable f32 enemyHitRadius    = 0.75f;
global_variable f32 enemyMaxSpeed     = 1.0f;
global_variable f32 enemyAcceleration = 10.0f;
global_variable s32 maxHealth         = 100;

// TODO
/*
- (Batch) Review batch buffer size: Ideally, should be large enough to handle a single render call per
  frame. Otherwise, I'm not sure how to send multiple draw calls in the same frame using batching approach.
- (Batch) Texture index assignation (remove index parameter in batch function and hardcoded values)
- (Renderer) Rethink TextureAlloc. See how to alloc simple textures as the white one. Check for different parameters
  (swizzle, min/mag filters, etc)
- (Renderer) Rethink DrawBuffer render command. (Is not enough flexible for batching and is not easy to change the
primitive type)
- (Audio) Make easy to tweak volumes (ignore db conversion)
- (Misc): Temporal arenas
- (Game): gamepad controller
*/

inline void BatchRect(BatchBuffer* batch, v2 topLeft, v2 bottomRight, v4 color)
{
    if ((batch->vertexCount + rectVertexCount > batch->maxVertexCount) ||
        (batch->indexCount + rectIndexCount > batch->maxIndexCount))
    {
        Assert(0);
    }

    for (u32 index = 0; index < rectIndexCount; index++)
    {
        *batch->indexBufferPtr = rectIndices[index] + batch->vertexCount;
        batch->indexBufferPtr++;

        batch->indexCount++;
    }

    // Top-right
    batch->vertexBufferPtr->position     = { topLeft.x + bottomRight.x, topLeft.y };
    batch->vertexBufferPtr->color        = color;
    batch->vertexBufferPtr->textureIndex = 0;
    batch->vertexBufferPtr++;
    // Bottom-right
    batch->vertexBufferPtr->position     = { topLeft.x + bottomRight.x, topLeft.y + bottomRight.y };
    batch->vertexBufferPtr->color        = color;
    batch->vertexBufferPtr->textureIndex = 0;
    batch->vertexBufferPtr++;
    // Bottom-left
    batch->vertexBufferPtr->position     = { topLeft.x, topLeft.y + bottomRight.y };
    batch->vertexBufferPtr->color        = color;
    batch->vertexBufferPtr->textureIndex = 0;
    batch->vertexBufferPtr++;
    // Top-left
    batch->vertexBufferPtr->position     = { topLeft.x, topLeft.y };
    batch->vertexBufferPtr->color        = color;
    batch->vertexBufferPtr->textureIndex = 0;
    batch->vertexBufferPtr++;

    batch->vertexCount += 4;
}

inline void BatchTextureRect(BatchBuffer* batch, v2 topLeft, v2 bottomRight, Texture* texture, u32 textureIndex = 1)
{
    if ((batch->vertexCount + rectVertexCount > batch->maxVertexCount) ||
        (batch->indexCount + rectIndexCount > batch->maxIndexCount))
    {
        Assert(0);
    }

    BatchVertex* vertexBufferPtr = batch->vertexBufferPtr;
    BatchRect(batch, topLeft, bottomRight, { 1.0f, 1.0f, 1.0f, 1.0f });

    // Top-right
    vertexBufferPtr->uv           = { 1.0f, 1.0f };
    vertexBufferPtr->textureIndex = textureIndex;
    vertexBufferPtr++;
    // Bottom-right
    vertexBufferPtr->uv           = { 1.0f, 0.0f };
    vertexBufferPtr->textureIndex = textureIndex;
    vertexBufferPtr++;
    // Bottom-left
    vertexBufferPtr->uv           = { 0.0f, 0.0f };
    vertexBufferPtr->textureIndex = textureIndex;
    vertexBufferPtr++;
    // Top-left
    vertexBufferPtr->uv           = { 0.0f, 1.0f };
    vertexBufferPtr->textureIndex = textureIndex;
}

inline void BatchTextureSubRect(BatchBuffer* batch, v2 topLeft, v2 bottomRight, Texture* texture, v2 textureTopLeft,
                                v2 textureBottomRight, u32 textureIndex = 1)
{
    if ((batch->vertexCount + rectVertexCount > batch->maxVertexCount) ||
        (batch->indexCount + rectIndexCount > batch->maxIndexCount))
    {
        Assert(0);
    }

    f32 textureW = (1.0f / texture->width) * textureBottomRight.x;
    f32 textureH = (1.0f / texture->height) * textureBottomRight.y;
    f32 textureX = (1.0f / texture->width) * textureTopLeft.x;
    f32 textureY = (1.0f / texture->height) * textureTopLeft.y;

    BatchVertex* vertexBufferPtr = batch->vertexBufferPtr;
    BatchRect(batch, topLeft, bottomRight, { 1.0f, 1.0f, 1.0f, 1.0f });

    // Top-right
    vertexBufferPtr->uv           = { textureX + textureW, textureY };
    vertexBufferPtr->textureIndex = textureIndex;
    vertexBufferPtr++;
    // Bottom-right
    vertexBufferPtr->uv           = { textureX + textureW, textureY + textureH };
    vertexBufferPtr->textureIndex = textureIndex;
    vertexBufferPtr++;
    // Bottom-left
    vertexBufferPtr->uv           = { textureX, textureY + textureH };
    vertexBufferPtr->textureIndex = textureIndex;
    vertexBufferPtr++;
    // Top-left
    vertexBufferPtr->uv           = { textureX, textureY };
    vertexBufferPtr->textureIndex = textureIndex;
}

inline void BatchText(GameState* state, BatchBuffer* batch, char* text, v2 position, v4 color, f32 scale = 1.0f)
{
    size_t textLength      = strlen(text);
    u32    textVertexCount = (u32)textLength * rectVertexCount;
    u32    textIndexCount  = (u32)textLength * rectIndexCount;

    if ((batch->vertexCount + textVertexCount > batch->maxVertexCount) ||
        (batch->indexCount + textIndexCount > batch->maxIndexCount))
    {
        Assert(0);
    }

    v2 rectTopLeft{ 0.0f, 0.0f };
    rectTopLeft += position;

    while (*text)
    {
        TTFGlyph* ttfChar = &state->ttfChars[*text++ - TTF_FIRST_GLYPH_OFFSET];

        rectTopLeft.x += (ttfChar->xoff * scale);
        rectTopLeft.y = position.y + (ttfChar->yoff * scale);
        v2 rectBottomRight{ ((f32)ttfChar->x1 - (f32)ttfChar->x0) * scale,
                            ((f32)ttfChar->y1 - (f32)ttfChar->y0) * scale };

        v2 subrectTopLeft{ (f32)ttfChar->x0 + ttfChar->s0, (f32)ttfChar->y0 + ttfChar->t0 };
        v2 subrectBottomRight{ ((f32)ttfChar->x1 - (f32)ttfChar->x0) - ttfChar->s1,
                               ((f32)ttfChar->y1 - (f32)ttfChar->y0) - ttfChar->t1 };

        BatchVertex* verterBufferPtr = batch->vertexBufferPtr;
        BatchTextureSubRect(batch, rectTopLeft, rectBottomRight, state->glyphAtlas, subrectTopLeft, subrectBottomRight,
                            2);

        verterBufferPtr->color = color;
        verterBufferPtr++;
        verterBufferPtr->color = color;
        verterBufferPtr++;
        verterBufferPtr->color = color;
        verterBufferPtr++;
        verterBufferPtr->color = color;
        verterBufferPtr++;

        rectTopLeft.x += (ttfChar->xadvance * scale);
    }
}

inline Entity* EntityGet(GameState* state, u32 index) { return &state->entities[index]; }

inline Entity* EntityNew(GameState* state, EntityType type)
{
    Assert(state->entityCount < ArrayCount(state->entities));

    Entity* entity = &state->entities[state->entityCount++];
    entity->type   = type;
    entity->health = maxHealth;

    return entity;
}

inline void EntityRemove(GameState* state, Entity* entity)
{
    u32 index = 0;
    for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++)
    {
        if (entity == &state->entities[entityIndex])
        {
            index = entityIndex;
            break;
        }
    }

    while (index < state->entityCount)
    {
        state->entities[index] = state->entities[index + 1];
        index++;
    }

    state->entityCount--;
}

inline void EntitiesRemoveFlag(GameState* state, u32 flag)
{
    for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++)
    {
        Entity* entity = EntityGet(state, entityIndex);
        entity->flags &= ~flag;
    }
}

// TODO: Move to debug?
void DebugDrawGridCell(DebugState* debug, OpenGL* opengl, cell_index cell, v4 color)
{
    s32 minCol = -(GRID_COLS / 2);
    s32 minRow = -(GRID_ROWS / 2);
    s32 row    = CELL_ROW(cell);
    s32 col    = CELL_COL(cell);

    v3 cellHalfExtent = { CELL_HALF, 0.0f, CELL_HALF };
    v3 worldPosition{ (col + minRow) + cellHalfExtent.x, 0.02f, (f32) - (row + minCol) - cellHalfExtent.z };

    DebugDrawPlane(debug, opengl, worldPosition, cellHalfExtent, color);
}

// Graph functions
#define GRAPH_EMPTY_NODE 0xFFFFFFFF

f32                     GraphGetMovementCost(cell_index from, cell_index to);
std::vector<cell_index> GraphFindBestPath(Graph* graph, cell_index start, cell_index goal);
void                    GraphComputeNodeEdges(GameState* state, u32 targetNodeIndex);
u32                     GraphAddNode(GameState* state, cell_index cell, EntityType type);
void                    GraphRemoveNode(GameState* state, u32 nodeIndex);
void                    GraphInit(GameState* state);
f32                     GraphHeuristicLength(cell_index a, cell_index b);

inline f32 GraphHeuristicLength(cell_index a, cell_index b)
{
    v3 aPos = WorldGridCellToPosition(a);
    v3 bPos = WorldGridCellToPosition(b);

    return Length(bPos - aPos);
}

f32 GraphGetMovementCost(cell_index from, cell_index to)
{
    f32 cost = 1.0f;

    u32 fromCol = CELL_COL(from);
    u32 fromRow = CELL_ROW(from);
    u32 toCol   = CELL_COL(to);
    u32 toRow   = CELL_ROW(to);

    // Bottom-right diagonal
    if ((fromCol + 1 == toCol) && (fromRow - 1) == toRow)
    {
        cost += 1.0f;
    }
    // Bottom-left diagonal
    if ((fromCol - 1 == toCol) && (fromRow - 1) == toRow)
    {
        cost += 1.0f;
    }
    // Top-left diagonal
    if ((fromCol - 1 == toCol) && (fromRow + 1) == toRow)
    {
        cost += 1.0f;
    }
    // Top-right diagonal
    if ((fromCol + 1 == toCol) && (fromRow + 1) == toRow)
    {
        cost += 1.0f;
    }

    return cost;
}

// https://www.redblobgames.com/pathfinding/a-star/implementation.html
// A* algorithm
// Returns best path from start to goal in reverse order (start → ... → goal).
std::vector<cell_index> GraphFindBestPath(Graph* graph, cell_index start, cell_index goal)
{
    std::unordered_map<cell_index, cell_index> cameFromMap{};
    std::unordered_map<cell_index, f32>        costMap{};

    u32 startNodeIndex = GRAPH_EMPTY_NODE;
    for (u32 nodeIndex = 0; nodeIndex < graph->nodes.size(); nodeIndex++)
    {
        if (graph->nodes[nodeIndex] == start)
        {
            startNodeIndex = nodeIndex;
            break;
        }
    }
    Assert(startNodeIndex != GRAPH_EMPTY_NODE);

    PriorityQueue<u32, f32> frontier;
    frontier.put(startNodeIndex, 0);

    cell_index startCell = graph->nodes[startNodeIndex];

    cameFromMap[start] = startCell;
    costMap[start]     = 0;

    while (!frontier.empty())
    {
        u32 currentIndex = frontier.get();

        cell_index currentCell = graph->nodes[currentIndex];

        if (currentCell == goal)
        {
            break;
        }

        for (u32 edgeIndex = 0; edgeIndex < graph->edges[currentIndex].size(); edgeIndex++)
        {
            u32 edgeNodeIndex = graph->edges[currentIndex][edgeIndex];

            cell_index nextCell = graph->nodes[edgeNodeIndex];

            f32 new_cost = costMap[currentCell] + GraphGetMovementCost(currentCell, nextCell);
            if (costMap.find(nextCell) == costMap.end() || new_cost < costMap[nextCell])
            {
                costMap[nextCell] = new_cost;
                f32 priority      = new_cost + GraphHeuristicLength(nextCell, goal);
                frontier.put(edgeNodeIndex, priority);
                cameFromMap[nextCell] = currentCell;
            }
        }
    }

    // Build path
    std::vector<cell_index> path;
    cell_index              current = goal;
    if (cameFromMap.find(goal) == cameFromMap.end())
    {
        return path; // no path can be found
    }
    while (current != start)
    {
        path.push_back(current);
        current = cameFromMap[current];
    }
    path.push_back(start);
    return path;
}

void GraphComputeNodeEdges(GameState* state, u32 targetNodeIndex, EntityType type)
{
    Graph* graph = state->graph;
    v3     nodeA = WorldGridCellToPosition(graph->nodes[targetNodeIndex]);

    for (u32 nodeIndex = 0; nodeIndex < graph->nodes.size(); nodeIndex++)
    {
        if (nodeIndex != targetNodeIndex && graph->nodes[nodeIndex] != GRAPH_EMPTY_NODE)
        {
            v3 nodeB = WorldGridCellToPosition(graph->nodes[nodeIndex]);

            b32 intersectEntity = false;
            for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++)
            {
                Entity* entity = EntityGet(state, entityIndex);
                if (entity->type == EntityType_Obstacle)
                {
                    AABB entityWorldAABB = EntityWorldAABB(entity);
                    entityWorldAABB      = AABBExpandXZ(entityWorldAABB, CELL_SIZE);

                    if (AABBSegmentIntersection(entityWorldAABB, nodeA, nodeB))
                    {
                        intersectEntity = true;
                        break;
                    }
                }
            }

            if (!intersectEntity)
            {
                graph->edges[targetNodeIndex].push_back(nodeIndex);
            }
        }
    }
}

u32 GraphAddNode(GameState* state, cell_index cell, EntityType entityType)
{
    Graph* graph = state->graph;

    u32 newNodeIndex = GRAPH_EMPTY_NODE;

    // Find for an empty node
    for (u32 nodeIndex = 0; nodeIndex < graph->nodes.size(); nodeIndex++)
    {
        if (graph->nodes[nodeIndex] == GRAPH_EMPTY_NODE)
        {
            // Reusing existing node
            newNodeIndex               = nodeIndex;
            graph->nodes[newNodeIndex] = cell;
            graph->edges[newNodeIndex].clear();
            break;
        }
    }

    // Append new node
    if (newNodeIndex == GRAPH_EMPTY_NODE)
    {
        newNodeIndex = (u32)graph->nodes.size();
        graph->nodes.push_back(cell);
        graph->edges.push_back({});
    }

    GraphComputeNodeEdges(state, newNodeIndex, entityType);

    // Bidirectional edges
    std::vector<u32>& edges = graph->edges[newNodeIndex];
    for (u32 edgeIndex = 0; edgeIndex < edges.size(); edgeIndex++)
    {
        u32 nodeIndex = edges[edgeIndex];
        Assert(nodeIndex != newNodeIndex);

        graph->edges[nodeIndex].push_back(newNodeIndex);
    }

    return newNodeIndex;
}

inline void GraphRemoveNode(GameState* state, u32 nodeIndex)
{
    Graph* graph            = state->graph;
    graph->nodes[nodeIndex] = GRAPH_EMPTY_NODE;

    for (u32 nodeEdgesIndex = 0; nodeEdgesIndex < graph->nodes.size(); nodeEdgesIndex++)
    {
        if (nodeEdgesIndex != nodeIndex)
        {
            auto& edges = graph->edges[nodeEdgesIndex];

            for (auto it = edges.begin(); it != edges.end();)
            {
                if (*it == nodeIndex)
                {
                    it = edges.erase(it);
                }
                else
                {
                    it++;
                }
            }
        }
    }
}

void GraphAddEntity(GameState* state, Entity* entity)
{
    Graph*            graph   = state->graph;
    EntityCellCorners corners = EntityGetCellCorners(entity);

    // Top-left corner
    GraphAddNode(state, CELL_INDEX(CELL_ROW(corners.topLeft) + 1, CELL_COL(corners.topLeft) - 1), entity->type);
    // Top-right corner
    GraphAddNode(state, CELL_INDEX(CELL_ROW(corners.topRight) + 1, CELL_COL(corners.topRight) + 1), entity->type);
    // Bottom-left corner
    GraphAddNode(state, CELL_INDEX(CELL_ROW(corners.bottomLeft) - 1, CELL_COL(corners.bottomLeft) - 1), entity->type);
    // Bottom-right corner
    GraphAddNode(state, CELL_INDEX(CELL_ROW(corners.bottomRight) - 1, CELL_COL(corners.bottomRight) + 1), entity->type);

    // for (u32 nodeIndex = 0; nodeIndex < graph->nodes.size(); nodeIndex++)
    //{
    //     GraphComputeNodeEdges(state, nodeIndex);
    // }

    graph->edges.clear();
    graph->edges.resize(graph->nodes.size());
    for (u32 nodeIndex = 0; nodeIndex < graph->nodes.size(); nodeIndex++)
    {
        GraphComputeNodeEdges(state, nodeIndex, entity->type);
    }
}

void GraphInit(GameState* state)
{
    // Graph* graph = state->graph;

    for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++)
    {
        Entity* entity = EntityGet(state, entityIndex);
        if (entity->type == EntityType_Obstacle)
        {
            GraphAddEntity(state, entity);
        }
    }
}

void EntityAttack(GameState* state, Entity* entity, Weapon* weapon, v3 dir)
{
    if (weapon->type == WeaponType_Hand)
    {
        Assert(entity->targetEntity);

        entity->targetEntity->velocity = { 0.0f, 0.0f, 0.0f };
        entity->targetEntity->velocity += dir * weapon->knockbackforce;
        entity->targetEntity->flags |= EntityFlag_InKnockback;
        entity->targetEntity->health -= weapon->damage;

        if (entity->targetEntity->health <= 0)
        {
            EntityRemove(state, entity->targetEntity);
        }
    }
    else
    {
        Ray shot    = { 0 };
        shot.origin = entity->position;
        shot.dir    = dir;

        for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++)
        {
            Entity* targetEntity = EntityGet(state, entityIndex);
            if (targetEntity->type == EntityType_Enemy)
            {
                AABB entityWorldAABB = AABBToWorld(targetEntity->aabb, targetEntity->position);

                if (AABBRayIntersection(entityWorldAABB, shot))
                {
                    v3 shotDir = Norm(targetEntity->position - entity->position);

                    targetEntity->flags |= EntityFlag_InKnockback;
                    targetEntity->velocity += shotDir * weapon->knockbackforce;
                    targetEntity->health -= weapon->damage;

                    // TODO: Move to update ???
                    if (targetEntity->health <= 0)
                    {
                        EntityRemove(state, targetEntity);
                    }

                    // Note: break stops the projectile trajectory, this way projectile can only impact once.
                    // TODO: Decide if allow penetration
                    break;
                }
            }
        }
    }
}

void PlayerUpdate(GameState* state, Entity* player, f32 delta, PlatformAPI* platform, GameInputController* controller,
                  Mouse* mouse, v3 cameraOffset)
{
    Assert(player->type == EntityType_Player);

    v2u     windowDim  = platform->WindowGetDimension();
    Camera* camera     = state->camera;
    mat4x4  projection = Perspective(Radians(45.0f), (f32)windowDim.w / (f32)windowDim.h, 0.1f, 100.0f);
    mat4x4  view       = CameraView(camera);

    if (controller->isConnected)
    {
        if (controller->isAnalog)
        {
            if (ButtonIsPressed(controller->rightTrigger))
            {
                platform->AudioClipPlay(state->pistolShot, 0);
            }
            if (ButtonIsDown(controller->leftTrigger))
            {
                platform->Logf("Gamepad aiming");
            }
        }
        else
        {
            v3 playerDirection = { 0.0f, 0.0f, 0.0f };
            v3 crosshairPoint  = WorldMousePicking(camera, projection, windowDim, mouse->pos);

            if (ButtonIsDown(controller->moveUp))
            {
                playerDirection.z = -1.0f;
            }
            if (ButtonIsDown(controller->moveDown))
            {
                playerDirection.z = 1.0f;
            }
            if (ButtonIsDown(controller->moveLeft))
            {
                playerDirection.x = -1.0f;
            }
            if (ButtonIsDown(controller->moveRight))
            {
                playerDirection.x = 1.0f;
            }
            playerDirection = Norm(playerDirection);

            if (ButtonIsPressed(mouse->left))
            {
                v3 dir = Norm(crosshairPoint - player->position);
                EntityAttack(state, player, &gWeaponPistol, dir);
            }

            // Player rotation
            {
                v3  dir       = Norm(crosshairPoint - player->position);
                f32 targetYaw = -atan2f(dir.x, -dir.z);
                f32 deltaYaw  = targetYaw - player->yaw;
                deltaYaw      = fmodf(deltaYaw + Pi, 2.0f * Pi) - Pi; // Wrap to [-Pi, Pi]
                player->yaw += deltaYaw * rotationSpeed;
            }

            // Deceleration
            f32 playerSpeed = Length(player->velocity);
            if (playerSpeed > 0.0f)
            {
                f32 decelerationStep = frictionForce * delta;

                if (playerSpeed <= decelerationStep)
                {
                    player->velocity = { 0.0f, 0.0f, 0.0f };
                    player->flags &= ~EntityFlag_InKnockback;
                }
                else
                {
                    if (player->flags & EntityFlag_InKnockback)
                    {
                        decelerationStep *= 2.0f;
                    }

                    player->velocity -= (player->velocity / playerSpeed) * decelerationStep;
                }
            }

            // Acceleration
            if (!(player->flags & EntityFlag_InKnockback))
            {
                v3 acceleration = playerDirection * moveAcceleration;
                player->velocity += acceleration * delta;
                if (Length(player->velocity) > maxSpeed)
                {
                    player->velocity = Norm(player->velocity) * maxSpeed;
                }
            }

            v3 newPlayerPosition = player->position + (player->velocity * delta);
            v3 correction        = { 0.0f, 0.0f, 0.0f };
            v3 totalCorrection   = { 0 };

            AABB playerWorldAABB = AABBToWorld(player->aabb, player->position);

            // Collision detection
            for (u32 entityIndex = 1; entityIndex < state->entityCount; entityIndex++)
            {
                Entity* entity = EntityGet(state, entityIndex);

                AABB intersection;
                if (EntitiesIntersect(player, entity, &intersection))
                {
                    if (player->flags & EntityFlag_InKnockback)
                    {
                        player->flags &= ~EntityFlag_InKnockback;
                    }

                    v3 penetration;
                    penetration.x = intersection.max.x - intersection.min.x;
                    penetration.y = intersection.max.y - intersection.min.y;
                    penetration.z = intersection.max.z - intersection.min.z;

                    // ----------------------------------------------------------------------------
                    // Correct using the minimal penetration axis
                    if (penetration.x < penetration.z)
                    {
                        correction.x = penetration.x;
                    }
                    else
                    {
                        correction.z = penetration.z;
                    }
                    // ----------------------------------------------------------------------------

                    // ----------------------------------------------------------------------------
                    // Determine correction axis
                    if (newPlayerPosition.x < entity->position.x)
                    {
                        correction.x = -correction.x;
                    }
                    if (newPlayerPosition.z < entity->position.z)
                    {
                        correction.z = -correction.z;
                    }
                    // ----------------------------------------------------------------------------

                    // ----------------------------------------------------------------------------
                    // Use the greatest penetration to resolve collisions
                    if (Abs(correction.x) > Abs(totalCorrection.x))
                    {
                        totalCorrection.x = correction.x;
                    }
                    if (Abs(correction.z) > Abs(totalCorrection.z))
                    {
                        totalCorrection.z = correction.z;
                    }
                    // ----------------------------------------------------------------------------
                }
            }

            // World limit
            // Note: Enemies might spawn away the limit.
            // This logic does not affect enemies.
            //
            {
                // Left limit
                if (newPlayerPosition.x < GRID_LEFT_LIMIT)
                {
                    newPlayerPosition.x = GRID_LEFT_LIMIT;
                }
                // Right limit
                if (newPlayerPosition.x > GRID_RIGHT_LIMIT)
                {
                    newPlayerPosition.x = GRID_RIGHT_LIMIT;
                }
                // Top limit
                if (newPlayerPosition.z < GRID_TOP_LIMIT)
                {
                    newPlayerPosition.z = GRID_TOP_LIMIT;
                }
                // Bottom limitd
                if (newPlayerPosition.z > GRID_BOTTOM_LIMIT)
                {
                    newPlayerPosition.z = GRID_BOTTOM_LIMIT;
                }
            }

            newPlayerPosition += totalCorrection;
            player->position = newPlayerPosition;
            camera->position = player->position + cameraOffset;
        }
    }
}

void EnemyUpdate(GameState* state, Entity* entity, f32 delta)
{
    Assert(entity->type == EntityType_Enemy);
    Assert(entity->targetEntity);

    // Attack
    {
        v3 dir = Norm(entity->velocity);
        v2 hitRectMinCorner{ entity->position.x - enemyHitRadius, entity->position.z - enemyHitRadius };
        v2 hitRectMaxCorner{ entity->position.x + enemyHitRadius, entity->position.z + enemyHitRadius };

        v2 targetMinCorner;
        v2 targetMaxCorner;
        targetMinCorner.x = entity->targetEntity->position.x - (entity->size.x * 0.5f);
        targetMinCorner.y = entity->targetEntity->position.z - (entity->size.z * 0.5f);
        targetMaxCorner.x = entity->targetEntity->position.x + (entity->size.x * 0.5f);
        targetMaxCorner.y = entity->targetEntity->position.z + (entity->size.z * 0.5f);

        b32 overlapsX = hitRectMinCorner.x <= targetMaxCorner.x && hitRectMaxCorner.x >= targetMinCorner.x;
        b32 overlapsZ = hitRectMinCorner.y <= targetMaxCorner.y && hitRectMaxCorner.y >= targetMinCorner.y;
        if (overlapsX && overlapsZ)
        {
            EntityAttack(state, entity, &gWeaponHand, dir);
        }
    }

    //----------------------------------------------------------------------------
    // Path finding
    cell_index enemyCell           = WorldPositionToGridCell(entity->position);
    u32        enemyGraphNodeIndex = GraphAddNode(state, enemyCell, entity->type);

    v3                      entityDir{ 0.0f, 0.0f, 0.0f };
    cell_index              originCellIndex = WorldPositionToGridCell(entity->position);
    cell_index              targetCellIndex = WorldPositionToGridCell(entity->targetEntity->position);
    std::vector<cell_index> path            = GraphFindBestPath(state->graph, originCellIndex, targetCellIndex);

    b32 availablePath = false;

    if (!path.empty())
    {
        v3 targetPosition = WorldGridCellToPosition(path[path.size() - 2]);
        entityDir         = Norm(targetPosition - entity->position);
        entityDir.y       = 0.0f;
        // entity->yaw       = (f32)atan2(entityDir.x, entityDir.z);
        availablePath = true;
    }
    else
    {
        entityDir   = Norm(entity->targetEntity->position - entity->position);
        entityDir.y = 0.0f;
    }
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // Entity acceleration
    if (!(entity->flags & EntityFlag_InKnockback))
    {
        v3 acceleration = entityDir * enemyAcceleration;
        entity->velocity += acceleration * delta;
        // TODO: Use constant speed for enemies???
        if (Length(entity->velocity) > enemyMaxSpeed)
        {
            entity->velocity = Norm(entity->velocity) * enemyMaxSpeed;
        }
    }
    else
    {
        // Friction force
        entity->velocity *= 0.70f;

        f32 speed = Length(entity->velocity);
        if (speed <= 0.01f)
        {
            entity->velocity = { 0, 0, 0 };
            entity->flags &= ~EntityFlag_InKnockback;
        }

        entity->position += entity->velocity * delta;
    }

    v3 newEntityPosition = entity->position + (entity->velocity * delta);
    //----------------------------------------------------------------------------

    // -----------------------------------------------------------
    // Collision detection
    v3 correction      = { 0.0f, 0.0f, 0.0f };
    v3 totalCorrection = { 0 };

    for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++)
    {
        Entity* entityPtr = EntityGet(state, entityIndex);
        if (entityPtr != entity && entityPtr->type == EntityType_Obstacle)
        {
            v3 lookAt{ sinf(entity->yaw), 0.0f, cosf(entity->yaw) };
            lookAt   = Norm(lookAt);
            v3 start = entity->position;
            v3 end   = start + (lookAt * 1.5f);

            if (AABBSegmentIntersection(EntityWorldAABB(entityPtr), start, end))
            {
                Entity* target       = entity->targetEntity;
                entity->targetEntity = entityPtr;
                EntityAttack(state, entity, &gWeaponHand, Norm(entity->velocity));
                entity->targetEntity = target;
                break;
            }
            else
            {
                AABB intersection;
                if (EntitiesIntersect(entity, entityPtr, &intersection))
                {
                    v3 penetration;
                    penetration.x = intersection.max.x - intersection.min.x;
                    penetration.y = intersection.max.y - intersection.min.y;
                    penetration.z = intersection.max.z - intersection.min.z;

                    //----------------------------------------------------------------------------
                    // Correct using the minimal penetration axis
                    if (penetration.x < penetration.z)
                    {
                        correction.x = penetration.x;
                    }
                    else
                    {
                        correction.z = penetration.z;
                    }
                    //----------------------------------------------------------------------------

                    //----------------------------------------------------------------------------
                    // Determine correction axis
                    if (newEntityPosition.x < entity->position.x)
                    {
                        correction.x = -correction.x;
                    }
                    if (newEntityPosition.z > entity->position.z)
                    {
                        correction.z = -correction.z;
                    }
                    //----------------------------------------------------------------------------

                    //----------------------------------------------------------------------------
                    // Use the greatest penetration to resolve collisions
                    if (Abs(correction.x) > Abs(totalCorrection.x))
                    {
                        totalCorrection.x = correction.x;
                    }
                    if (Abs(correction.z) > Abs(totalCorrection.z))
                    {
                        totalCorrection.z = correction.z;
                    }
                    //----------------------------------------------------------------------------
                }
            }
        }
    }
    // -----------------------------------------------------------
    newEntityPosition += totalCorrection;
    entity->position = newEntityPosition;

    if (enemyGraphNodeIndex != GRAPH_EMPTY_NODE)
    {
        GraphRemoveNode(state, enemyGraphNodeIndex);
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(GameState) <= memory->permanentStorageSize);

    GameState*  state    = (GameState*)memory->permanentStorage;
    PlatformAPI platform = memory->platform;
    OpenGL*     opengl   = &memory->opengl;
    Arena*      arena    = &state->arena;

    // ----------------------------------------------------------------------------
    // Init
    if (!state->initialized)
    {
        platform.Logf("Initializing game state...");
        state->initialized = true;

        ArenaInit(arena, (size_t)memory->permanentStorageSize - sizeof(GameState),
                  (u8*)memory->permanentStorage + sizeof(GameState));

        state->program                = PushStruct(arena, Program);
        state->camera                 = PushStruct(arena, Camera);
        state->planeBuffer            = PushStruct(arena, GeometryBuffer);
        state->characterBuffer        = PushStruct(arena, GeometryBuffer);
        state->fenceBuffer            = PushStruct(arena, GeometryBuffer);
        state->fenceDiffuseMapTexture = PushStruct(arena, Texture);
        state->whiteTexture           = PushStruct(arena, Texture);
        state->crosshairAtlas         = PushStruct(arena, Texture);
        state->glyphAtlas             = PushStruct(arena, Texture);
        state->batchBuffer            = PushStruct(arena, BatchBuffer);
        state->grid                   = PushArray(arena, GRID_CELLS, GridCell);
        memset(state->grid, 0, sizeof(GridCell) * GRID_CELLS);

        state->mode                  = GameMode_Round;
        state->roundMaxEnemy         = 1;
        state->roundCount            = 1;
        state->roundSpawnIntervalSec = 0.3;
        state->roundLastSpawnTime    = 0;
        state->roundEnemyCount       = 0;
        state->buildObstacle         = 0;
        state->buildModeDurationSec  = 10;
        state->buildModeBeginTime    = 0;

#ifdef BUILD_TYPE_DEBUG
        state->debug   = PushStruct(arena, DebugState);
        void* permMem  = PushBlock(arena, Kilobytes(64));
        void* frameMem = PushBlock(arena, Kilobytes(128));

        ArenaInit(&state->debug->arena, Kilobytes(64), permMem);
        ArenaInit(&state->debug->frameArena, Kilobytes(128), frameMem);

        DebugInit(state->debug, opengl, &platform);
#endif
        FileReadResult vertexSourceFile   = platform.FileReadEntire("../src/shaders/basic.vert");
        FileReadResult fragmentSourceFile = platform.FileReadEntire("../src/shaders/basic.frag");

        ProgramInit(opengl, state->program);
        ProgramAttachShader(opengl, state->program, (char*)vertexSourceFile.content, vertexSourceFile.contentSize,
                            GL_VERTEX_SHADER);
        ProgramAttachShader(opengl, state->program, (char*)fragmentSourceFile.content, fragmentSourceFile.contentSize,
                            GL_FRAGMENT_SHADER);
        ProgramBuild(opengl, state->program);

        platform.FileFree(vertexSourceFile.content);
        platform.FileFree(fragmentSourceFile.content);

        // Plane
        {
            GeometryBufferInit(opengl, state->planeBuffer, GL_TRIANGLES);
            GeometryBufferVBOAlloc(opengl, state->planeBuffer, planeVertexs, sizeof(planeVertexs), sizeof(Vertex),
                                   GL_STATIC_DRAW);
            GeometryBufferEBOAlloc(opengl, state->planeBuffer, planeIndices, ArrayCount(planeIndices) * sizeof(u32),
                                   sizeof(u32), GL_STATIC_DRAW);
            GeometryBufferVertexAttrib(opengl, state->planeBuffer, 0, 3, GL_FLOAT, sizeof(Vertex),
                                       offsetof(Vertex, position));
            GeometryBufferVertexAttrib(opengl, state->planeBuffer, 1, 3, GL_FLOAT, sizeof(Vertex),
                                       offsetof(Vertex, normal));
            GeometryBufferVertexAttrib(opengl, state->planeBuffer, 2, 2, GL_FLOAT, sizeof(Vertex),
                                       offsetof(Vertex, uv));
        }

        // Texture loading
        {
            FileReadResult imageReadResult = platform.FileReadEntire("../data/crosshairs.png");
            if (imageReadResult.contentSize > 0)
            {
                TextureAlloc(opengl, state->crosshairAtlas, imageReadResult.content, imageReadResult.contentSize);
                platform.FileFree(imageReadResult.content);
            }
            else
            {
                Assert(0);
            }

            u32 pixels = 0xFFFFFFFF;
            opengl->glGenTextures(1, &state->whiteTexture->id);
            opengl->glBindTexture(GL_TEXTURE_2D, state->whiteTexture->id);
            opengl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels);
        }

        // 2D batching
        {
            BatchBuffer* batch = state->batchBuffer;

            batch->maxVertexCount   = 1000;
            batch->maxIndexCount    = batch->maxVertexCount * 6;
            batch->vertexCount      = 0;
            batch->indexCount       = 0;
            batch->vertexBufferBase = PushArray(&state->arena, batch->maxVertexCount, BatchVertex);
            batch->indexBufferBase  = PushArray(&state->arena, batch->maxIndexCount, u32);
            batch->vertexBufferPtr  = batch->vertexBufferBase;
            batch->indexBufferPtr   = batch->indexBufferBase;

            Program* program = &state->batchBuffer->program;

            FileReadResult vsFile = platform.FileReadEntire("../src/shaders/batch.vert");
            FileReadResult fsFile = platform.FileReadEntire("../src/shaders/batch.frag");

            ProgramInit(opengl, program);
            ProgramAttachShader(opengl, program, (char*)vsFile.content, vsFile.contentSize, GL_VERTEX_SHADER);
            ProgramAttachShader(opengl, program, (char*)fsFile.content, fsFile.contentSize, GL_FRAGMENT_SHADER);
            ProgramBuild(opengl, program);

            platform.FileFree(vsFile.content);
            platform.FileFree(fsFile.content);

            size_t vertexSize = sizeof(BatchVertex);

            GeometryBuffer* buffer = &batch->buffer;

            GeometryBufferInit(opengl, buffer, GL_TRIANGLES);
            GeometryBufferVBOAlloc(opengl, buffer, 0, vertexSize * batch->maxVertexCount, vertexSize, GL_DYNAMIC_DRAW);
            GeometryBufferEBOAlloc(opengl, buffer, 0, sizeof(u32) * batch->maxIndexCount, sizeof(u32), GL_DYNAMIC_DRAW);
            GeometryBufferVertexAttrib(opengl, buffer, 0, 3, GL_FLOAT, vertexSize, offsetof(BatchVertex, position));
            GeometryBufferVertexAttrib(opengl, buffer, 1, 2, GL_FLOAT, vertexSize, offsetof(BatchVertex, uv));
            GeometryBufferVertexAttrib(opengl, buffer, 2, 4, GL_FLOAT, vertexSize, offsetof(BatchVertex, color));
            GeometryBufferVertexAttrib(opengl, buffer, 3, 1, GL_INT, vertexSize, offsetof(BatchVertex, textureIndex));
        }

        CameraInit(state->camera,          //
                   { 0.0f, 16.0f, 5.0f },  // Position
                   { 0.0f, -0.9f, -0.4f }, // Target
                   { 0.0f, 1.0f, 0.0f },   // Up
                   -68.0f,                 // Pitch
                   -90.0f,                 // Yaw
                   45.0f                   // Fov
        );

        state->pistolShot      = platform.AudioClipLoad("../data/pistol.wav", AudioClipType_Sfx);
        state->backgroundMusic = platform.AudioClipLoad("../data/background.wav", AudioClipType_Music);

        platform.AudioSetVolume(-35.0f, AudioClipType_Music);
        platform.AudioSetVolume(-3.0f, AudioClipType_Sfx);
        // platform.AudioClipPlay(state->backgroundMusic, AudioClipPlayFlag_Loop);

        // Font loading
        {
            FileReadResult fontFile = platform.FileReadEntire("c:\\windows\\fonts\\calibri.ttf");
            if (fontFile.contentSize > 0)
            {
                stbtt_fontinfo fontInfo   = { 0 };
                u8*            fontBuffer = (u8*)fontFile.content;

                if (stbtt_InitFont(&fontInfo, fontBuffer, 0))
                {
                    int fontAtlasWidth  = 1024;
                    int fontAtlasHeight = 1024;
                    f32 fontSize        = 64.0f;
                    // TODO: (Temporal arenas) Free bitmap after allocating texture
                    u8* bitmapFontBuffer = PushArray(arena, fontAtlasWidth * fontAtlasHeight, u8);

                    stbtt_pack_context packCtx;
                    stbtt_packedchar   packedChars[TTF_GLYPH_COUNT];

                    stbtt_PackBegin(&packCtx, bitmapFontBuffer, fontAtlasWidth, fontAtlasHeight, 0, 1, 0);
                    stbtt_PackFontRange(&packCtx, fontBuffer, 0, fontSize, TTF_FIRST_GLYPH_OFFSET, TTF_GLYPH_COUNT,
                                        packedChars);
                    stbtt_PackEnd(&packCtx);

                    for (u32 charIndex = 0; charIndex < TTF_GLYPH_COUNT; charIndex++)
                    {
                        float x, y;

                        stbtt_aligned_quad alignedQuad;
                        stbtt_GetPackedQuad(packedChars, fontAtlasWidth, fontAtlasHeight, (int)charIndex, &x, &y,
                                            &alignedQuad, 0);

                        TTFGlyph* ttfChar = &state->ttfChars[charIndex];
                        ttfChar->x0       = packedChars[charIndex].x0;
                        ttfChar->y0       = packedChars[charIndex].y0;
                        ttfChar->x1       = packedChars[charIndex].x1;
                        ttfChar->y1       = packedChars[charIndex].y1;
                        ttfChar->xoff     = packedChars[charIndex].xoff;
                        ttfChar->yoff     = packedChars[charIndex].yoff;
                        ttfChar->xadvance = packedChars[charIndex].xadvance;
                        ttfChar->s0       = alignedQuad.s0;
                        ttfChar->t0       = alignedQuad.t0;
                        ttfChar->s1       = alignedQuad.s1;
                        ttfChar->t1       = alignedQuad.t1;
                    }

                    platform.FileFree(fontFile.content);

                    opengl->glGenTextures(1, &state->glyphAtlas->id);
                    opengl->glBindTexture(GL_TEXTURE_2D, state->glyphAtlas->id);
                    opengl->glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, (GLsizei)fontAtlasWidth, (GLsizei)fontAtlasHeight, 0,
                                         GL_RED, GL_UNSIGNED_BYTE, (void*)bitmapFontBuffer);
                    GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
                    opengl->glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
                    opengl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    opengl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    state->glyphAtlas->width  = (u32)fontAtlasWidth;
                    state->glyphAtlas->height = (u32)fontAtlasHeight;
                }
                else
                {
                    platform.Logf("Unable to init .ttf font");
                    Assert(0);
                }
            }
            else
            {
                platform.Logf("Unable to load font");
                Assert(0);
            }
        }

        // Obj test
        {
            // Fence
            FileReadResult fenceFile = platform.FileReadEntire("../data/fence_2.obj");
            if (fenceFile.contentSize > 0)
            {
                Obj obj = ObjReadData(fenceFile.content, fenceFile.contentSize, platform.FileReadEntire,
                                      platform.FileFree, platform.Logf, arena);
                platform.FileFree(fenceFile.content);

                // TODO: Fix Y-axis
                state->fenceAABB = PushStruct(arena, AABB);

                // Simetric hack
                state->fenceAABB->max = obj.aabb.max;
                state->fenceAABB->min = -obj.aabb.max;

                char fenceDiffuseMapFilepath[256];
                sprintf(fenceDiffuseMapFilepath, "%s", "../data/");
                strcat(fenceDiffuseMapFilepath, obj.materials[0].diffuseMap);

                ObjInitGeometryBuffer(&obj, arena, opengl, state->fenceBuffer);

                FileReadResult fenceDiffuseMapReadResult = platform.FileReadEntire(fenceDiffuseMapFilepath);
                if (fenceDiffuseMapReadResult.contentSize > 0)
                {
                    TextureAlloc(opengl, state->fenceDiffuseMapTexture, fenceDiffuseMapReadResult.content,
                                 fenceDiffuseMapReadResult.contentSize);
                    platform.FileFree(fenceDiffuseMapReadResult.content);
                }
                else
                {
                    Assert(0);
                }
            }
            else
            {
                Assert(0);
            }

            // Stickman
            FileReadResult characterFile = platform.FileReadEntire("../data/character.obj");
            if (characterFile.contentSize > 0)
            {
                Obj characterObj = ObjReadData(characterFile.content, characterFile.contentSize,
                                               platform.FileReadEntire, platform.FileFree, platform.Logf, arena);
                platform.FileFree(characterFile.content);
                state->characterAABB  = PushStruct(arena, AABB);
                *state->characterAABB = characterObj.aabb;

                ObjInitGeometryBuffer(&characterObj, arena, opengl, state->characterBuffer);
            }
            else
            {
                Assert(0);
            }
        }

        Entity* player   = EntityNew(state, EntityType_Player);
        player->position = { 0.0f, 0.0f, 0.0f };
        player->size     = { 1.0f, 1.0f, 1.0f };
        player->aabb     = *state->characterAABB;

        // Entity* enemy0       = EntityNew(state, EntityType_Enemy);
        // enemy0->position     = { -2.0f, 0.0f, -8.0f };
        // enemy0->size         = { 1.0f, 1.0f, 1.0f };
        // enemy0->aabb         = *state->characterAABB;
        // enemy0->targetEntity = player;

        // Entity* enemy1       = EntityNew(state, EntityType_Enemy);
        // enemy1->position     = { -2.0f, 0.0f, -10.0f };
        // enemy1->size         = { 1.0f, 1.0f, 1.0f };
        // enemy1->aabb         = characterAABB;
        // enemy1->targetEntity = player;

        Entity* fence0   = EntityNew(state, EntityType_Obstacle);
        fence0->position = { 0.0f, 0.0f, -2.0f };
        fence0->size     = { 1.0f, 1.0f, 1.0f };
        fence0->aabb     = *state->fenceAABB;
        ObstaclePlace(state->grid, fence0);

#if 0
        for (u32 i = 0; i < 20; i++)
        {
            {
                Entity* enemy1       = EntityNew(state, EntityType_Enemy);
                enemy1->position     = { (f32)i, 0.0f, -12.0f };
                enemy1->size         = { 1.0f, 1.0f, 1.0f };
                enemy1->aabb         = characterAABB;
                enemy1->targetEntity = player;
            }
            {
                Entity* enemy1       = EntityNew(state, EntityType_Enemy);
                enemy1->position     = { (f32)i, 0.0f, 12.0f };
                enemy1->size         = { 1.0f, 1.0f, 1.0f };
                enemy1->aabb         = characterAABB;
                enemy1->targetEntity = player;
            }
        }
#endif

        state->graph = PushStruct(arena, Graph);
        GraphInit(state);
    }

    // ----------------------------------------------------------------------------

    // ----------------------------------------------------------------------------
    // Update
    v2u     windowDim  = platform.WindowGetDimension();
    Entity* player     = EntityGet(state, 0);
    Camera* camera     = state->camera;
    mat4x4  projection = Perspective(Radians(45.0f), (f32)windowDim.w / (f32)windowDim.h, 0.1f, 100.0f);
    mat4x4  view       = CameraView(camera);

    v4 playerColor = white;

    local_persist cell_index debugSelectedCellIndex = 0xFFFFFFFF;

    // TODO: Assign controller to player
    // for (u32 controllerIndex = 0; controllerIndex < ArrayCount(input->controllers); controllerIndex++)
    //{
    //    GameInputController* controller = GetController(input, controllerIndex);
    //}

    GameInputController* controller = GetController(input, 0);

#if 1
    v3 cameraOffset{ 0.0f, 16.0f, 5.0f };
    CameraSetPitch(camera, -68.0f);
#else
    v3 cameraOffset{ 0.0f, 4.0f, 8.0f };
    CameraSetPitch(camera, -26.0f);
#endif

#if BUILD_TYPE_DEBUG
    if (ButtonIsPressed(input->debug.f1))
    {
        // state->buildMode = !state->buildMode;
        if (state->mode != GameMode_Build)
        {
            state->mode                 = GameMode_Build;
            state->buildModeDurationSec = 2000;
        }
        else
        {
            state->mode                 = GameMode_Round;
            state->buildModeDurationSec = 10;
        }
    }
#endif

    if (player->health <= 0)
    {
        state->mode = GameMode_GameOver;
    }

    Mouse* mouse = &input->mouse;

    switch (state->mode)
    {
    case GameMode_Pause:
    {
        break;
    }
    case GameMode_Build:
    {
#if BUILD_TYPE_DEBUG
        // Destroy entity
        {
            if (ButtonIsPressed(mouse->middle))
            {
                v3         crosshairPoint = WorldMousePicking(camera, projection, windowDim, mouse->pos);
                cell_index crosshairCell  = WorldPositionToGridCell(crosshairPoint);

                for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++)
                {
                    Entity* entity = EntityGet(state, entityIndex);
                    if (entity->type != EntityType_Obstacle)
                    {
                        continue;
                    }

                    EntityCellCorners cellCorners = EntityGetCellCorners(entity);
                    u32               beginRow    = CELL_ROW(cellCorners.bottomRight);
                    u32               endRow      = CELL_ROW(cellCorners.topRight);
                    u32               beginCol    = CELL_COL(cellCorners.bottomLeft);
                    u32               endCol      = CELL_COL(cellCorners.bottomRight);

                    for (u32 row = beginRow; row <= endRow; row++)
                    {
                        for (u32 col = beginCol; col <= endCol; col++)
                        {
                            if (crosshairCell == CELL_INDEX(row, col))
                            {
                                // platform.Logf("Found entity");
                                EntityRemove(state, entity);
                            }
                        }
                    }
                }
            }
        }
#endif

        // Start timer
        if (state->buildModeBeginTime == 0)
        {
            state->buildModeBeginTime = time(0);
        }
        else
        {
            int elapsedSeconds = (int)difftime(time(0), state->buildModeBeginTime);

            if (elapsedSeconds >= (int)state->buildModeDurationSec)
            {
                // Set round mode stuff
                state->mode = GameMode_Round;
                // state->roundMaxEnemy *= 2;
                state->roundCount++;
                // state->roundSpawnIntervalSec = 0.3;
                state->roundLastSpawnTime = 0;
                state->roundEnemyCount    = 0;

                // Clean up build mode stuff
                state->buildModeBeginTime = 0;
                state->buildObstacle      = 0;

                break;
            }
        }

        // Spawn obstacle
        if (!state->buildObstacle && ButtonIsPressed(mouse->left))
        {
            // TODO: Move this logic to `ObstacleSpawn` function
            v3 position = WorldMousePicking(camera, projection, windowDim, mouse->pos);

            state->buildObstacle           = EntityNew(state, EntityType_Obstacle);
            state->buildObstacle->position = position;
            state->buildObstacle->size     = { 1.0f, 1.0f, 1.0f };
            state->buildObstacle->flags |= EntityFlag_Positioning;
            state->buildObstacle->aabb = *state->fenceAABB;
        }
        else if (state->buildObstacle)
        {
            // Place object if valid position
            if (ButtonIsPressed(mouse->left))
            {
                if (ObstacleIsValidPosition(state->entities, state->entityCount, state->grid, state->buildObstacle))
                {
                    ObstaclePlace(state->grid, state->buildObstacle);
                    GraphAddEntity(state, state->buildObstacle);
                    state->buildObstacle->flags &= ~(EntityFlag_Positioning);
                    state->buildObstacle = 0;
                    EntitiesRemoveFlag(state, EntityFlag_Snapping);
                }
                else
                {
                    platform.Logf("Invalid obstacle position");
                }
            }
            // Cancel placing
            else if (ButtonIsPressed(mouse->right))
            {
                EntityRemove(state, state->buildObstacle);
                state->buildObstacle = 0;
            }
            // Drag, rotate and snap obstacle
            else
            {
                ObstacleDrag(state->buildObstacle, camera, projection, windowDim, mouse->pos);

                if (ButtonIsPressed(input->keyboard.moveLeft))
                {
                    ObstacleRotate(state->buildObstacle, true);
                }
                else if (ButtonIsPressed(input->keyboard.moveRight))
                {
                    ObstacleRotate(state->buildObstacle, false);
                }

                if (ObstacleIsValidPosition(state->entities, state->entityCount, state->grid, state->buildObstacle))
                {
                    state->buildObstacle->flags &= ~EntityFlag_InvalidPosition;
                    SnapCandidate snapCandidate = ObstacleFindNearestSnap(state->grid, state->buildObstacle);
                    if (snapCandidate.entity)
                    {
                        ObstaclesSnap(state->grid, state->buildObstacle, &snapCandidate);
                        state->buildObstacle->flags |= EntityFlag_Snapping;
                    }
                    else
                    {
                        state->buildObstacle->flags &= ~EntityFlag_Snapping;
                    }
                }
                else
                {
                    state->buildObstacle->flags |= EntityFlag_InvalidPosition;
                }
            }
        }
        break;
    }
    case GameMode_GameOver:
    {
        if (ButtonIsPressed(mouse->left))
        {
            state->mode = GameMode_Round;
        }
        break;
    }
    case GameMode_Round:
    {
        PlayerUpdate(state, player, delta, &platform, controller, mouse, cameraOffset);
        u32 playerGraphNodeIndex = GraphAddNode(state, WorldPositionToGridCell(player->position), player->type);

        for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++)
        {
            Entity* entity = EntityGet(state, entityIndex);
            if (entity->type == EntityType_Enemy)
            {
                EnemyUpdate(state, entity, delta);
            }
        }

        // Remove playerNode
        if (playerGraphNodeIndex != GRAPH_EMPTY_NODE)
        {
            GraphRemoveNode(state, playerGraphNodeIndex);
        }

        if (state->roundLastSpawnTime == 0)
        {
            Assert(state->roundEnemyCount == 0);
            state->roundLastSpawnTime = time(0);
            state->roundEnemyCount++;

            Entity* enemy       = EntityNew(state, EntityType_Enemy);
            enemy->position     = { -2.0f, 0.0f, -8.0f };
            enemy->size         = { 1.0f, 1.0f, 1.0f };
            enemy->aabb         = *state->characterAABB;
            enemy->targetEntity = player;
        }
        else if (state->roundEnemyCount < state->roundMaxEnemy)
        {
            time_t now = time(0);

            f64 seconds = difftime(now, state->roundLastSpawnTime);
            if (seconds > state->roundSpawnIntervalSec)
            {
                state->roundLastSpawnTime = now;
                state->roundEnemyCount++;

                Entity* enemy       = EntityNew(state, EntityType_Enemy);
                enemy->position     = { -2.0f, 0.0f, -8.0f };
                enemy->size         = { 1.0f, 1.0f, 1.0f };
                enemy->aabb         = *state->characterAABB;
                enemy->targetEntity = player;
            }
        }

        if (state->roundEnemyCount == state->roundMaxEnemy)
        {
            u32 enemyCount = 0;
            for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++)
            {
                Entity* entity = EntityGet(state, entityIndex);
                if (entity->type == EntityType_Enemy)
                {
                    enemyCount++;
                }
            }
            if (enemyCount == 0)
            {
                state->mode = GameMode_Build;
            }
        }

        break;
    }
    }

    if (ButtonIsPressed(mouse->middle))
    {
        v3 mousePoint          = WorldMousePicking(camera, projection, windowDim, mouse->pos);
        debugSelectedCellIndex = WorldPositionToGridCell(mousePoint);
    }
    // ----------------------------------------------------------------------------

    // ----------------------------------------------------------------------------
    // Draw
    RenderCommandQueue* commandQueue = RendererFrameBegin(opengl);

    FramebufferClear* framebufferClear = PushRenderCommand(commandQueue, FramebufferClear);
    // framebufferClear->color.r          = 0.18f;
    // framebufferClear->color.g          = 0.31f;
    // framebufferClear->color.b          = 0.52f;
    framebufferClear->color.r = 0.0f;
    framebufferClear->color.g = 0.0f;
    framebufferClear->color.b = 0.0f;

    BatchBuffer* batch     = state->batchBuffer;
    batch->vertexBufferPtr = batch->vertexBufferBase;
    batch->indexBufferPtr  = batch->indexBufferBase;
    batch->vertexCount     = 0;
    batch->indexCount      = 0;

    // TODO: Find a better way to handle textures
    int textureArray[] = { 0, 1, 2 };
    opengl->glActiveTexture(GL_TEXTURE0);
    opengl->glBindTexture(GL_TEXTURE_2D, state->whiteTexture->id);
    opengl->glActiveTexture(GL_TEXTURE1);
    opengl->glBindTexture(GL_TEXTURE_2D, state->crosshairAtlas->id);
    opengl->glActiveTexture(GL_TEXTURE2);
    opengl->glBindTexture(GL_TEXTURE_2D, state->glyphAtlas->id);
    opengl->glActiveTexture(GL_TEXTURE3);
    opengl->glBindTexture(GL_TEXTURE_2D, state->fenceDiffuseMapTexture->id);

    // TODO: Do this kind of operations with render commands
    opengl->glDisable(GL_DEPTH_TEST);
    opengl->glEnable(GL_BLEND);
    opengl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Batch
    {
        mat4x4 view2D = Orthographic(0, (f32)windowDim.w, (f32)windowDim.h, 0);

        // Cursors: 828, 965
        BatchTextureSubRect(batch, { (f32)mouse->pos.x, (f32)mouse->pos.y }, { 32.0f, 32.0f }, state->crosshairAtlas,
                            { 965.0f, 0.0f }, { 128.0f, 128.0f });

        char coordBuffer[64];
#if 1
        sprintf(coordBuffer, "%.2f %.2f", player->position.x, player->position.z);
#else
        sprintf(coordBuffer, "%d %d", (int)player->position.x, (int)player->position.z);
#endif
        BatchText(state, batch, coordBuffer, { 0.0f, 50.0f }, white);

        // Debug grid
        {
            if (debugSelectedCellIndex != -1)
            {
                char debugSelectedCellBuffer[64];
                sprintf(debugSelectedCellBuffer, "%d (%d %d)", debugSelectedCellIndex, CELL_ROW(debugSelectedCellIndex),
                        CELL_COL(debugSelectedCellIndex));
                BatchText(state, batch, debugSelectedCellBuffer, { 0.0f, 100.0f }, magenta);
            }
        }

        if (state->mode == GameMode_Build)
        {
            BatchText(state, batch, "Build", { 0.0f, 150.0f }, green);

            char timerBuf[100];
            int  elapsedSeconds   = (int)difftime(time(0), state->buildModeBeginTime);
            int  remainingSeconds = (int)state->buildModeDurationSec - elapsedSeconds;
            sprintf(timerBuf, "%d", remainingSeconds);
            BatchText(state, batch, timerBuf, { windowDim.w - 150.0f, 50.0f }, red);
        }
        else if (state->mode == GameMode_GameOver)
        {
            BatchText(state, batch, "GameOver", { 0.0f, 150.0f }, green);
        }
        else if (state->mode == GameMode_Round)
        {
            char buff[100];
            sprintf(buff, "Round %d", state->roundCount);
            BatchText(state, batch, buff, { 0.0f, 150.0f }, green);
        }

        // Batch
        if (batch->vertexCount > 0)
        {
            // TODO: This is ugly, rethink DrawBuffer render command. (It might take index/primitive count)
            batch->buffer.indexCount  = batch->indexCount;
            batch->buffer.vertexCount = batch->vertexCount;

            GeometryBufferVBOSubdata(opengl, &batch->buffer, batch->vertexBufferBase,
                                     sizeof(BatchVertex) * batch->vertexCount);
            GeometryBufferEBOSubdata(opengl, &batch->buffer, batch->indexBufferBase, sizeof(u32) * batch->indexCount);

            PushRenderProgramUse(commandQueue, batch->program.id);
            PushRenderUploadUniformMat4x4(commandQueue, batch->program.id, "viewProj", view2D);
            PushRenderUploadUniformIntArray(commandQueue, batch->program.id, "textureArray", textureArray,
                                            ArrayCount(textureArray));

            PushRenderDrawBuffer(commandQueue, &batch->buffer);

            batch->vertexBufferPtr = batch->vertexBufferBase;
            batch->indexBufferPtr  = batch->indexBufferBase;
            batch->vertexCount     = 0;
            batch->indexCount      = 0;
        }
    }

    // 3D
    {
        mat4x4 viewProj = projection * view;

        PushRenderProgramUse(commandQueue, state->program->id);
        PushRenderUploadUniformInt(commandQueue, state->program->id, "hasDiffuse", 0);

        // Floor
        {
            mat4x4 translate = Translate(Identity(), { 0.0f, 0.0f, 0.0f });
            mat4x4 scale     = Scale(Identity(), 20.0f);
            // mat4x4 scale = Scale(Identity(), { GRID_ROWS * 0.5f, 0.0f, GRID_COLS * 0.5f });
            mat4x4 model = translate * scale;

            PushRenderUploadUniformMat4x4(commandQueue, state->program->id, "mvp", viewProj * model);
            PushRenderUploadUniformVec4(commandQueue, state->program->id, "color", { 1.0f, 1.0f, 1.0f, 0.5f });
            PushRenderDrawBuffer(commandQueue, state->planeBuffer);
        }

        // Entities
        {
            for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++)
            {
                Entity* entity = &state->entities[entityIndex];

                mat4x4 translate = Translate(Identity(), entity->position);
                mat4x4 rotate    = Rotate(Identity(), entity->yaw, { 0.0f, 1.0f, 0.0f });
                mat4x4 scale     = Scale(Identity(), entity->size);

                mat4x4 model = translate * rotate * scale;

                if (entity->type != EntityType_Obstacle)
                {
                    PushRenderUploadUniformMat4x4(commandQueue, state->program->id, "mvp", viewProj * model);
                    PushRenderUploadUniformVec4(commandQueue, state->program->id, "color",
                                                entityIndex == 0 ? playerColor : green);
                    PushRenderDrawBuffer(commandQueue, state->characterBuffer);
                }
                else
                {
                    v4 tintColor = white;

                    if (entity->flags & (EntityFlag_InvalidPosition))
                    {
                        tintColor = red;
                    }
                    else if (entity->flags & (EntityFlag_Positioning | EntityFlag_Snapping))
                    {
                        tintColor = green;
                    }

                    PushRenderUploadUniformInt(commandQueue, state->program->id, "hasDiffuse", 1);
                    PushRenderUploadUniformInt(commandQueue, state->program->id, "diffuseMap", 3);
                    PushRenderUploadUniformMat4x4(commandQueue, state->program->id, "mvp", viewProj * model);
                    PushRenderUploadUniformVec4(commandQueue, state->program->id, "color", tintColor);
                    PushRenderDrawBuffer(commandQueue, state->fenceBuffer);
                }
            }
        }
    }

    RendererFrameEnd(opengl);

#ifdef BUILD_TYPE_DEBUG
    DebugFrameBegin(state->debug, opengl, projection * view);
    {
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
                DebugDrawLine(state->debug, opengl, { (f32)col, y, (f32)minRow }, { (f32)col, y, (f32)maxRow }, black);
            }
            // Horizontal lines
            for (s32 row = minRow; row <= maxRow; row++)
            {
                DebugDrawLine(state->debug, opengl, { (f32)minCol, y, (f32)row }, { (f32)maxCol, y, (f32)row }, black);
            }

            // Debug selected cell
            if (debugSelectedCellIndex != -1)
            {
                DebugDrawGridCell(state->debug, opengl, debugSelectedCellIndex, magenta);
            }
        }

        // Shooting
        {
            if (ButtonIsDown(mouse->left))
            {
                v3 end = WorldMousePicking(camera, projection, windowDim, mouse->pos);

                DebugDrawLine(state->debug, opengl, player->position, end, green);
            }
        }

        // Debug graph
        {
            Graph* graph = state->graph;

            Entity* enemy = EntityGet(state, 1);

            cell_index enemyCell  = WorldPositionToGridCell(enemy->position);
            cell_index playerCell = WorldPositionToGridCell(player->position);

            u32 playerGraphNodeIndex = GraphAddNode(state, playerCell, player->type);
            u32 enemyGraphNodeIndex  = GraphAddNode(state, enemyCell, enemy->type);

            for (u32 nodeIndex = 0; nodeIndex < graph->nodes.size(); nodeIndex++)
            {
                cell_index nodeCell = graph->nodes[nodeIndex];

                if (nodeCell != GRAPH_EMPTY_NODE)
                {
                    DebugDrawGridCell(state->debug, opengl, nodeCell, green);

                    v3 nodePos = WorldGridCellToPosition(nodeCell);

                    for (auto edgeIndex : graph->edges[nodeIndex])
                    {
                        cell_index dstCell    = graph->nodes[edgeIndex];
                        v3         dstNodePos = WorldGridCellToPosition(dstCell);

                        DebugDrawLine(state->debug, opengl, nodePos, dstNodePos, magenta);
                    }
                }
            }

            if (playerGraphNodeIndex != GRAPH_EMPTY_NODE)
            {
                GraphRemoveNode(state, playerGraphNodeIndex);
            }
            if (enemyGraphNodeIndex != GRAPH_EMPTY_NODE)
            {
                GraphRemoveNode(state, enemyGraphNodeIndex);
            }
        }

        // Debug entities
        for (u32 entityIndex = 0; entityIndex < state->entityCount; entityIndex++)
        {
            Entity* entity          = &state->entities[entityIndex];
            AABB    entityWorldAABB = AABBToWorld(entity->aabb, entity->position);

            // Entity rotation
            {
                v3 lookAt{ sinf(entity->yaw), 0.0f, cosf(entity->yaw) };
                v3 p0{ entity->position.x, entity->aabb.max.y, entity->position.z };
                v3 p1 = p0 + (lookAt * 1.5f);

                DebugDrawLine(state->debug, opengl, p0, p1, blue);
            }

            // Entity AABB
            DebugDrawAABB(state->debug, opengl, entity->position, entity->yaw, entity->aabb, red);

            // Entity Y orientation
            if (entity->type == EntityType_Obstacle)
            {
                // Snap points
                if (state->mode == GameMode_Build)
                {
                    EntityWorldCorners worldCorners = EntityGetWorldCorners(entity);
                    for (u32 cornerIndex = 0; cornerIndex < ArrayCount(worldCorners.arr); cornerIndex++)
                    {
                        v3 snapPoint = worldCorners.arr[cornerIndex];
                        snapPoint.y  = 0.0f;

                        DebugDrawPlane(state->debug, opengl, snapPoint, { CELL_HALF, 0.0f, CELL_HALF }, yellow);
                    }
                }

                // Occupied cells
#if 0
                {
                    EntityCellCorners cellCorners = EntityGetCellCorners(entity);

                    u32 beginRow = CELL_ROW(cellCorners.bottomRight);
                    u32 endRow   = CELL_ROW(cellCorners.topRight);
                    u32 beginCol = CELL_COL(cellCorners.bottomLeft);
                    u32 endCol   = CELL_COL(cellCorners.bottomRight);

                    for (u32 row = beginRow; row <= endRow; row++)
                    {
                        for (u32 col = beginCol; col <= endCol; col++)
                        {
                            DebugDrawGridCell(state->debug, opengl, CELL_INDEX(row, col), orange);
                        }
                    }
                }
#endif
            }
            else if (entity->type == EntityType_Enemy)
            {
                // Enemy hitbox
                DebugDrawPlane(state->debug, opengl, entity->position, { enemyHitRadius, 0, enemyHitRadius }, yellow);

                // Path finding
                {
                    u32 playerGraphNodeIndex =
                        GraphAddNode(state, WorldPositionToGridCell(player->position), player->type);
                    u32 enemyGraphNodeIndex =
                        GraphAddNode(state, WorldPositionToGridCell(entity->position), entity->type);

                    cell_index startCell = WorldPositionToGridCell(entity->position);
                    cell_index dstCell   = WorldPositionToGridCell(entity->targetEntity->position);

                    std::vector<cell_index> path = GraphFindBestPath(state->graph, startCell, dstCell);
                    if (!path.empty())
                    {
                        for (u32 i = 0; i < path.size() - 1; i++)
                        {
                            v3 start = WorldGridCellToPosition(path[i]);
                            v3 end   = WorldGridCellToPosition(path[i + 1]);

                            start.y = 0.01f;
                            end.y   = 0.01f;

                            DebugDrawLine(state->debug, opengl, start, end, yellow);
                        }
                    }

                    if (playerGraphNodeIndex != GRAPH_EMPTY_NODE)
                    {
                        GraphRemoveNode(state, playerGraphNodeIndex);
                    }
                    if (enemyGraphNodeIndex != GRAPH_EMPTY_NODE)
                    {
                        GraphRemoveNode(state, enemyGraphNodeIndex);
                    }
                }
            }
        }
    }
    DebugFrameEnd(state->debug, opengl);
#endif
    return 0;
}