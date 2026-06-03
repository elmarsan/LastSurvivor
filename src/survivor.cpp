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

internal WorldCollider BuildWorldCollider(glm::vec3 position, glm::vec3 min, glm::vec3 max)
{
    WorldCollider collider;
    collider.position = position;
    collider.aabb.min = min;
    collider.aabb.max = max;

    return collider;
}

global_variable WorldCollider gWorldColliders[] = {
    BuildWorldCollider({ -148.0f, 1.1f, 27.0f }, { -7.0f, -1.0f, -0.3f }, { 6.0f, 1.0f, 0.3f }),
    BuildWorldCollider({ -80.0f, 1.1f, 26.9f }, { -13.9f, -1.0f, -0.4f }, { 15.7f, 1.0f, 0.4f }),
    BuildWorldCollider({ -78.0f, 1.2f, 57.0f }, { -20.0f, -1.0f, -0.4f }, { 19.0f, 1.0f, 0.4f }),
    BuildWorldCollider({ -53.0f, 1.2f, 53.5f }, { -6.7f, -1.0f, -0.4f }, { 5.0f, 1.0f, 0.4f }),
    BuildWorldCollider({ -58.7f, 1.2f, 55.0f }, { -1.0f, -1.0f, -1.5f }, { 1.0f, 1.0f, 1.5f }),
    BuildWorldCollider({ -48.8f, 1.2f, 50.0f }, { -0.3f, -1.0f, -4.6f }, { 0.3f, 1.0f, 3.0f }),
    BuildWorldCollider({ -41.0f, 1.2f, 45.6f }, { -7.5f, -1.0f, -0.2f }, { 5.7f, 1.0f, 0.2f }),
    BuildWorldCollider({ -41.0f, 1.2f, 37.2f }, { -7.8f, -1.0f, -0.2f }, { 5.7f, 1.0f, 0.2f }),
    BuildWorldCollider({ -34.9f, 1.2f, 40.2f }, { -0.2f, -1.0f, -3.0f }, { 0.2f, 1.0f, 6.2f }),
    BuildWorldCollider({ -48.6f, 1.2f, 26.2f }, { -0.2f, -1.0f, -7.7f }, { 0.2f, 1.0f, 11.0f }),
    BuildWorldCollider({ -37.8f, 1.2f, -2.0f }, { 0.2f, -1.0f, -22.7f }, { 0.2f, 1.0f, 20.5f }),
    BuildWorldCollider({ -44.0f, 1.2f, 18.8f }, { -4.4f, -1.0f, -0.3f }, { 6.4f, 1.0f, 0.3f }),
    BuildWorldCollider({ -47.0f, 1.2f, -24.8f }, { -10.2f, -1.0f, -0.3f }, { 9.3f, 1.0f, 0.3f }),
    BuildWorldCollider({ -57.3f, 1.2f, -17.0f }, { -0.3f, -1.0f, -7.5f }, { 0.3f, 1.0f, 8.2f }),
    BuildWorldCollider({ -62.3f, 1.2f, -9.1f }, { -5.6f, -1.0f, -0.3f }, { 5.3f, 1.0f, 0.3f }),
    BuildWorldCollider({ -74.3f, 1.2f, -13.0f }, { -6.3f, -1.0f, -0.3f }, { 6.4f, 1.0f, 0.3f }),
    BuildWorldCollider({ -80.6f, 1.2f, -10.5f }, { -0.3f, -1.0f, -2.3f }, { 0.4f, 1.0f, 1.75f }),
    BuildWorldCollider({ -80.6f, 1.2f, -10.5f }, { -0.3f, -1.0f, -2.3f }, { 0.4f, 1.0f, 1.75f }),
    BuildWorldCollider({ -84.0f, 1.2f, -9.05f }, { -6.3f, -1.0f, -0.3f }, { 3.1f, 1.0f, 0.3f }),
    BuildWorldCollider({ -90.1f, 1.2f, -2.0f }, { -0.3f, -1.0f, -6.8f }, { 0.1f, 1.0f, 4.52f }),
    BuildWorldCollider({ -104.0f, 1.2f, 2.25f }, { -22.48f, -1.0f, -0.3f }, { 14.0f, 1.0f, 0.3f }),
    BuildWorldCollider({ -147.0f, 1.2f, 2.25f }, { -22.68f, -1.0f, -0.3f }, { 13.81f, 1.0f, 0.3f }),
    BuildWorldCollider({ -192.0f, 1.2f, -15.1f }, { -31.15f, -1.0f, -0.3f }, { 23.0f, 1.0f, 0.3f }),
    BuildWorldCollider({ -67.76f, 1.2f, -11.0f }, { -0.15f, -1.0f, -2.3f }, { 0.0f, 1.0f, 2.0f }),
    BuildWorldCollider({ -169.6f, 1.2f, -7.0f }, { -0.15f, -1.0f, -8.5f }, { 0.0f, 1.0f, 9.0f }),
    BuildWorldCollider({ -222.8f, 1.2f, 15.0f }, { -0.15f, -1.0f, -30.0f }, { 0.0f, 1.0f, 22.5f }),
    BuildWorldCollider({ -222.5f, 1.2f, 41.0f }, { -0.15f, -1.0f, -3.5f }, { 0.0f, 1.0f, 3.9f }),
    BuildWorldCollider({ -222.75f, 1.2f, 77.0f }, { -0.15f, -1.0f, -30.5f }, { 0.0f, 1.0f, 28.0f }),
    BuildWorldCollider({ -207.8f, 1.2f, 104.2f }, { -15.0f, -1.0f, -0.5f }, { 22.0f, 1.0f, 0.5f }),
    BuildWorldCollider({ -220.0f, 1.2f, 46.0f }, { -2.7f, -1.0f, -1.05f }, { 1.15f, 1.0f, 1.0f }),
    BuildWorldCollider({ -126.0f, 1.2f, -21.0f }, { -0.5f, -1.0f, -4.4f }, { 9.15f, 1.0f, 23.0f }),
    BuildWorldCollider({ -142.35f, 1.2f, -21.0f }, { -8.5f, -1.0f, -4.4f }, { 9.15f, 1.0f, 23.0f }),
    BuildWorldCollider({ -143.0f, 1.2f, -39.0f }, { -0.3f, -1.0f, -4.6f }, { 0.1f, 1.0f, 3.9f }),
    BuildWorldCollider({ -143.0f, 1.2f, -52.5f }, { -0.3f, -1.0f, -5.3f }, { 0.1f, 1.0f, 4.0f }),
    BuildWorldCollider({ -150.6f, 1.2f, -52.5f }, { -0.3f, -1.0f, -6.15f }, { 0.1f, 1.0f, 27.1f }),
    BuildWorldCollider({ -137.6f, 1.2f, -58.27f }, { -13.0f, -1.0f, -0.3f }, { 20.2f, 1.0f, 0.3f }),
    BuildWorldCollider({ -117.6f, 1.2f, -41.0f }, { -0.3f, -1.0f, -20.0f }, { 0.3f, 1.0f, 15.5f }),
    BuildWorldCollider({ -146.0f, 1.2f, 10.0f }, { -4.05f, -1.0f, -0.2f }, { 2.32f, 1.0f, 0.01f }),
    BuildWorldCollider({ -193.6f, 1.2f, 1.0f }, { -0.40f, -1.0f, -9.2f }, { 0.4f, 1.0f, 8.8f }),
    BuildWorldCollider({ -183.6f, 1.2f, 27.0f }, { -6.35f, -1.0f, -0.3f }, { 6.38f, 1.0f, 0.3f }),
    BuildWorldCollider({ -173.0f, 1.2f, 60.0f }, { -0.3f, -1.0f, -9.7f }, { 0.3f, 1.0f, 13.3f }),
    BuildWorldCollider({ -183.0f, 1.2f, 73.3f }, { -10.0f, -1.0f, -0.2f }, { 21.0f, 1.0f, 0.2f }),
    BuildWorldCollider({ -192.8f, 1.2f, 74.0f }, { -0.3f, -1.0f, -0.88f }, { 0.3f, 1.0f, 2.7f }),
    BuildWorldCollider({ -189.0f, 1.2f, 76.5f }, { -4.0f, -1.0f, -0.3f }, { 3.0f, 1.0f, 0.2f }),
    BuildWorldCollider({ -186.0f, 1.2f, 89.5f }, { -0.3f, -1.0f, -12.8f }, { 0.3f, 1.0f, 14.2f }),
    BuildWorldCollider({ -103.0f, 1.2f, 9.8f }, { -3.85f, -1.0f, -0.05f }, { 2.55f, 1.0f, 0.2f }),
    BuildWorldCollider({ -98.0f, 1.2f, 54.0f }, { -0.3f, -1.0f, -1.0f }, { 0.3f, 1.0f, 2.6f }),
    BuildWorldCollider({ -102.0f, 1.2f, 53.3f }, { -3.5f, -1.0f, -0.3f }, { 3.7f, 1.0f, 0.3f }),
    BuildWorldCollider({ -106.4f, 1.2f, 62.0f }, { -0.3f, -1.0f, -8.3f }, { 0.3f, 1.0f, 10.3f }),
    BuildWorldCollider({ -120.4f, 1.2f, 72.5f }, { -19.0f, -1.0f, -0.3f }, { 13.7f, 1.0f, 0.3f }),
    BuildWorldCollider({ -151.0f, 1.2f, 82.0f }, { -10.8f, -1.0f, -0.3f }, { 11.0f, 1.0f, 0.3f }),
    BuildWorldCollider({ -139.8f, 1.2f, 77.0f }, { -0.3f, -1.0f, -4.2f }, { 0.3f, 1.0f, 5.3f }),
    BuildWorldCollider({ -162.0f, 1.2f, 77.0f }, { -0.3f, -1.0f, -3.2f }, { 0.3f, 1.0f, 5.3f }),
    BuildWorldCollider({ -130.6f, 1.2f, 59.0f }, { -0.6f, -1.0f, -9.55f }, { 0.3f, 1.0f, 13.3f }),
    BuildWorldCollider({ -166.0f, 1.2f, 50.8f }, { -0.35f, -1.0f, -0.3f }, { 0.1f, 1.0f, 0.1f }),
    BuildWorldCollider({ -138.3f, 1.2f, 50.0f }, { -0.5f, -1.0f, -0.3f }, { 0.1f, 1.0f, 0.05f }),
    BuildWorldCollider({ -207.0f, 1.2f, -11.0f }, { -0.6f, -1.0f, -0.3f }, { 0.01f, 1.0f, 0.3f }),
    BuildWorldCollider({ -212.8f, 1.2f, 26.0f }, { -0.5f, -1.0f, -0.3f }, { 0.05f, 1.0f, 0.2f }),
    BuildWorldCollider({ -212.8f, 1.2f, 47.1f }, { -0.5f, -1.0f, -0.3f }, { 0.05f, 1.0f, 0.2f }),
};

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
// TODO: Move collision solving/detection to this function
internal void Physics_Update(EntityManager* manager, f32 delta)
{
    f32 maxSpeed          = 0.0f;
    f32 accelerationSpeed = 0.0f;
    f32 damping           = 0.03f;

    for (u32 entityIndex = 0; entityIndex < manager->entityCount; entityIndex++)
    {
        Entity* entity = Entity_Get(manager, entityIndex);

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
            continue;
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

internal void UpdatePlayer(Entity* player, GameController* controller, EntityManager* manager, f32 delta)
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

    /////////////////////////////////////////////////////////////////////////////////
    // Environment collisions
    AABB worldPlayerAABB = AABBToWorld(player->aabb, player->position);

    for (u32 entityIndex = 0; entityIndex < manager->entityCount; entityIndex++)
    {
        Entity* entity = Entity_Get(manager, entityIndex);
        if (entity->type == EntityType_Collider)
        {
            AABB worldAABB = AABBToWorld(entity->aabb, entity->position);

            AABB overlap;
            if (AABBOverlaps(worldAABB, worldPlayerAABB, &overlap))
            {
                glm::vec3 size = overlap.max - overlap.min;

                glm::vec3 normal{ 0.0f, 0.0f, 0.0f };
                if (size.x < size.y && size.x < size.z)
                {
                    normal.x = (player->position.x < entity->position.x) ? -1.0f : 1.0f;
                }
                else if (size.y < size.z)
                {
                    normal.y = (player->position.y < entity->position.y) ? -1.0f : 1.0f;
                }
                else
                {
                    normal.z = (player->position.z < entity->position.z) ? -1.0f : 1.0f;
                }

                f32 intoWall = glm::dot(player->velocity, normal);
                if (intoWall < 0.0f)
                {
                    player->velocity -= normal * intoWall;
                }
            }
        }
    }
    /////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////
    // Animation
    {
    }
    /////////////////////////////////////////////////////////////////////////////////
}

internal void UpdateEnemies(EntityManager* manager, Assets* assets, f32 delta)
{
    for (u32 entityIndex = 1; entityIndex < manager->entityCount; entityIndex++)
    {
        Entity* entity = Entity_Get(manager, entityIndex);

        if (entity->type != EntityType_Enemy)
        {
            continue;
        }

        entity->forward = SafeNorm(entity->targetEntity->position - entity->position);
        // entity->direction = { 0.0f, 0.0f, 0.0f };

        // Enemy rotation
        {
            glm::vec3 direction = entity->targetEntity->position - entity->position;
            entity->rotation.y  = atan2(direction.x, direction.z);
        }

        // Animation
        {
            Skeleton* skeleton = entity->skeleton;
            Skeleton_UpdatePose(skeleton);

            // Update current animation
            if (entity->animation.current)
            {
                Animation* animation = entity->animation.current;

                entity->animation.time += delta;

                // Loop: start the animation again from the beginning.
                if (entity->animation.time >= animation->duration)
                {
                    entity->animation.time = 0.0f;
                }

                Skeleton_ApplyAnimation(skeleton, animation, entity->animation.time);
            }
            else
            {
                // Set joints to bind pos
                for (u32 jointIndex = 0; jointIndex < skeleton->jointCount; jointIndex++)
                {
                    Joint* joint = skeleton->joints + jointIndex;

                    skeleton->jointGlobalMatrices[jointIndex] = glm::inverse(joint->inverseBindMatrix);
                }
            }
        }
    }
}

internal void LoadAssets(Assets* assets)
{
    Assets_Load(assets, Model_ZombieMaleA);
    Assets_Load(assets, Model_ZombieFemaleA);
    Assets_Load(assets, Model_Parking);

    Assets_Load(assets, Texture_Crosshair);

    Assets_Load(assets, Anim_ZombieMaleAttackLeft);
    Assets_Load(assets, Anim_ZombieMaleAttackRight);
    Assets_Load(assets, Anim_ZombieMaleIdle);
    Assets_Load(assets, Anim_ZombieMaleIdleAlert);
    Assets_Load(assets, Anim_ZombieMaleIdle2);
    Assets_Load(assets, Anim_ZombieMaleRunning);
    Assets_Load(assets, Anim_ZombieMaleSlowWalk);
    Assets_Load(assets, Anim_ZombieMaleWalk);
    Assets_Load(assets, Anim_ZombieMaleWalkAgressive);
    Assets_Load(assets, Anim_ZombieMaleWalkLimp);
    Assets_Load(assets, Anim_ZombieMaleCrawlingForward);
    Assets_Load(assets, Anim_ZombieMaleCrawlingIdle);

    Assets_Load(assets, Anim_ZombieFemaleAttackLeft);
    Assets_Load(assets, Anim_ZombieFemaleAttackRight);
    Assets_Load(assets, Anim_ZombieFemaleIdle);
    Assets_Load(assets, Anim_ZombieFemaleIdleAlert);
    Assets_Load(assets, Anim_ZombieFemaleIdle2);
    Assets_Load(assets, Anim_ZombieFemaleRunning);
    Assets_Load(assets, Anim_ZombieFemaleSlowWalk);
    Assets_Load(assets, Anim_ZombieFemaleWalk);
    Assets_Load(assets, Anim_ZombieFemaleWalkAgressive);
    Assets_Load(assets, Anim_ZombieFemaleWalkLimp);
    Assets_Load(assets, Anim_ZombieFemaleCrawlingForward);
    Assets_Load(assets, Anim_ZombieFemaleCrawlingIdle);
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
        Assets_Init(assets, arena, renderer, platform);
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

        Entity* player  = Entity_Spawn(state->entityManager, EntityType_Player, { -108.0f, 0.0f, 21.15f });
        player->forward = { 0.0f, 0.0f, -1.0f };

        Entity* enemy       = Entity_Spawn(state->entityManager, EntityType_Enemy, { -108.0f, 0.0f, 20.15f });
        enemy->targetEntity = player;

        for (u32 colliderIndex = 0; colliderIndex < ArrayCount(gWorldColliders); colliderIndex++)
        {
            Entity_SpawnCollider(state->entityManager, gWorldColliders[colliderIndex]);
        }

#if 0
        for (u32 i = 0; i < 20; i++)
        {
            {
                Entity* enemy1       = Entity_Spawn(state->entityManager, EntityType_Enemy, { (f32)i, 0.0f, -12.0f });
                enemy1->targetEntity = player;
            }
            {
                Entity* enemy1       = Entity_Spawn(state->entityManager, EntityType_Enemy, { (f32)i, 0.0f, 12.0f });
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
    Entity*        player        = Entity_Get(entityManager, 0);
    Entity*        enemy1        = Entity_Get(entityManager, 1);
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
            // EntityManagerFreeTransient(entityManager);
            //  TODO: game mode transitions
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

        UpdatePlayer(player, controller, entityManager, delta);
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
        Texture* crosshairAtlas = Assets_GetTexture(assets, Texture_Crosshair);

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
            PushRenderProgramUse(renderer, state->program->id);
            PushRenderUploadUniformInt(renderer, state->program->id, "hasDiffuse", true);

            Model* scene = Assets_GetModel(assets, Model_Parking);

            // World matrix
            glm::mat4 worldMatrix = glm::translate(glm::mat4{ 1.0f }, scene->localTranslation) *
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
            Texture* zombieTexture = Assets_GetModel(assets, Model_ZombieMaleA)->textures;

            for (u32 entityIndex = 1; entityIndex < entityManager->entityCount; entityIndex++)
            {
                Entity* entity = &entityManager->entities[entityIndex];

                if (entity->type != EntityType_Enemy)
                {
                    continue;
                }

                PushRenderProgramUse(renderer, state->programSkinned->id);
                PushRenderUploadUniformMat4x4(renderer, state->programSkinned->id, "viewProj", viewProj);
                PushRenderBindTexture(renderer, zombieTexture, 0);
                PushRenderUploadUniformInt(renderer, state->programSkinned->id, "diffuseMap", 0);

                Model* entityModel = Assets_GetModel(assets, entity->assetID);

                glm::mat4 worldMatrix = glm::translate(glm::mat4{ 1.0f }, entity->position) *
                                        glm::mat4_cast(glm::quat(entity->rotation)) *
                                        glm::scale(glm::mat4{ 1.0f }, MODEL_SCALE);

                Skeleton* skeleton = entity->skeleton;

                for (u32 i = 0; i < skeleton->jointCount; i++)
                {
                    char uniformBuffer[64];
                    sprintf(uniformBuffer, "%s[%d]", "uJoints", i);

                    if (entity->animation.current)
                    {
                        u32       jointIndex  = skeleton->jointIndexBindOrder[i];
                        glm::mat4 jointMatrix = skeleton->jointSkinMatrices[jointIndex];
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

                // Debug skeleton
#if 0
                {
                    PushRenderProgramUse(renderer, state->program->id);

                    PushRenderUploadUniformMat4x4(renderer, state->program->id, "viewProj", viewProj);
                    PushRenderUploadUniformInt(renderer, state->program->id, "hasDiffuse", 0);
                    PushRenderUploadUniformVec4(renderer, state->program->id, "color", color_white);

                    for (u32 jointIndex = 0; jointIndex < skeleton->jointCount; jointIndex++)
                    {
                        Joint* joint = &skeleton->joints[jointIndex];

                        glm::mat4 worldPos = worldMatrix * skeleton->jointGlobalMatrices[jointIndex];

                        PushRenderUploadUniformMat4x4(renderer, state->program->id, "world", worldPos);
                        PushRenderDrawBuffer(renderer, state->cubeBuffer, GL_TRIANGLES);

                        glm::vec3 p0 = { worldPos[3][0], worldPos[3][1], worldPos[3][2] };

                        for (u32 childJointIndex = 0; childJointIndex < joint->childrenCount; childJointIndex++)
                        {
                            u32       jointIndex    = joint->childrenIndexes[childJointIndex];
                            glm::mat4 childWorldPos = worldMatrix * skeleton->jointGlobalMatrices[jointIndex];
                            glm::vec3 p1            = { childWorldPos[3][0], childWorldPos[3][1], childWorldPos[3][2] };

                            DrawLine(renderer, p0, p1, color_green);
                        }
                    }
                }
#endif
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