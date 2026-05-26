#include "survivor.h"
#include "survivor_renderer_opengl.cpp"
#include "survivor_debug_geometry.cpp"
#include "survivor_assets.cpp"
#include "survivor_entity.cpp"
#include "survivor_ui.cpp"
#if BUILD_TYPE_DEBUG
#include "survivor_debug.cpp"
#endif

// TODO
/*
- (Audio) Make easy to tweak volumes (ignore db conversion)
- (Game): game mode transitions
*/

internal void Shoot(AmmoRound* ammoRound, AmmoRoundType type, glm::vec3 position, glm::vec3 direction)
{
    Particle* particle = &ammoRound->particle;

    direction            = SafeNorm(direction);
    ammoRound->type      = type;
    particle->position   = position;
    particle->forceAccum = glm::vec3{ 0.0f, 0.0f, 0.0f };

    switch (ammoRound->type)
    {
    case PISTOL:
    {
        f32 speed = 35.0f; // 35m/s

        Particle_SetMass(particle, 2.0f); // 2.0kg
        particle->velocity     = direction * speed;
        particle->acceleration = GRAVITY;
        particle->damping      = 0.99f;
        break;
    }
    // case ARTILLERY:
    //{
    //     Particle_SetMass(particle, 200.0f); // 200.0kg
    //     particle->velocity     = glm::vec3{ 0.0f, 30.0f, -40.0f };
    //     particle->acceleration = glm::vec3{ 0.0f, -20.0f, 0.0f };
    //     particle->damping      = 0.99f;
    //     break;
    // }
    case GRENADE:
    {
        f32 speed = 10.0f; // 10m/s

        Particle_SetMass(particle, 1.0f); // 1.0kg
        particle->velocity     = direction * speed;
        particle->velocity.y   = 10.0f;
        particle->acceleration = glm::vec3{ 0.0f, -20.0f, 0.0f };
        particle->damping      = 0.8f;

        break;
    }

        InvalidDefaultCase;
    }
}

// Physics system tick
internal void Physics_Update(EntityManager* manager, f32 delta)
{
    f32 maxSpeed          = 0.0f;
    f32 accelerationSpeed = 0.0f;
    f32 damping           = 0.03f;

    for (u32 entityIndex = 0; entityIndex < manager->entityCount; entityIndex++)
    {
        Entity* entity = EntityGet(manager, entityIndex);

        if (entity->type == EntityType_Player)
        {
            maxSpeed          = playerMaxSpeed;
            accelerationSpeed = playerMoveAcceleration;
        }
        else if (entity->type == EntityType_Enemy)
        {
            maxSpeed          = enemyMaxSpeed;
            accelerationSpeed = enemyAcceleration;
        }
        else
        {
            InvalidCodePath;
        }

        // Update linear position
        entity->position += (entity->velocity * delta);

        glm::vec3 acceleration = entity->wishDir * accelerationSpeed;

        entity->velocity += acceleration * delta;
        entity->velocity *= powf(damping, delta);

        f32 speed = glm::length(entity->velocity);
        if (speed > maxSpeed)
        {
            entity->velocity = SafeNorm(entity->velocity) * maxSpeed;
        }
    }
}

