#include "survivor.h"

#include "survivor_renderer_opengl.cpp"
#include "survivor_debug_geometry.cpp"
#include "survivor_debug.cpp"
#include "survivor_obj.cpp"
#include "survivor_gltf.cpp"
#include "survivor_entity.cpp"
#include "survivor_world.cpp"
#include "survivor_build.cpp"
#include "survivor_assets.cpp"

// TODO
/*
- (Audio) Make easy to tweak volumes (ignore db conversion)
- (Misc): Temporal arenas
- (Game): gamepad controller
*/

internal void EntityAttack(EntityManager* manager, Entity* entity, World* world, Weapon* weapon, glm::vec3 dir)
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
            EntityDestroy(manager, entity, world);
        }
    }
    else
    {
        Ray shot;
        shot.origin = entity->position;
        shot.dir    = dir;

        for (u32 entityIndex = 0; entityIndex < manager->entityCount; entityIndex++)
        {
            Entity* targetEntity = EntityGet(manager, entityIndex);
            if (targetEntity->type == EntityType_Enemy)
            {
                AABB entityWorldAABB = AABBToWorld(targetEntity->aabb, targetEntity->position);

                if (AABBRayIntersection(entityWorldAABB, shot))
                {
                    glm::vec3 shotDir = SafeNorm(targetEntity->position - entity->position);

                    targetEntity->flags |= EntityFlag_InKnockback;
                    targetEntity->velocity += shotDir * weapon->knockbackforce;
                    targetEntity->health -= weapon->damage;

                    // TODO: Move to update ???
                    if (targetEntity->health <= 0)
                    {
                        EntityDestroy(manager, targetEntity, world);
                    }

                    // Note: break stops the projectile trajectory, this way projectile can only impact once.
                    // TODO: Decide if allow penetration
                    break;
                }
            }
        }
    }
}

