#include "survivor.h"
#include "survivor_renderer_opengl.cpp"
#include "survivor_debug_geometry.cpp"
#include "survivor_debug.cpp"
#include "survivor_assets.cpp"
#include "survivor_entity.cpp"
#include "survivor_world.cpp"
#include "survivor_build.cpp"

// TODO
/*
- (Audio) Make easy to tweak volumes (ignore db conversion)
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
#if 0
                    targetEntity->health = 0;
#endif

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
                f32       deltaYaw  = targetYaw - player->rotation.y;
                deltaYaw            = fmodf(deltaYaw + Pi, 2.0f * Pi) - Pi; // Wrap to [-Pi, Pi]
                player->rotation.y += deltaYaw * rotationSpeed;
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

    // Animation
    {
        if (entity->animation.current)
        {
            Skeleton*  skeleton  = entity->skeleton;
            Animation* animation = entity->animation.current;

            entity->animation.time += delta;
            if (entity->animation.time >= animation->duration)
            {
                entity->animation.time = 0.0f;
            }
            f32 time = entity->animation.time;

            SkeletonApplyAnimation(skeleton, animation, time);
            SkeletonUpdatePose(skeleton);
        }
    }

    // Attack
    {
        glm::vec3 dir = SafeNorm(entity->velocity);
        glm::vec2 hitRectMinCorner{ entity->position.x - enemyHitRadius, entity->position.z - enemyHitRadius };
        glm::vec2 hitRectMaxCorner{ entity->position.x + enemyHitRadius, entity->position.z + enemyHitRadius };

        glm::vec2 targetMinCorner;
        glm::vec2 targetMaxCorner;
        targetMinCorner.x = entity->targetEntity->position.x - (entity->scale.x * 0.5f);
        targetMinCorner.y = entity->targetEntity->position.z - (entity->scale.z * 0.5f);
        targetMaxCorner.x = entity->targetEntity->position.x + (entity->scale.x * 0.5f);
        targetMaxCorner.y = entity->targetEntity->position.z + (entity->scale.z * 0.5f);

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
        // entityDir                = SafeNorm(targetPosition - entity->position);
        // entityDir.y              = 0.0f;
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
            glm::vec3 lookAt{ sinf(entity->rotation.y), 0.0f, cosf(entity->rotation.y) };
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
    AssetsLoad(assets, Texture_Zombie);
    AssetsLoad(assets, Texture_Crosshair);
    AssetsLoad(assets, Texture_Fence);
    AssetsLoad(assets, Model_Fence);
    AssetsLoad(assets, Model_ZombieFemaleA);
    AssetsLoad(assets, Model_ZombieMaleA);
    AssetsLoad(assets, Model_Stickman);

    AssetsLoad(assets, Anim_ZombieMaleAttackLeft);
    AssetsLoad(assets, Anim_ZombieMaleAttackRight);
    AssetsLoad(assets, Anim_ZombieMaleIdle);
    AssetsLoad(assets, Anim_ZombieMaleIdleAlert);
    AssetsLoad(assets, Anim_ZombieMaleIdle2);
    AssetsLoad(assets, Anim_ZombieMaleRunning);
    AssetsLoad(assets, Anim_ZombieMaleSlowWalk);
    AssetsLoad(assets, Anim_ZombieMaleWalk);
    AssetsLoad(assets, Anim_ZombieMaleWalkAgressive);
    AssetsLoad(assets, Anim_ZombieMaleWalkLimp);

    AssetsLoad(assets, Anim_ZombieFemaleAttackLeft);
    AssetsLoad(assets, Anim_ZombieFemaleAttackRight);
    AssetsLoad(assets, Anim_ZombieFemaleIdle);
    AssetsLoad(assets, Anim_ZombieFemaleIdleAlert);
    AssetsLoad(assets, Anim_ZombieFemaleIdle2);
    AssetsLoad(assets, Anim_ZombieFemaleRunning);
    AssetsLoad(assets, Anim_ZombieFemaleSlowWalk);
    AssetsLoad(assets, Anim_ZombieFemaleWalk);
    AssetsLoad(assets, Anim_ZombieFemaleWalkAgressive);
    AssetsLoad(assets, Anim_ZombieFemaleWalkLimp);
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
        srand((unsigned int)time(NULL));

        platform->Logf("Initializing game state...");
        state->initialized = true;

        ArenaInit(arena, (size_t)memory->permanentStorageSize - sizeof(GameState),
                  (u8*)memory->permanentStorage + sizeof(GameState));

        state->program               = PushStruct(arena, Program);
        state->programSkinned        = PushStruct(arena, Program);
        state->camera                = PushStruct(arena, Camera);
        state->planeBuffer           = PushStruct(arena, GPUBuffer);
        state->world                 = PushStruct(arena, World);
        state->world->grid           = PushArray(arena, GRID_CELLS, GridCell);
        state->entityManager         = PushStruct(arena, EntityManager);
        state->renderer              = PushStruct(arena, Renderer);
        state->assets                = PushStruct(arena, Assets);
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
        state->debug                    = PushStruct(arena, Debug);
        state->debug->state             = state;
        state->debug->selectedCellIndex = CELL_EMPTY;
#endif

        memset(state->world->grid, 0, sizeof(GridCell) * GRID_CELLS);

        Renderer* renderer = state->renderer;
        Assets*   assets   = state->assets;

        RendererInit(renderer, arena, gl, platform);
        AssetsInit(assets, arena, renderer, platform);
        EntityManagerInit(state->entityManager, arena, assets);

        // Basic program
        {
            FileReadResult vertexSourceFile   = platform->FileReadEntire("../src/shaders/basic.vert");
            FileReadResult fragmentSourceFile = platform->FileReadEntire("../src/shaders/basic.frag");

            ProgramInit(renderer, state->program);
            ProgramAttachShader(renderer, state->program, (char*)vertexSourceFile.content, vertexSourceFile.contentSize,
                                GL_VERTEX_SHADER);
            ProgramAttachShader(renderer, state->program, (char*)fragmentSourceFile.content,
                                fragmentSourceFile.contentSize, GL_FRAGMENT_SHADER);
            ProgramBuild(renderer, state->program);

            platform->FileFree(vertexSourceFile.content);
            platform->FileFree(fragmentSourceFile.content);
        }

        // Skinned program
        {
            FileReadResult vertexSourceFile   = platform->FileReadEntire("../src/shaders/skinned.vert");
            FileReadResult fragmentSourceFile = platform->FileReadEntire("../src/shaders/skinned.frag");

            ProgramInit(renderer, state->programSkinned);
            ProgramAttachShader(renderer, state->programSkinned, (char*)vertexSourceFile.content,
                                vertexSourceFile.contentSize, GL_VERTEX_SHADER);
            ProgramAttachShader(renderer, state->programSkinned, (char*)fragmentSourceFile.content,
                                fragmentSourceFile.contentSize, GL_FRAGMENT_SHADER);
            ProgramBuild(renderer, state->programSkinned);

            platform->FileFree(vertexSourceFile.content);
            platform->FileFree(fragmentSourceFile.content);
        }

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

        Entity* player = EntitySpawn(state->entityManager, EntityType_Player, { 0.0f, 0.0f, 0.0f });
        Entity* fence0 = EntitySpawn(state->entityManager, EntityType_Obstacle, { 0.0f, 0.0f, -2.0f });
        BuildPlaceObstacle(state->world, state->entityManager, fence0);

#if 0
        for (u32 i = 0; i < 20; i++)
        {
            {
                Entity* enemy1       = EntitySpawn(state->entityManager, EntityType_Enemy, { (f32)i, 0.0f, -12.0f });
                enemy1->targetEntity = player;
            }
            {
                Entity* enemy1       = EntitySpawn(state->entityManager, EntityType_Enemy, { (f32)i, 0.0f, 12.0f });
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
    Assets*        assets        = state->assets;

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
                EntityManagerFreeTransient(entityManager);
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

            state->buildObstacle = EntitySpawn(entityManager, EntityType_Obstacle, position);
            state->buildObstacle->flags |= EntityFlag_Positioning;
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
            EntityManagerFreeTransient(entityManager);
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

            Entity* enemy       = EntitySpawn(entityManager, EntityType_Enemy, { -2.0f, 0.0f, -8.0f });
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

                Entity* enemy       = EntitySpawn(entityManager, EntityType_Enemy, { -2.0f, 0.0f, -8.0f });
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

        // Debug animations
        {
            if (ButtonIsPressed(input->debug.f3))
            {
                Entity* enemy = EntityGet(entityManager, 2);

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
                        enemy->animation.current = AssetsAnimationGet(assets, Anim_ZombieMaleSlowWalk);
                    }
                    else if (enemy->assetID == Model_ZombieFemaleA)
                    {
                        enemy->animation.current = AssetsAnimationGet(assets, Anim_ZombieFemaleWalk);
                        // enemy->animation.current = AssetsAnimationGet(assets, Anim_ZombieFemaleAttackLeft);
                        // enemy->animation.current = AssetsAnimationGet(assets, Anim_ZombieFemaleIdle);
                    }
                    else
                    {
                        Assert(0);
                    }
                }
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
    // ----------------------------------------------------------------------------

    // ----------------------------------------------------------------------------
    // Draw
    Renderer* renderer = state->renderer;

    RendererFrameBegin(renderer, projection * view);

    PushRenderCommand(&renderer->commandQueue, FramebufferClear);

    Texture* crosshairAtlas = AssetsTextureGet(assets, Texture_Crosshair);
    Texture* zombieTexture  = AssetsTextureGet(assets, Texture_Zombie);
    Texture* fenceTexture   = AssetsTextureGet(assets, Texture_Fence);

    // 2D
    {
        glm::vec2 crosshairSpriteSize{ 128.0f, 128.0f };
        glm::vec2 cursorSize{ 32.0f, 32.0f };

        Texture* crosshairAtlas = AssetsTextureGet(assets, Texture_Crosshair);

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
        gl->ActiveTexture(GL_TEXTURE0);
        gl->BindTexture(GL_TEXTURE_2D, fenceTexture->id);
        gl->ActiveTexture(GL_TEXTURE1);
        gl->BindTexture(GL_TEXTURE_2D, zombieTexture->id);

        glm::mat4 viewProj = projection * view;

        // Floor
        {
            glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, { 0.0f, 0.0f, 0.0f });
            glm::mat4 scale     = glm::scale(glm::mat4{ 1.0f }, glm::vec3{ 20.0f });
            glm::mat4 model     = translate * scale;

            PushRenderProgramUse(renderer, state->program->id);
            PushRenderUploadUniformInt(renderer, state->program->id, "hasDiffuse", 0);
            PushRenderUploadUniformMat4x4(renderer, state->program->id, "mvp", viewProj * model);
            PushRenderUploadUniformVec4(renderer, state->program->id, "color", { 1.0f, 1.0f, 1.0f, 0.5f });
            PushRenderDrawBuffer(renderer, state->planeBuffer);
        }

        // Entities
        {
            for (u32 entityIndex = 0; entityIndex < entityManager->entityCount; entityIndex++)
            {
                Entity* entity      = &entityManager->entities[entityIndex];
                Model*  entityModel = AssetsModelGet(assets, entity->assetID);

                glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, entity->position);
                glm::mat4 rotate    = glm::mat4_cast(glm::quat(entity->rotation));
                glm::mat4 scale     = glm::scale(glm::mat4{ 1.0f }, entity->scale);
                glm::mat4 model     = translate * rotate * scale;

                if (entity->type == EntityType_Obstacle)
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

                    PushRenderProgramUse(renderer, state->program->id);
                    PushRenderUploadUniformInt(renderer, state->program->id, "hasDiffuse", 1);
                    PushRenderUploadUniformInt(renderer, state->program->id, "diffuseMap", 0);
                    PushRenderUploadUniformMat4x4(renderer, state->program->id, "mvp", viewProj * model);
                    PushRenderDrawBuffer(renderer, entityModel->gpuBuffer);
                }
                else if (entity->type == EntityType_Player)
                {
                    PushRenderProgramUse(renderer, state->program->id);
                    PushRenderUploadUniformInt(renderer, state->program->id, "hasDiffuse", 0);
                    PushRenderUploadUniformMat4x4(renderer, state->program->id, "mvp", viewProj * model);
                    PushRenderUploadUniformVec4(renderer, state->program->id, "color", green);
                    PushRenderDrawBuffer(renderer, entityModel->gpuBuffer);
                }
                else if (entity->type == EntityType_Enemy)
                {
                    PushRenderProgramUse(renderer, state->programSkinned->id);
                    PushRenderUploadUniformInt(renderer, state->programSkinned->id, "diffuseMap", 1);

                    Model*    entityModel = AssetsModelGet(assets, entity->assetID);
                    Skeleton* skeleton    = entity->skeleton;

                    for (u32 i = 0; i < skeleton->jointCount - 1; i++)
                    {
                        char uniformBuffer[64];
                        sprintf(uniformBuffer, "%s[%d]", "uJoints", i);

                        if (entity->animation.current)
                        {
                            u32       jointIndex  = skeleton->jointIndexBindOrder[i];
                            glm::mat4 jointMatrix = skeleton->jointMatrices[jointIndex];
                            PushRenderUploadUniformMat4x4(renderer, state->programSkinned->id, uniformBuffer,
                                                          jointMatrix);
                        }
                        else
                        {
                            PushRenderUploadUniformMat4x4(renderer, state->programSkinned->id, uniformBuffer,
                                                          glm::mat4{ 1.0f });
                        }
                    }

                    PushRenderUploadUniformMat4x4(renderer, state->programSkinned->id, "mvp", viewProj * model);
                    PushRenderDrawBuffer(renderer, entityModel->gpuBuffer);
                }
                else
                {
                    Assert(0);
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