internal void UpdatePlayer(Entity* player, GameController* controller)
{
    Assert(player->type == EntityType_Player);

    f32 xOffset = 0.0f;
    f32 yOffset = 0.0f;

    // TODO: Config mouse/gamepad sensitivity
    if (controller->type == ControllerType_Keyboard)
    {
        local_persist f32 mouseSensitivity = 0.1f;

        xOffset = (f32)controller->mouse.delta.x * mouseSensitivity;
        yOffset = (f32)controller->mouse.delta.y * mouseSensitivity;
    }
    else /* Gamepad */
    {
        local_persist f32 gamepadSensitivity = 0.5f;

        xOffset = (f32)controller->gamepad.rightStick.x * gamepadSensitivity;
        yOffset = (f32)controller->gamepad.rightStick.y * gamepadSensitivity;
    }

    /////////////////////////////////////////////////////////////////////////////////
    // Camera rotation
    player->rotation.x += Radians(yOffset); // Pitch
    player->rotation.y += Radians(xOffset); // Yaw

    // Clamp pitch
    if (player->rotation.x > Radians(89.0f))
    {
        player->rotation.x = Radians(89.0f);
    }
    if (player->rotation.x < Radians(-89.0f))
    {
        player->rotation.x = Radians(-89.0f);
    }

    f32 pitch = player->rotation.x;
    f32 yaw   = player->rotation.y;

    glm::vec3 forward;
    forward.x       = cosf(yaw) * cosf(pitch);
    forward.y       = sinf(pitch);
    forward.z       = sinf(yaw) * cosf(pitch);
    player->forward = glm::normalize(forward);
    /////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////
    // Movement
    glm::vec3 moveForward;
    moveForward.x = cosf(yaw);
    moveForward.y = 0.0f;
    moveForward.z = sinf(yaw);
    moveForward   = glm::normalize(moveForward);

    glm::vec3 worldUp{ 0.0f, 1.0f, 0.0f };
    glm::vec3 right = glm::normalize(glm::cross(moveForward, worldUp));

    player->wishDir = { 0.0f, 0.0f, 0.0f };
    if (controller->type == ControllerType_Keyboard)
    {
        if (ButtonIsDown(controller->moveUp))
        {
            player->wishDir += moveForward;
        }
        if (ButtonIsDown(controller->moveDown))
        {
            player->wishDir -= moveForward;
        }
        if (ButtonIsDown(controller->moveLeft))
        {
            player->wishDir -= right;
        }
        if (ButtonIsDown(controller->moveRight))
        {
            player->wishDir += right;
        }
    }
    else /* Gamepad */
    {
        Gamepad gamepad = controller->gamepad;
        player->wishDir += moveForward * gamepad.leftStick.y;
        player->wishDir += right * gamepad.leftStick.x;
    }
    player->wishDir = SafeNorm(player->wishDir);
    /////////////////////////////////////////////////////////////////////////////////
}

internal void UpdateEnemies(EntityManager* manager, Assets* assets, f32 delta)
{
    for (u32 entityIndex = 1; entityIndex < manager->entityCount; entityIndex++)
    {
        Entity* entity = EntityGet(manager, entityIndex);

        entity->forward = SafeNorm(entity->targetEntity->position - entity->position);
        // entity->direction = { 0.0f, 0.0f, 0.0f };

        // Enemy rotation
        {
            glm::vec3 direction = entity->targetEntity->position - entity->position;
            entity->rotation.y  = atan2(direction.x, direction.z);
        }

        // Animation
        {
            // Update current animation
            if (entity->animation.current)
            {
                Skeleton*  skeleton  = entity->skeleton;
                Animation* animation = entity->animation.current;

                // f32 prevTime    = entity->animation.time;d
                // f32 currentTime = 0.0f;
                entity->animation.time += delta;
                // currentTime = entity->animation.time;

                // Loop
                if (entity->animation.time >= animation->duration)
                {
                    entity->animation.time = 0.0f;
                }

                SkeletonApplyAnimation(skeleton, animation, entity->animation.time);
                SkeletonUpdatePose(skeleton);
            }

            else if (!entity->animation.current && glm::length(entity->velocity) > 0.0f)
            {
                entity->animation.current = AssetsAnimationGet(assets, Anim_ZombieFemaleWalk);
                entity->animation.time    = 0.0f;
            }
        }
    }
}