internal void PlayerUpdate(GameState* state, Entity* player, f32 delta, PlatformAPI* platform,
                           GameInputController* controller, Mouse* mouse, glm::vec3 cameraOffset)
{
    Assert(player->type == EntityType_Player);

    glm::uvec2 windowDim  = platform->WindowGetDimension();
    Camera*    camera     = state->camera;
    glm::mat4  projection = glm::perspective(Radians(45.0f), (f32)windowDim.x / (f32)windowDim.y, 0.1f, 100.0f);
    glm::mat4  view       = CameraView(camera);

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
            glm::vec3 playerDirection{ 0.0f, 0.0f, 0.0f };
            glm::vec3 crosshairPoint = WorldMousePicking(camera, projection, windowDim, mouse->pos);

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
            playerDirection = SafeNorm(playerDirection);

            if (ButtonIsPressed(mouse->left))
            {
                glm::vec3 dir = SafeNorm(crosshairPoint - player->position);
                EntityAttack(state->entityManager, player, state->world, &gWeaponPistol, dir);
            }

            // Player rotation
            {
                glm::vec3 dir       = SafeNorm(crosshairPoint - player->position);
                f32       targetYaw = -atan2f(dir.x, -dir.z);
                f32       deltaYaw  = targetYaw - player->yaw;
                deltaYaw            = fmodf(deltaYaw + Pi, 2.0f * Pi) - Pi; // Wrap to [-Pi, Pi]
                player->yaw += deltaYaw * rotationSpeed;
            }

            // Deceleration
            f32 playerSpeed = glm::length(player->velocity);
            if (playerSpeed > 0.0f)
            {
                f32 decelerationStep = frictionForce * delta;

                if (playerSpeed <= decelerationStep)
                {
                    player->velocity = glm::vec3{ 0.0f, 0.0f, 0.0f };
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
                glm::vec3 acceleration = playerDirection * moveAcceleration;
                player->velocity += acceleration * delta;
                if (glm::length(player->velocity) > maxSpeed)
                {
                    player->velocity = SafeNorm(player->velocity) * maxSpeed;
                }
            }

            glm::vec3 newPlayerPosition = player->position + (player->velocity * delta);
            glm::vec3 correction{ 0.0f, 0.0f, 0.0f };
            glm::vec3 totalCorrection{ 0 };

            AABB playerWorldAABB = AABBToWorld(player->aabb, player->position);

            // Collision detection
            for (u32 entityIndex = 1; entityIndex < state->entityManager->entityCount; entityIndex++)
            {
                Entity* entity = EntityGet(state->entityManager, entityIndex);

                AABB intersection;
                if (EntitiesIntersect(player, entity, &intersection))
                {
                    if (player->flags & EntityFlag_InKnockback)
                    {
                        player->flags &= ~EntityFlag_InKnockback;
                    }

                    glm::vec3 penetration;
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

internal void EnemyUpdate(EntityManager* manager, World* world, Entity* entity, f32 delta)
{
    Assert(entity->type == EntityType_Enemy);
    Assert(entity->targetEntity);

    // Attack
    {
        glm::vec3 dir = SafeNorm(entity->velocity);
        glm::vec2 hitRectMinCorner{ entity->position.x - enemyHitRadius, entity->position.z - enemyHitRadius };
        glm::vec2 hitRectMaxCorner{ entity->position.x + enemyHitRadius, entity->position.z + enemyHitRadius };

        glm::vec2 targetMinCorner;
        glm::vec2 targetMaxCorner;
        targetMinCorner.x = entity->targetEntity->position.x - (entity->size.x * 0.5f);
        targetMinCorner.y = entity->targetEntity->position.z - (entity->size.z * 0.5f);
        targetMaxCorner.x = entity->targetEntity->position.x + (entity->size.x * 0.5f);
        targetMaxCorner.y = entity->targetEntity->position.z + (entity->size.z * 0.5f);

        b32 overlapsX = hitRectMinCorner.x <= targetMaxCorner.x && hitRectMaxCorner.x >= targetMinCorner.x;
        b32 overlapsZ = hitRectMinCorner.y <= targetMaxCorner.y && hitRectMaxCorner.y >= targetMinCorner.y;
        if (overlapsX && overlapsZ)
        {
            EntityAttack(manager, entity, world, &gWeaponHand, dir);
        }
    }

    //----------------------------------------------------------------------------
    // Path finding
    cell_index              enemyCellIndex  = WorldPositionToGridCell(entity->position);
    cell_index              playerCellIndex = WorldPositionToGridCell(entity->targetEntity->position);
    std::vector<cell_index> path            = WorldFindBestPath(world, manager, enemyCellIndex, playerCellIndex);
    glm::vec3               entityDir{ 0.0f, 0.0f, 0.0f };

    if (!path.empty())
    {
        glm::vec3 targetPosition = WorldGridCellToPosition(path[path.size() - 2]);
        entityDir                = SafeNorm(targetPosition - entity->position);
        entityDir.y              = 0.0f;
        // entity->yaw       = (f32)atan2(entityDir.x, entityDir.z);
    }
    // TODO: Break obstacles
    else
    {
        // entityDir   = SafeNormalize(entity->targetEntity->position - entity->position);
        // entityDir.y = 0.0f;
    }
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // Entity acceleration
    if (!(entity->flags & EntityFlag_InKnockback))
    {
        glm::vec3 acceleration = entityDir * enemyAcceleration;
        entity->velocity += acceleration * delta;
        // TODO: Use constant speed for enemies???
        if (glm::length(entity->velocity) > enemyMaxSpeed)
        {
            entity->velocity = SafeNorm(entity->velocity) * enemyMaxSpeed;
        }
    }
    else
    {
        // Friction force
        entity->velocity *= 0.70f;

        f32 speed = glm::length(entity->velocity);
        if (speed <= 0.01f)
        {
            entity->velocity = { 0, 0, 0 };
            entity->flags &= ~EntityFlag_InKnockback;
        }

        entity->position += entity->velocity * delta;
    }

    glm::vec3 newEntityPosition = entity->position + (entity->velocity * delta);
    //----------------------------------------------------------------------------

    // -----------------------------------------------------------
    // Collision detection
    glm::vec3 correction{ 0.0f, 0.0f, 0.0f };
    glm::vec3 totalCorrection{ 0 };

    for (u32 entityIndex = 0; entityIndex < manager->entityCount; entityIndex++)
    {
        Entity* entityPtr = EntityGet(manager, entityIndex);
        if (entityPtr != entity && entityPtr->type == EntityType_Obstacle)
        {
            glm::vec3 lookAt{ sinf(entity->yaw), 0.0f, cosf(entity->yaw) };
            lookAt          = SafeNorm(lookAt);
            glm::vec3 start = entity->position;
            glm::vec3 end   = start + (lookAt * 1.5f);

            // TODO: Break obstacles
            // if (AABBSegmentIntersection(EntityWorldAABB(entityPtr), start, end))
            //{
            //     Entity* target       = entity->targetEntity;
            //     entity->targetEntity = entityPtr;
            //     EntityAttack(manager, entity, world, &gWeaponHand, SafeNormalize(entity->velocity));
            //     entity->targetEntity = target;
            //     break;
            // }
            // else
            {
                AABB intersection;
                if (EntitiesIntersect(entity, entityPtr, &intersection))
                {
                    glm::vec3 penetration;
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
}

internal void BuildExit(GameState* state)
{
    if (state->buildObstacle)
    {
        EntityDestroy(state->entityManager, state->buildObstacle, state->world);
        state->buildObstacle = 0;
    }
}

internal void LoadAssets(Assets* assets)
{
    AssetLoad(assets, AssetID_ZombieTexture);
    AssetLoad(assets, AssetID_CrosshairTexture);
    AssetLoad(assets, AssetID_FenceTexture);
    AssetLoad(assets, AssetID_Fence);
    AssetLoad(assets, AssetID_ZombieFemaleA);
    AssetLoad(assets, AssetID_ZombieMaleA);
    AssetLoad(assets, AssetID_Stickman);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(GameState) <= memory->permanentStorageSize);

    GameState*   state    = (GameState*)memory->permanentStorage;
    PlatformAPI* platform = &memory->platform;
    OpenGL*      gl       = &memory->opengl;
    Arena*       arena    = &state->arena;

    // ----------------------------------------------------------------------------
    // Init
    if (!state->initialized)
    {
        platform->Logf("Initializing game state...");
        state->initialized = true;

        ArenaInit(arena, (size_t)memory->permanentStorageSize - sizeof(GameState),
                  (u8*)memory->permanentStorage + sizeof(GameState));

        state->program               = PushStruct(arena, Program);
        state->camera                = PushStruct(arena, Camera);
        state->planeBuffer           = PushStruct(arena, GPUBuffer);
        state->world                 = PushStruct(arena, World);
        state->world->grid           = PushArray(arena, GRID_CELLS, GridCell);
        state->mode                  = GameMode_Round;
        state->roundMaxEnemy         = 1;
        state->roundCount            = 1;
        state->roundSpawnIntervalSec = 0.3;
        state->roundLastSpawnTime    = 0;
        state->roundEnemyCount       = 0;
        state->buildObstacle         = 0;
        state->buildModeDurationSec  = 10;
        state->buildModeBeginTime    = 0;
        state->entityManager         = PushStruct(arena, EntityManager);

        memset(state->world->grid, 0, sizeof(GridCell) * GRID_CELLS);

        // Renderer setup
        size_t rendererArenaSize = Megabytes(10);
        state->renderer          = PushStruct(arena, Renderer);
        void* rendererPermMem    = PushBlock(arena, rendererArenaSize);
        ArenaInit(&state->renderer->arena, rendererArenaSize, rendererPermMem);
        RendererInit(state->renderer, gl, platform);
        Renderer* renderer = state->renderer;

        // Assets
        {
            state->assets           = PushStruct(arena, Assets);
            state->assets->platform = platform;
            state->assets->renderer = renderer;
            size_t arenaSize        = Megabytes(10);
            void*  arenaBlock       = PushBlock(arena, arenaSize);
            ArenaInit(&state->assets->arena, arenaSize, arenaBlock);
        }
        Assets* assets = state->assets;

#ifdef BUILD_TYPE_DEBUG
        state->debug                    = PushStruct(arena, Debug);
        state->debug->state             = state;
        state->debug->selectedCellIndex = CELL_EMPTY;
#endif
        FileReadResult vertexSourceFile   = platform->FileReadEntire("../src/shaders/basic.vert");
        FileReadResult fragmentSourceFile = platform->FileReadEntire("../src/shaders/basic.frag");

        ProgramInit(renderer, state->program);
        ProgramAttachShader(renderer, state->program, (char*)vertexSourceFile.content, vertexSourceFile.contentSize,
                            GL_VERTEX_SHADER);
        ProgramAttachShader(renderer, state->program, (char*)fragmentSourceFile.content, fragmentSourceFile.contentSize,
                            GL_FRAGMENT_SHADER);
        ProgramBuild(renderer, state->program);

        platform->FileFree(vertexSourceFile.content);
        platform->FileFree(fragmentSourceFile.content);

        // Plane
        {
            size_t vertexSize = sizeof(f32) * 8;

            GPUBufferInit(renderer, state->planeBuffer);
            GPUBufferVBOAlloc(renderer, state->planeBuffer, planeVertexs, sizeof(planeVertexs), vertexSize,
                              GL_STATIC_DRAW);
            GPUBufferEBOAlloc(renderer, state->planeBuffer, planeIndices, ArrayCount(planeIndices) * sizeof(u32),
                              sizeof(u32), GL_STATIC_DRAW);
            GPUBufferVertexAttrib(renderer, state->planeBuffer, 0, 3, GL_FLOAT, vertexSize, offsetof(Vertex, position));
            GPUBufferVertexAttrib(renderer, state->planeBuffer, 1, 3, GL_FLOAT, vertexSize, offsetof(Vertex, normal));
            GPUBufferVertexAttrib(renderer, state->planeBuffer, 2, 2, GL_FLOAT, vertexSize, offsetof(Vertex, uv));
        }

        // Font loading
        RendererTTFLoad(state->renderer, "c:\\windows\\fonts\\calibri.ttf");

        CameraInit(state->camera,          //
                   { 0.0f, 16.0f, 5.0f },  // Position
                   { 0.0f, -0.9f, -0.4f }, // Target
                   { 0.0f, 1.0f, 0.0f },   // Up
                   -68.0f,                 // Pitch
                   -90.0f,                 // Yaw
                   45.0f                   // Fov
        );

        state->pistolShot      = platform->AudioClipLoad("../data/pistol.wav", AudioClipType_Sfx);
        state->backgroundMusic = platform->AudioClipLoad("../data/background.wav", AudioClipType_Music);

        platform->AudioSetVolume(-35.0f, AudioClipType_Music);
        platform->AudioSetVolume(-3.0f, AudioClipType_Sfx);
        // platform->AudioClipPlay(state->backgroundMusic, AudioClipPlayFlag_Loop);

        LoadAssets(assets);
        // GLTFModel                  model      = GLTFParse("../data/ZombieMale_A_joined.gltf", platform);
        // std::vector<GLTFAnimation> animations = GLTFParseAnimations("../data/ZombieMale@attack_left_70f.gltf",
        // platform);

        Entity* player   = EntityNew(state->entityManager, EntityType_Player);
        player->position = glm::vec3{ 0.0f, 0.0f, 0.0f };
        player->size     = glm::vec3{ 1.0f, 1.0f, 1.0f };
        player->aabb     = assets->models[AssetID_Stickman]->aabb;

        Entity* fence0   = EntityNew(state->entityManager, EntityType_Obstacle);
        fence0->position = { 0.0f, 0.0f, -2.0f };
        fence0->size     = { 1.0f, 1.0f, 1.0f };
        fence0->aabb     = assets->models[AssetID_Fence]->aabb;
        BuildPlaceObstacle(state->world, state->entityManager, fence0);

#if 0
        for (u32 i = 0; i < 20; i++)
        {
            {
                Entity* enemy1       = EntityNew(state->entityManager, EntityType_Enemy);
                enemy1->position     = { (f32)i, 0.0f, -12.0f };
                enemy1->size         = { 1.0f, 1.0f, 1.0f };
                enemy1->aabb         = assets->models[AssetID_Stickman]->aabb;
                enemy1->targetEntity = player;
            }
            {
                Entity* enemy1       = EntityNew(state->entityManager, EntityType_Enemy);
                enemy1->position     = { (f32)i, 0.0f, 12.0f };
                enemy1->size         = { 1.0f, 1.0f, 1.0f };
                enemy1->aabb         = assets->models[AssetID_Stickman]->aabb;
                enemy1->targetEntity = player;
            }
        }
#endif

        WorldComputeStaticNodes(state->world, state->entityManager);
    }
    // ----------------------------------------------------------------------------

    // ----------------------------------------------------------------------------
    // Update
    glm::uvec2     windowDim     = platform->WindowGetDimension();
    EntityManager* entityManager = state->entityManager;
    Entity*        player        = EntityGet(entityManager, 0);
    Camera*        camera        = state->camera;
    glm::mat4      projection    = glm::perspective(Radians(45.0f), (f32)windowDim.x / (f32)windowDim.y, 0.1f, 100.0f);
    glm::mat4      view          = CameraView(camera);
    World*         world         = state->world;

    glm::vec4 playerColor{ white };

    local_persist cell_index debugSelectedCellIndex = CELL_EMPTY;

    // TODO: Assign controller to player
    // for (u32 controllerIndex = 0; controllerIndex < ArrayCount(input->controllers); controllerIndex++)
    //{
    //    GameInputController* controller = GetController(input, controllerIndex);
    //}

    GameInputController* controller = GetController(input, 0);

#if 1
    glm::vec3 cameraOffset{ 0.0f, 16.0f, 5.0f };
    CameraSetPitch(camera, -68.0f);
#else
    glm::vec3 cameraOffset{ 0.0f, 4.0f, 8.0f };
    CameraSetPitch(camera, -26.0f);
#endif

#if BUILD_TYPE_DEBUG
    if (ButtonIsPressed(input->debug.f1))
    {
        if (state->mode != GameMode_Build)
        {
            state->mode                 = GameMode_Build;
            state->buildModeDurationSec = 2000;
        }
        else
        {
            state->mode                 = GameMode_Round;
            state->buildModeDurationSec = 10;
            BuildExit(state);
        }
    }

    if (ButtonIsPressed(input->debug.f2))
    {
        CameraToggleTopDownMode(camera);
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
                glm::vec3  crosshairPoint = WorldMousePicking(camera, projection, windowDim, mouse->pos);
                cell_index crosshairCell  = WorldPositionToGridCell(crosshairPoint);

                for (u32 entityIndex = 0; entityIndex < entityManager->entityCount; entityIndex++)
                {
                    Entity* entity = EntityGet(entityManager, entityIndex);
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
                                platform->Logf("Entity at cell %d removed", CELL_INDEX(row, col));
                                EntityDestroy(entityManager, entity, world);
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
            glm::vec3 position = WorldMousePicking(camera, projection, windowDim, mouse->pos);

            state->buildObstacle           = EntityNew(entityManager, EntityType_Obstacle);
            state->buildObstacle->position = position;
            state->buildObstacle->size     = { 1.0f, 1.0f, 1.0f };
            state->buildObstacle->flags |= EntityFlag_Positioning;
            state->buildObstacle->aabb = state->assets->models[AssetID_Fence]->aabb;
        }
        else if (state->buildObstacle)
        {
            // Place object if valid position
            if (ButtonIsPressed(mouse->left))
            {
                if (BuildIsObstacleValidPosition(world, entityManager, state->buildObstacle))
                {
                    BuildPlaceObstacle(state->world, entityManager, state->buildObstacle);
                    WorldComputeStaticNodes(world, entityManager);
                    state->buildObstacle->flags &= ~(EntityFlag_Positioning);
                    state->buildObstacle = 0;
                    EntitiesRemoveFlag(entityManager, EntityFlag_Snapping);
                }
                else
                {
                    platform->Logf("Invalid obstacle position");
                }
            }
            // Cancel placing
            else if (ButtonIsPressed(mouse->right))
            {
                BuildExit(state);
            }
            // Drag, rotate and snap obstacle
            else
            {
                BuildDragObstacle(state->buildObstacle, camera, projection, windowDim, mouse->pos);

                if (ButtonIsPressed(input->keyboard.moveLeft))
                {
                    BuildRotateObstacle(state->buildObstacle, true);
                }
                else if (ButtonIsPressed(input->keyboard.moveRight))
                {
                    BuildRotateObstacle(state->buildObstacle, false);
                }

                if (BuildIsObstacleValidPosition(world, entityManager, state->buildObstacle))
                {
                    state->buildObstacle->flags &= ~EntityFlag_InvalidPosition;
                    SnapCandidate snapCandidate = BuildFindSnapCandidate(world, state->buildObstacle);
                    if (snapCandidate.entity)
                    {
                        BuildSnapObstacles(world, state->buildObstacle, &snapCandidate);
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
        if (state->roundLastSpawnTime == 0)
        {
            Assert(state->roundEnemyCount == 0);
            state->roundLastSpawnTime = time(0);
            state->roundEnemyCount++;

            Entity* enemy       = EntityNew(entityManager, EntityType_Enemy);
            enemy->position     = { -2.0f, 0.0f, -8.0f };
            enemy->size         = { 1.0f, 1.0f, 1.0f };
            enemy->aabb         = state->assets->models[AssetID_Stickman]->aabb;
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

                Entity* enemy       = EntityNew(entityManager, EntityType_Enemy);
                enemy->position     = { -2.0f, 0.0f, -8.0f };
                enemy->size         = { 1.0f, 1.0f, 1.0f };
                enemy->aabb         = state->assets->models[AssetID_Stickman]->aabb;
                enemy->targetEntity = player;
            }
        }

        if (state->roundEnemyCount == state->roundMaxEnemy)
        {
            u32 enemyCount = 0;
            for (u32 entityIndex = 0; entityIndex < entityManager->entityCount; entityIndex++)
            {
                Entity* entity = EntityGet(entityManager, entityIndex);
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

        WorldUpdate(world, entityManager);
        PlayerUpdate(state, player, delta, platform, controller, mouse, cameraOffset);

        for (u32 entityIndex = 0; entityIndex < entityManager->entityCount; entityIndex++)
        {
            Entity* entity = EntityGet(entityManager, entityIndex);
            if (entity->type == EntityType_Enemy)
            {
                EnemyUpdate(entityManager, world, entity, delta);
            }
        }

        break;
    }
    }
    //  ----------------------------------------------------------------------------

    // ----------------------------------------------------------------------------
    // Draw
    Renderer* renderer = state->renderer;

    RendererFrameBegin(renderer, projection * view);

    PushRenderCommand(&renderer->commandQueue, FramebufferClear);

    // 2D
    {
        glm::vec2 crosshairSpriteSize{ 128.0f, 128.0f };
        glm::vec2 cursorSize{ 32.0f, 32.0f };

        Texture* crosshairAtlas = state->assets->textures[1];

        DrawRect(renderer, { (f32)mouse->pos.x, (f32)mouse->pos.y }, cursorSize, crosshairAtlas, { 965.0f, 0.0f },
                 crosshairSpriteSize);

        DrawRect(renderer, { 50.0f, 50.0f }, cursorSize, crosshairAtlas, { 2074.0f, 142.0f }, crosshairSpriteSize);

        char coordBuffer[64];
#if 1
        sprintf(coordBuffer, "%.2f %.2f", player->position.x, player->position.z);
#else
        sprintf(coordBuffer, "%d %d", (int)player->position.x, (int)player->position.z);
#endif
        DrawText(renderer, coordBuffer, { 0.0f, 50.0f }, white);

        // Debug grid
        {
            if (debugSelectedCellIndex != CELL_EMPTY)
            {
                char debugSelectedCellBuffer[64];
                sprintf(debugSelectedCellBuffer, "%d (%d %d)", debugSelectedCellIndex, CELL_ROW(debugSelectedCellIndex),
                        CELL_COL(debugSelectedCellIndex));
                DrawText(renderer, debugSelectedCellBuffer, { 0.0f, 100.0f }, magenta);
            }
        }

        if (state->mode == GameMode_Build)
        {
            DrawText(renderer, "Build", { 0.0f, 150.0f }, green);

            char timerBuf[100];
            int  elapsedSeconds   = (int)difftime(time(0), state->buildModeBeginTime);
            int  remainingSeconds = (int)state->buildModeDurationSec - elapsedSeconds;
            sprintf(timerBuf, "%d", remainingSeconds);
            DrawText(renderer, timerBuf, { windowDim.x - 150.0f, 50.0f }, red);
        }
        else if (state->mode == GameMode_GameOver)
        {
            DrawText(renderer, "GameOver", { 0.0f, 150.0f }, green);
        }
        else if (state->mode == GameMode_Round)
        {
            char buff[100];
            sprintf(buff, "Round %d", state->roundCount);
            DrawText(renderer, buff, { 0.0f, 150.0f }, green);
        }
    }

    // 3D
    {
        // TODO: .obj material
        gl->ActiveTexture(GL_TEXTURE3);
        gl->BindTexture(GL_TEXTURE_2D, state->assets->textures[2]->id); // Fence
        gl->ActiveTexture(GL_TEXTURE4);
        gl->BindTexture(GL_TEXTURE_2D, state->assets->textures[0]->id); // Zombie

        glm::mat4 viewProj = projection * view;

        PushRenderProgramUse(renderer, state->program->id);

        PushRenderUploadUniformInt(renderer, state->program->id, "hasDiffuse", 1);
        PushRenderUploadUniformInt(renderer, state->program->id, "diffuseMap", 4);
        PushRenderUploadUniformVec4(renderer, state->program->id, "color", { 1.0f, 1.0f, 1.0f, 1.0f });

        {
            glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, { 5.0f, 0.0f, -2.0f });
            glm::mat4 scale     = glm::scale(glm::mat4{ 1.0f }, glm::vec3{ 0.015f });
            glm::mat4 model     = translate * scale;

            PushRenderUploadUniformMat4x4(renderer, state->program->id, "mvp", viewProj * model);
            PushRenderDrawBuffer(renderer, state->assets->models[AssetID_ZombieFemaleA]->gpuBuffer);
        }

        {
            glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, { 8.0f, 0.0f, -2.0f });
            glm::mat4 scale     = glm::scale(glm::mat4{ 1.0f }, glm::vec3{ 0.015f });
            glm::mat4 model     = translate * scale;
            PushRenderUploadUniformMat4x4(renderer, state->program->id, "mvp", viewProj * model);
            PushRenderDrawBuffer(renderer, state->assets->models[AssetID_ZombieMaleA]->gpuBuffer);
        }

        // Floor
        {
            glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, { 0.0f, 0.0f, 0.0f });
            glm::mat4 scale     = glm::scale(glm::mat4{ 1.0f }, glm::vec3{ 20.0f });
            glm::mat4 model     = translate * scale;

            PushRenderUploadUniformInt(renderer, state->program->id, "hasDiffuse", 0);
            PushRenderUploadUniformMat4x4(renderer, state->program->id, "mvp", viewProj * model);
            PushRenderUploadUniformVec4(renderer, state->program->id, "color", { 1.0f, 1.0f, 1.0f, 0.5f });
            PushRenderDrawBuffer(renderer, state->planeBuffer);
        }

        // Entities
        {
            for (u32 entityIndex = 0; entityIndex < entityManager->entityCount; entityIndex++)
            {
                Entity* entity = &entityManager->entities[entityIndex];

                glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, entity->position);
                glm::mat4 rotate    = glm::rotate(glm::mat4{ 1.0f }, entity->yaw, { 0.0f, 1.0f, 0.0f });
                glm::mat4 scale     = glm::scale(glm::mat4{ 1.0f }, glm::vec3{ entity->size });
                glm::mat4 model     = translate * rotate * scale;

                if (entity->type != EntityType_Obstacle)
                {
                    PushRenderUploadUniformMat4x4(renderer, state->program->id, "mvp", viewProj * model);
                    PushRenderUploadUniformVec4(renderer, state->program->id, "color",
                                                entityIndex == 0 ? playerColor : green);
                    PushRenderDrawBuffer(renderer, state->assets->models[AssetID_Stickman]->gpuBuffer);
                }
                else
                {
                    glm::vec4 tintColor{ white };

                    if (entity->flags & (EntityFlag_InvalidPosition))
                    {
                        tintColor = red;
                    }
                    else if (entity->flags & (EntityFlag_Positioning | EntityFlag_Snapping))
                    {
                        tintColor = green;
                    }

                    PushRenderUploadUniformInt(renderer, state->program->id, "hasDiffuse", 1);
                    PushRenderUploadUniformInt(renderer, state->program->id, "diffuseMap", 3);
                    PushRenderUploadUniformMat4x4(renderer, state->program->id, "mvp", viewProj * model);
                    PushRenderUploadUniformVec4(renderer, state->program->id, "color", tintColor);
                    PushRenderDrawBuffer(renderer, state->assets->models[AssetID_Fence]->gpuBuffer);
                }
            }
        }
    }

#ifdef BUILD_TYPE_DEBUG
    DebugDraw(state->debug, input, platform);
#endif

    RendererFrameEnd(state->renderer);
    return 0;
}