internal void LoadAssets(Assets* assets)
{
    AssetsLoad(assets, Model_ZombieMaleA);
    AssetsLoad(assets, Model_ZombieFemaleA);
    AssetsLoad(assets, Model_Parking);

    AssetsLoad(assets, Texture_Crosshair);

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
    AssetsLoad(assets, Anim_ZombieMaleCrawlingForward);
    AssetsLoad(assets, Anim_ZombieMaleCrawlingIdle);

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
    AssetsLoad(assets, Anim_ZombieFemaleCrawlingForward);
    AssetsLoad(assets, Anim_ZombieFemaleCrawlingIdle);
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

        state->program        = PushStruct(arena, Program);
        state->programSkinned = PushStruct(arena, Program);
        state->cubeBuffer     = PushStruct(arena, GPUBuffer);
        state->entityManager  = PushStruct(arena, EntityManager);
        state->renderer       = PushStruct(arena, Renderer);
        state->assets         = PushStruct(arena, Assets);
        state->ui             = PushStruct(arena, UI);
        state->mode           = GameMode_Play;
#ifdef BUILD_TYPE_DEBUG
        state->debug        = PushStruct(arena, Debug);
        state->debug->state = state;
#endif

        Renderer* renderer = state->renderer;
        Assets*   assets   = state->assets;

        RendererInit(renderer, arena, gl, platform);
        AssetsInit(assets, arena, renderer, platform);
        EntityManagerInit(state->entityManager, arena, assets);
        UI_Init(state->ui, arena, renderer, platform);

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

        // Cube
        {
            size_t vertexSize = sizeof(f32) * 8;

            GPUBufferInit(renderer, state->cubeBuffer);
            GPUBufferVBOAlloc(renderer, state->cubeBuffer, cubeVertexs, sizeof(cubeVertexs), vertexSize,
                              GL_STATIC_DRAW);
            GPUBufferVertexAttrib(renderer, state->cubeBuffer, 0, 3, GL_FLOAT, vertexSize, offsetof(Vertex, position));
            GPUBufferVertexAttrib(renderer, state->cubeBuffer, 1, 3, GL_FLOAT, vertexSize, offsetof(Vertex, normal));
            GPUBufferVertexAttrib(renderer, state->cubeBuffer, 2, 2, GL_FLOAT, vertexSize, offsetof(Vertex, uv));
        }

        // Font loading
        RendererTTFLoad(state->renderer, "../data/november/novem___.ttf");

        state->pistolShot      = platform->AudioClipLoad("../data/pistol.wav", AudioClipType_Sfx);
        state->backgroundMusic = platform->AudioClipLoad("../data/background.wav", AudioClipType_Music);

        platform->AudioSetVolume(-35.0f, AudioClipType_Music);
        platform->AudioSetVolume(-3.0f, AudioClipType_Sfx);
        // platform->AudioClipPlay(state->backgroundMusic, AudioClipPlayFlag_Loop);

        LoadAssets(assets);

        Entity* player  = EntitySpawn(state->entityManager, EntityType_Player, { -108.0f, 0.0f, 21.15f });
        player->forward = { 0.0f, 0.0f, -1.0f };

        Entity* enemy = EntitySpawn(state->entityManager, EntityType_Enemy, { -108.0f, 0.0f, 20.15f });

        enemy->targetEntity = player;

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

        // WorldComputeStaticNodes(state->world, state->entityManager);

        platform->CursorHide();
    }
    // ----------------------------------------------------------------------------

    // ----------------------------------------------------------------------------
    // Update
    b32 closeGame = false;

    glm::uvec2     windowDim     = platform->WindowGetDimension();
    EntityManager* entityManager = state->entityManager;
    Entity*        player        = EntityGet(entityManager, 0);
    Assets*        assets        = state->assets;
    UI*            ui            = state->ui;

    // Set current controller
    GameController* keyboard   = GetController(input, CONTROLLER_KEYBOARD);
    GameController* gamepad    = GetController(input, CONTROLLER_GAMEPAD);
    GameController* controller = keyboard;
    if (gamepad->lastTick > keyboard->lastTick)
    {
        controller = gamepad;
    }

    if (player->health <= 0)
    {
        // TODO: game mode transitions
        state->mode = GameMode_GameOver;
    }

    Mouse* mouse = &input->keyboard.mouse;

    switch (state->mode)
    {
    case GameMode_Pause:
    {
        if (controller == keyboard)
        {
            platform->CursorShow();
        }

        if (ButtonIsPressed(controller->start))
        {
            // TODO: game mode transitions
            state->mode = GameMode_Play;
        }
        break;
    }
    case GameMode_GameOver:
    {
        if (ButtonIsPressed(mouse->left))
        {
            EntityManagerFreeTransient(entityManager);
            // TODO: game mode transitions
            state->mode = GameMode_Play;
        }
        break;
    }
    case GameMode_Play:
    {
        platform->CursorHide();

        if (ButtonIsPressed(controller->start))
        {
            // TODO: game mode transitions
            state->mode = GameMode_Pause;
        }

        UpdatePlayer(player, controller);
        UpdateEnemies(entityManager, assets, delta);
        Physics_Update(entityManager, delta);

        break;
    }
    }

    AmmoRound* ammoRound = &state->ammoRound;

    // Ballistic test
    {
        // Shoot
        if (ammoRound->type == UNKNOWN)
        {
            glm::vec3 position{ player->position.x, 1.5f, player->position.z };
            glm::vec3 direction{ sinf(player->rotation.y), 0.0f, cosf(player->rotation.y) };

#if BUILD_TYPE_DEBUG

            if (ButtonIsPressed(input->debug.f1))
            {
                platform->Logf("Shooting....");
                Shoot(ammoRound, PISTOL, position, direction);
            }
            else if (ButtonIsPressed(input->debug.f2))
            {
                platform->Logf("Shooting....");
                Shoot(ammoRound, GRENADE, position, direction);
            }
#endif
        }

        // Update
        Particle_Integrate(&ammoRound->particle, delta);
        if (ammoRound->type != UNKNOWN &&
            (ammoRound->particle.position.z < -50.0f || ammoRound->particle.position.z > 50.0f ||
             ammoRound->particle.position.y < 0.0f))
        {
            platform->Logf("Ammo round destroyed");
            ammoRound->type = UNKNOWN;
        }
    }

    // ----------------------------------------------------------------------------

    // ----------------------------------------------------------------------------
    // Draw
    glm::mat4 projection = glm::perspective(Radians(45.0f), (f32)windowDim.x / (f32)windowDim.y, 0.1f, 500.0f);
    glm::vec3 cameraEye  = player->position + glm::vec3{ 0.0f, 2.2f, 0.0f };
    glm::mat4 view       = glm::lookAt(cameraEye, cameraEye + player->forward, { 0.0f, 1.0f, 0.0f });

    glm::mat4 viewProj = projection * view;
    Renderer* renderer = state->renderer;
    RendererFrameBegin(renderer, viewProj);
    PushRenderCommand(&renderer->commandQueue, FramebufferClear);

#ifdef BUILD_TYPE_DEBUG
    DebugUpdateAndRender(state->debug, input, platform);
#endif

    switch (state->mode)
    {
    case GameMode_Play:
    {
        Texture* crosshairAtlas = AssetsTextureGet(assets, Texture_Crosshair);

        // 2D
        {
            glm::vec2 crosshairSpriteSize{ 128.0f, 128.0f };
            glm::vec2 aimingDotSize{ 64.0f, 64.0f };
            glm::vec2 aimingDotPos{ (f32)windowDim.x / 2.0f - aimingDotSize.x / 2.0f,
                                    (f32)windowDim.y / 2.0f - aimingDotSize.y / 2.0f };

            // TODO: Pass tint color
            DrawRect(renderer, aimingDotPos, aimingDotSize, crosshairAtlas, { 0.0f, 0.0f }, crosshairSpriteSize);
        }

        // 3D
        PushRenderProgramUse(renderer, state->program->id);
        PushRenderUploadUniformMat4x4(renderer, state->program->id, "viewProj", viewProj);

        // Projectile
        {
            if (ammoRound->type != UNKNOWN)
            {
                glm::mat4 world = glm::translate(glm::mat4{ 1.0f }, ammoRound->particle.position) *
                                  glm::scale(glm::mat4{ 1.0f }, glm::vec3{ 0.15f });

                PushRenderUploadUniformInt(renderer, state->program->id, "hasDiffuse", false);
                PushRenderUploadUniformMat4x4(renderer, state->program->id, "world", world);
                PushRenderUploadUniformVec4(renderer, state->program->id, "color", color_red);
                PushRenderDrawBuffer(renderer, state->cubeBuffer);
            }
        }

        // Parking scene
        {
            PushRenderUploadUniformInt(renderer, state->program->id, "hasDiffuse", true);

            Model* scene = AssetsModelGet(assets, Model_Parking);

            // World matrix
            glm::mat4 worldMatrix = glm::translate(glm::mat4{ 1.0f }, { 0.0f, 0.0f, 0.0f }) *
                                    glm::mat4_cast(scene->localRotation) * glm::scale(glm::mat4{ 1.0f }, MODEL_SCALE);

            for (u32 meshIndex = 0; meshIndex < scene->meshCount; meshIndex++)
            {
                Mesh*     mesh      = scene->meshes + meshIndex;
                Material* material  = scene->materials + mesh->materialIndex;
                Texture*  baseColor = scene->textures + material->baseColorIndex;

                PushRenderBindTexture(renderer, baseColor, 0);
                PushRenderUploadUniformInt(renderer, state->program->id, "diffuseMap", 0);
                PushRenderUploadUniformMat4x4(renderer, state->program->id, "world", worldMatrix);
                PushRenderDrawBuffer(renderer, mesh->gpuBuffer);
            }
        }

        // Entities
        {
            Texture* zombieTexture = AssetsModelGet(assets, Model_ZombieMaleA)->textures;

            PushRenderProgramUse(renderer, state->programSkinned->id);
            PushRenderUploadUniformMat4x4(renderer, state->programSkinned->id, "viewProj", viewProj);
            PushRenderBindTexture(renderer, zombieTexture, 0);
            PushRenderUploadUniformInt(renderer, state->programSkinned->id, "diffuseMap", 0);

            for (u32 entityIndex = 1; entityIndex < entityManager->entityCount; entityIndex++)
            {
                Entity* entity      = &entityManager->entities[entityIndex];
                Model*  entityModel = AssetsModelGet(assets, entity->assetID);

                glm::mat4 worldMatrix = glm::translate(glm::mat4{ 1.0f }, entity->position) *
                                        glm::mat4_cast(glm::quat(entity->rotation)) *
                                        glm::scale(glm::mat4{ 1.0f }, MODEL_SCALE);

                Assert(entity->type == EntityType_Enemy);

                Skeleton* skeleton = entity->skeleton;

                for (u32 i = 0; i < skeleton->jointCount - 1; i++)
                {
                    char uniformBuffer[64];
                    sprintf(uniformBuffer, "%s[%d]", "uJoints", i);

                    if (entity->animation.current)
                    {
                        u32       jointIndex  = skeleton->jointIndexBindOrder[i];
                        glm::mat4 jointMatrix = skeleton->jointMatrices[jointIndex];
                        PushRenderUploadUniformMat4x4(renderer, state->programSkinned->id, uniformBuffer, jointMatrix);
                    }
                    else
                    {
                        PushRenderUploadUniformMat4x4(renderer, state->programSkinned->id, uniformBuffer,
                                                      glm::mat4{ 1.0f });
                    }
                }

                PushRenderUploadUniformMat4x4(renderer, state->programSkinned->id, "world", worldMatrix);
                PushRenderDrawBuffer(renderer, entityModel->meshes->gpuBuffer);
            }
        }

        break;
    }
    case GameMode_Pause:
    {
        UI_BeginFrame(ui, controller);
        {
            UI_Node* container   = UI_BeginNode(ui, "container");
            container->bgColor   = color_black;
            container->width     = UI_FIXED(windowDim.x);
            container->height    = UI_FIXED(windowDim.y);
            container->direction = UI_Direction_TopToBottom;
            container->childGap  = 24.0f;
            container->alignX    = UI_Align_Center;
            container->alignY    = UI_Align_Center;
            container->padding   = { 8.0f, 8.0f, 8.0f, 8.0f };
            {
                f32 scale = 0.5f;

                enum
                {
                    Btn_Continue,
                    Btn_Restart,
                    Btn_Settings,
                    Btn_Quit
                };
                char* options[] = { "Continue", "Restart", "Settings", "Quit" };

                glm::vec2 maxTextSize{ 0.0f, 0.0f };
                for (u32 optionIndex = 0; optionIndex < ArrayCount(options); optionIndex++)
                {
                    glm::vec2 textSize = UI_GetTextSize(ui, options[optionIndex], scale);

                    maxTextSize.x = Max(maxTextSize.x, textSize.x);
                    maxTextSize.y = Max(maxTextSize.y, textSize.y);
                }

                glm::vec2 btnSize{ maxTextSize.x + maxTextSize.x * 0.7f, maxTextSize.y + maxTextSize.y * 0.55f };

                for (u32 optionIndex = 0; optionIndex < ArrayCount(options); optionIndex++)
                {
                    char id[32];
                    sprintf(id, "button_%d", optionIndex);

                    if (UI_Button(ui, id, options[optionIndex], scale, btnSize))
                    {
                        platform->Logf("%s", options[optionIndex]);

                        switch (optionIndex)
                        {
                        case Btn_Continue:
                        {
                            state->mode = GameMode_Play;
                            break;
                        }
                        case Btn_Restart:
                        {
                            break;
                        }
                        case Btn_Settings:
                        {
                            break;
                        }
                        case Btn_Quit:
                        {
                            closeGame = true;
                            break;
                        }
                        }
                    }
                }
            }
            UI_EndNode(ui);
        }
        UI_EndFrame(ui);
        break;
    }
    }

    RendererFrameEnd(state->renderer);
    return closeGame;
}