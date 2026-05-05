#include "survivor.h"
#include "survivor_renderer_opengl.cpp"
#include "survivor_debug_geometry.cpp"
#include "survivor_assets.cpp"
#include "survivor_entity.cpp"
#include "survivor_world.cpp"
// #include "survivor_build.cpp"
#include "survivor_ui.cpp"
#if BUILD_TYPE_DEBUG
#include "survivor_debug.cpp"
#endif

// TODO
/*
- (Audio) Make easy to tweak volumes (ignore db conversion)
- (Game): gamepad controller
- (Game): game mode transitions
- (Game): camera
*/

internal void Shoot(AmmoRound* ammoRound, AmmoRoundType type, glm::vec3 position, glm::vec3 direction)
{
    Particle* particle = &ammoRound->particle;

    ammoRound->type = type;

    particle->position   = position;
    particle->forceAccum = glm::vec3{ 0.0f, 0.0f, 0.0f };

    switch (ammoRound->type)
    {
    case PISTOL:
    {
        f32 speed          = 35.0f; // 35m/s
        particle->velocity = direction * speed;

        f32 speed = glm::length(particle->velocity);
        if (speed != 0.0f)
        {
            particle->velocity.x = (particle->velocity.x / speed) * speed;
            particle->velocity.y = (particle->velocity.y / speed) * speed;
            particle->velocity.z = (particle->velocity.z / speed) * speed;
        }

        Particle_SetMass(particle, 2.0f); // 2.0kg
        // particle->velocity     = glm::vec3{ 0.0f, 0.0f, -35.0f };
        particle->acceleration = glm::vec3{ 0.0f, -1.0f, 0.0f };
        particle->damping      = 0.99f;
        break;
    }
    case ARTILLERY:
    {
        Particle_SetMass(particle, 200.0f); // 200.0kg
        particle->velocity     = glm::vec3{ 0.0f, 30.0f, -40.0f };
        particle->acceleration = glm::vec3{ 0.0f, -20.0f, 0.0f };
        particle->damping      = 0.99f;
        break;
    }
    case GRENADE:
    {
        Particle_SetMass(particle, 1.0f); // 1.0kg
        particle->velocity = glm::vec3{ 0.0f, 3.0f, -1.0f };
        // particle->acceleration = glm::vec3{ 0.0f, -20.0f, 0.0f };
        particle->acceleration = GRAVITY;
        particle->damping      = 0.8f;

        break;
    }

        InvalidDefaultCase;
    }
}

internal void EntityAttack(EntityManager* manager, Entity* entity, Weapon* weapon, glm::vec3 dir)
{
    if (weapon->type == WeaponType_Hand)
    {
        Assert(entity->targetEntity);

        entity->targetEntity->velocity = { 0.0f, 0.0f, 0.0f };
        entity->targetEntity->velocity += dir * weapon->knockbackforce;
        entity->targetEntity->flags |= EntityFlag_InKnockback;
        // entity->targetEntity->health -= weapon->damage;
        entity->targetEntity->health -= weapon->damage * 3;

        if (entity->targetEntity->health <= 0)
        {
            // EntityDestroy(manager, entity->targetEntity, world);
            EntityRemove(manager, entity->targetEntity);
            entity->targetEntity = EntityGet(manager, 0);
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
                        EntityRemove(manager, targetEntity);
                        // EntityDestroy(manager, targetEntity, world);
                    }

                    // Note: break stops the projectile trajectory, this way projectile can only impact once.
                    // TODO: Decide if allow penetration
                    break;
                }
            }
        }
    }
}

// internal void PlayerUpdate(GameState* state, Entity* player, f32 delta, PlatformAPI* platform,
//                            GameInputController* controller, Mouse* mouse, glm::vec3 cameraOffset)
//{
//     Assert(player->type == EntityType_Player);

//    glm::uvec2 windowDim  = platform->WindowGetDimension();
//    Camera*    camera     = state->camera;
//    glm::mat4  projection = glm::perspective(Radians(45.0f), (f32)windowDim.x / (f32)windowDim.y, 0.1f, 100.0f);
//    glm::mat4  view       = CameraGetView(camera);

//    if (controller->isConnected)
//    {
//        if (controller->isAnalog)
//        {
//            if (ButtonIsPressed(controller->rightTrigger))
//            {
//                platform->AudioClipPlay(state->pistolShot, 0);
//            }
//            if (ButtonIsDown(controller->leftTrigger))
//            {
//                platform->Logf("Gamepad aiming");
//            }
//        }
//        else
//        {
//            glm::vec3 playerDirection{ 0.0f, 0.0f, 0.0f };
//            glm::vec3 crosshairPoint = WorldMousePicking(camera, windowDim, mouse->pos);

//            if (ButtonIsDown(controller->moveUp))
//            {
//                playerDirection.z = -1.0f;
//            }
//            if (ButtonIsDown(controller->moveDown))
//            {
//                playerDirection.z = 1.0f;
//            }
//            if (ButtonIsDown(controller->moveLeft))
//            {
//                playerDirection.x = -1.0f;
//            }
//            if (ButtonIsDown(controller->moveRight))
//            {
//                playerDirection.x = 1.0f;
//            }
//            playerDirection = SafeNorm(playerDirection);

//            if (ButtonIsPressed(mouse->left))
//            {
//                glm::vec3 dir = SafeNorm(crosshairPoint - player->position);
//                EntityAttack(state->entityManager, player, &gWeaponPistol, dir);
//            }

//            // Player rotation
//            {
//                glm::vec3 dir       = SafeNorm(crosshairPoint - player->position);
//                f32       targetYaw = -atan2f(dir.x, -dir.z);
//                f32       deltaYaw  = targetYaw - player->rotation.y;
//                deltaYaw            = fmodf(deltaYaw + Pi, 2.0f * Pi) - Pi; // Wrap to [-Pi, Pi]
//                player->rotation.y += deltaYaw * rotationSpeed;
//            }

//            // Deceleration
//            f32 playerSpeed = glm::length(player->velocity);
//            if (playerSpeed > 0.0f)
//            {
//                f32 decelerationStep = playerFrictionForce * delta;

//                if (playerSpeed <= decelerationStep)
//                {
//                    player->velocity = glm::vec3{ 0.0f, 0.0f, 0.0f };
//                    player->flags &= ~EntityFlag_InKnockback;
//                }
//                else
//                {
//                    if (player->flags & EntityFlag_InKnockback)
//                    {
//                        decelerationStep *= 2.0f;
//                    }

//                    player->velocity -= (player->velocity / playerSpeed) * decelerationStep;
//                }
//            }

//            // Acceleration
//            if (!(player->flags & EntityFlag_InKnockback))
//            {
//                glm::vec3 acceleration = playerDirection * playerMoveAcceleration;
//                player->velocity += acceleration * delta;
//                if (glm::length(player->velocity) > playerMaxSpeed)
//                {
//                    player->velocity = SafeNorm(player->velocity) * playerMaxSpeed;
//                }
//            }

//            glm::vec3 newPlayerPosition = player->position + (player->velocity * delta);
//            glm::vec3 correction{ 0.0f, 0.0f, 0.0f };
//            glm::vec3 totalCorrection{ 0 };

//            AABB playerWorldAABB = AABBToWorld(player->aabb, player->position);

//            // Collision detection
//            for (u32 entityIndex = 1; entityIndex < state->entityManager->entityCount; entityIndex++)
//            {
//                Entity* entity = EntityGet(state->entityManager, entityIndex);

//                AABB intersection;
//                if (EntitiesIntersect(player, entity, &intersection))
//                {
//                    if (player->flags & EntityFlag_InKnockback)
//                    {
//                        player->flags &= ~EntityFlag_InKnockback;
//                    }

//                    glm::vec3 penetration;
//                    penetration.x = intersection.max.x - intersection.min.x;
//                    penetration.y = intersection.max.y - intersection.min.y;
//                    penetration.z = intersection.max.z - intersection.min.z;

//                    // ----------------------------------------------------------------------------
//                    // Correct using the minimal penetration axis
//                    if (penetration.x < penetration.z)
//                    {
//                        correction.x = penetration.x;
//                    }
//                    else
//                    {
//                        correction.z = penetration.z;
//                    }
//                    // ----------------------------------------------------------------------------

//                    // ----------------------------------------------------------------------------
//                    // Determine correction axis
//                    if (newPlayerPosition.x < entity->position.x)
//                    {
//                        correction.x = -correction.x;
//                    }
//                    if (newPlayerPosition.z < entity->position.z)
//                    {
//                        correction.z = -correction.z;
//                    }
//                    // ----------------------------------------------------------------------------

//                    // ----------------------------------------------------------------------------
//                    // Use the greatest penetration to resolve collisions
//                    if (Abs(correction.x) > Abs(totalCorrection.x))
//                    {
//                        totalCorrection.x = correction.x;
//                    }
//                    if (Abs(correction.z) > Abs(totalCorrection.z))
//                    {
//                        totalCorrection.z = correction.z;
//                    }
//                    // ----------------------------------------------------------------------------
//                }
//            }

//            // World limit
//            // Note: Enemies might spawn away the limit.
//            // This logic does not affect enemies.
//            //
//            {
//                // Left limit
//                if (newPlayerPosition.x < GRID_LEFT_LIMIT)
//                {
//                    newPlayerPosition.x = GRID_LEFT_LIMIT;
//                }
//                // Right limit
//                if (newPlayerPosition.x > GRID_RIGHT_LIMIT)
//                {
//                    newPlayerPosition.x = GRID_RIGHT_LIMIT;
//                }
//                // Top limit
//                if (newPlayerPosition.z < GRID_TOP_LIMIT)
//                {
//                    newPlayerPosition.z = GRID_TOP_LIMIT;
//                }
//                // Bottom limitd
//                if (newPlayerPosition.z > GRID_BOTTOM_LIMIT)
//                {
//                    newPlayerPosition.z = GRID_BOTTOM_LIMIT;
//                }
//            }

//            newPlayerPosition += totalCorrection;
//            player->position = newPlayerPosition;
//            camera->position = player->position + cameraOffset;
//        }
//    }
//}

internal void UpdateEntity(EntityManager* manager, Entity* entity, PlatformAPI* platform,
                           GameInputController* controller, Camera* camera, f32 delta)
{
    // TODO: Parameters
    glm::uvec2 windowDim = platform->WindowGetDimension();
    Assets*    assets    = manager->assets;

    f32 maxSpeed          = 0.0f;
    f32 accelerationSpeed = 0.0f;
    f32 frictionForce     = 0.0f;

    // Newton's first law (law of inertia)
    // An object continues with a constant velocity unless a force acts upon it.
    // f32 damping = 0.972f;
    f32 damping = 0.972f;

    switch (entity->type)
    {
    case EntityType_Player:
    {
        maxSpeed          = playerMaxSpeed;
        accelerationSpeed = playerMoveAcceleration;
        frictionForce     = playerFrictionForce;

        entity->direction = { 0.0f, 0.0f, 0.0f };

        if (controller->type == ControllerType_Keyboard)
        {
            if (ButtonIsDown(controller->moveUp))
            {
                entity->direction.z = -1.0f;
            }
            if (ButtonIsDown(controller->moveDown))
            {
                entity->direction.z = 1.0f;
            }
            if (ButtonIsDown(controller->moveLeft))
            {
                entity->direction.x = -1.0f;
            }
            if (ButtonIsDown(controller->moveRight))
            {
                entity->direction.x = 1.0f;
            }
            entity->direction = SafeNorm(entity->direction);

            // Player rotation
            {
                glm::vec3 crosshairPoint = WorldMousePicking(camera, windowDim, controller->mouse.pos);
                glm::vec3 dir            = SafeNorm(crosshairPoint - entity->position);
                f32       targetYaw      = -atan2f(dir.x, -dir.z);
                f32       deltaYaw       = targetYaw - entity->rotation.y;
                deltaYaw                 = fmodf(deltaYaw + Pi, 2.0f * Pi) - Pi; // Wrap to [-Pi, Pi]
                entity->rotation.y += deltaYaw * rotationSpeed;
            }
        }
        // Gamepad
        else
        {
        }
        break;
    }
    case EntityType_Enemy:
    {
        Assert(entity->targetEntity);

        maxSpeed          = enemyMaxSpeed;
        accelerationSpeed = enemyAcceleration;
        frictionForce     = playerFrictionForce;

        entity->direction = SafeNorm(entity->targetEntity->position - entity->position);
        // entity->direction = { 0.0f, 0.0f, 0.0f };

        // Enemy rotation
        {
            glm::vec3 direction = entity->targetEntity->position - entity->position;
            entity->rotation.y  = atan2(direction.x, direction.z);
        }

        break;
    }
        InvalidDefaultCase;
    }

    // platform->Logf("Direction: %.2f %.2f %.2f", entity->direction.x, entity->direction.y, entity->direction.z);

    // Update linear position
    entity->position += (entity->velocity * delta);

    // Acceleration
    glm::vec3 acceleration = entity->direction * accelerationSpeed;

    // Update linear velocity from acceleration
    entity->velocity += acceleration;
    f32 speed = glm::length(entity->velocity);
    if (speed > maxSpeed)
    {
        entity->velocity = SafeNorm(entity->velocity) * maxSpeed;
    }

    // Drag
    entity->velocity *= damping;

#if 0
    // Acceleration
    {
        glm::vec3 acceleration = entity->direction * accelerationSpeed;
        entity->velocity += acceleration * delta;
        f32 speed = glm::length(entity->velocity);
        if (speed > maxSpeed)
        {
            entity->velocity = SafeNorm(entity->velocity) * maxSpeed;
        }
    }

    entity->velocity *= damping;

    entity->position += (entity->velocity * delta);

    // TODO: Collision detection
    {
        //
    }
#endif

    if (entity->type == EntityType_Player)
    {
        glm::vec3 cameraOffset{ 0.0f, 18.0f, 13.0f };
        camera->position = entity->position + cameraOffset;
    }

    // Animation
    {
        if (entity->type == EntityType_Enemy)
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

// internal void EnemyUpdate(PlatformAPI* platform, EntityManager* manager, World* world, Entity* entity, f32 delta)
//{
//     Assets* assets = manager->assets;
//     Entity* player = EntityGet(manager, 0);

//    Assert(entity->type == EntityType_Enemy);
//    // TODO: Can be used for destroy fences
//    Assert(entity->targetEntity);

//    if (entity->flags & EntityFlag_Climbing)
//    {
//        entity->rotation.x = Radians(-90.0f);
//    }

//    // Animation
//    {
//        // Update current animation
//        if (entity->animation.current)
//        {
//            Skeleton*  skeleton  = entity->skeleton;
//            Animation* animation = entity->animation.current;

//            f32 prevTime    = entity->animation.time;
//            f32 currentTime = 0.0f;
//            entity->animation.time += delta;
//            currentTime = entity->animation.time;

//            if (animation->id == Anim_ZombieMaleAttackLeft)
//            {
//                if (prevTime < 0.74f && currentTime >= 0.74f)
//                {
//                    entity->flags |= EntityFlag_Hitting;
//                }
//                if (prevTime < 0.91f && currentTime >= 0.91f)
//                {
//                    platform->Logf("End");
//                    entity->flags &= ~EntityFlag_Hitting;
//                }
//            }

//            // End of the animation
//            if (entity->animation.time >= animation->duration)
//            {
//                entity->animation.time    = 0.0f;
//                entity->animation.current = 0;
//            }

//            SkeletonApplyAnimation(skeleton, animation, entity->animation.time);
//            SkeletonUpdatePose(skeleton);
//        }

//        // Attack animation
//        // TODO: Randomize attack animation
//        if (!entity->animation.current)
//        {
//            if (entity->flags & EntityFlag_InAttack)
//            {
//                u32 attackAnimationId = 0;

//                if (entity->assetID == Model_ZombieMaleA)
//                {
//                    attackAnimationId = Anim_ZombieMaleAttackLeft;
//                }
//                else if (entity->assetID == Model_ZombieFemaleA)
//                {
//                    attackAnimationId = Anim_ZombieFemaleAttackLeft;
//                }
//                else
//                {
//                    InvalidCodePath;
//                }

//                entity->animation.current = AssetsAnimationGet(assets, attackAnimationId);
//                entity->animation.time    = 0.0f;
//            }
//            else if (entity->flags & EntityFlag_Idle)
//            {
//                u32 idleAnimationId = 0;
//                if (entity->assetID == Model_ZombieMaleA)
//                {
//                    idleAnimationId = Anim_ZombieMaleIdleAlert;
//                }
//                else if (entity->assetID == Model_ZombieFemaleA)
//                {
//                    idleAnimationId = Anim_ZombieFemaleIdleAlert;
//                }
//                else
//                {
//                    InvalidCodePath;
//                }

//                entity->animation.current = AssetsAnimationGet(assets, idleAnimationId);
//                entity->animation.time    = 0.0f;
//            }
//            else if (entity->flags & EntityFlag_Climbing)
//            {
//                u32 climbAnimationId = 0;
//                if (entity->assetID == Model_ZombieMaleA)
//                {
//                    climbAnimationId = Anim_ZombieMaleCrawlingForward;
//                }
//                else if (entity->assetID == Model_ZombieFemaleA)
//                {
//                    climbAnimationId = Anim_ZombieFemaleCrawlingForward;
//                }
//                else
//                {
//                    InvalidCodePath;
//                }

//                entity->animation.current = AssetsAnimationGet(assets, climbAnimationId);
//                entity->animation.time    = 0.0f;
//            }
//        }
//    }

//    // Attack
//    {
//        if (!entity->targetEntity)
//        {
//            entity->targetEntity = player;
//        }

//        if (!(entity->flags & EntityFlag_Climbing))
//        {

//            glm::vec3 lookAt{ sinf(entity->rotation.y), 0.0f, cosf(entity->rotation.y) };
//            lookAt = SafeNorm(lookAt);
//            glm::vec3 start{ entity->position.x, 0.5f, entity->position.z };
//            glm::vec3 end = start + (lookAt * enemyStrikingRange);

//            AABB worldAABB = AABBToWorld(entity->targetEntity->aabb, entity->targetEntity->position);
//            if (AABBSegmentIntersection(worldAABB, start, end))
//            {
//                entity->flags |= EntityFlag_InAttack;

//                if (entity->flags & EntityFlag_Hitting)
//                {
//                    EntityAttack(manager, entity, &gWeaponHand, entity->velocity);
//                    entity->flags &= ~EntityFlag_Hitting;
//                }
//            }
//            else
//            {
//                entity->flags &= ~EntityFlag_InAttack;
//            }
//        }
//    }

//    //----------------------------------------------------------------------------
//    // Path finding
//    glm::vec3 entityDir{ 0.0f, 0.0f, 0.0f };

//    if (entity->flags & EntityFlag_Climbing)
//    {
//        entityDir.y = 1.0f;
//    }
//    else if (player == entity->targetEntity)
//    {
//        cell_index              enemyCellIndex  = WorldPositionToGridCell(entity->position);
//        cell_index              playerCellIndex = WorldPositionToGridCell(entity->targetEntity->position);
//        std::vector<cell_index> path            = WorldFindBestPath(world, manager, enemyCellIndex, playerCellIndex);
//        // glm::vec3               entityDir{ 0.0f, 0.0f, 0.0f };

//        if (!path.empty())
//        {
//            glm::vec3 targetPosition = WorldGridCellToPosition(path[path.size() - 2]);
//            entityDir                = SafeNorm(targetPosition - entity->position);
//            entityDir.y              = 0.0f;
//            //  entity->yaw       = (f32)atan2(entityDir.x, entityDir.z);
//        }
//        // TODO: Break obstacles
//        else
//        {
//            entityDir = SafeNorm(entity->targetEntity->position - entity->position);
//            //   entityDir.y = 0.0f;
//        }
//    }
//    //----------------------------------------------------------------------------

//    //----------------------------------------------------------------------------
//    // Entity acceleration
//    if (!(entity->flags & EntityFlag_InKnockback))
//    {
//        f32 accel = enemyAcceleration;
//        if (entity->flags & EntityFlag_Climbing)
//        {
//            accel = 0.1f;
//        }

//        glm::vec3 acceleration = entityDir * enemyAcceleration;
//        entity->velocity += acceleration * delta;
//        // TODO: Use constant speed for enemies???
//        if (glm::length(entity->velocity) > enemyMaxSpeed)
//        {
//            entity->velocity = SafeNorm(entity->velocity) * enemyMaxSpeed;
//        }
//    }
//    else
//    {
//        // Friction force
//        entity->velocity *= 0.70f;

//        f32 speed = glm::length(entity->velocity);
//        if (speed <= 0.01f)
//        {
//            entity->velocity = { 0, 0, 0 };
//            entity->flags &= ~EntityFlag_InKnockback;
//        }

//        entity->position += entity->velocity * delta;
//    }

//    glm::vec3 newEntityPosition = entity->position + (entity->velocity * delta);
//    //----------------------------------------------------------------------------

//    // -----------------------------------------------------------
//    // Collision detection
//    glm::vec3 correction{ 0.0f, 0.0f, 0.0f };
//    glm::vec3 totalCorrection{ 0 };

//    if (!(entity->flags & EntityFlag_Climbing))
//    {
//        // Attack logic
//        // Note: this is not part of collision detection
//        for (u32 entityIndex = 0; entityIndex < manager->entityCount; entityIndex++)
//        {
//            Entity* entityPtr = EntityGet(manager, entityIndex);
//            if (entityPtr->type == EntityType_Player)
//            {
//                glm::vec3 lookAt{ sinf(entity->rotation.y), 0.0f, cosf(entity->rotation.y) };
//                lookAt              = SafeNorm(lookAt);
//                glm::vec3 start     = entity->position;
//                start.y             = 0.5f;
//                glm::vec3 end       = start + (lookAt * enemyStrikingRange);
//                AABB      worldAABB = AABBToWorld(entityPtr->aabb, entityPtr->position);

//                if (AABBSegmentIntersection(worldAABB, start, end))
//                {
//                    newEntityPosition = entity->position;

//                    if (!(entity->flags & EntityFlag_InAttack))
//                    {
//                        entity->targetEntity = entityPtr;
//                    }
//                }
//            }
//            else if (entityPtr->type == EntityType_Obstacle)
//            {
//                glm::vec3 lookAt{ sinf(entity->rotation.y), 0.0f, cosf(entity->rotation.y) };
//                lookAt              = SafeNorm(lookAt);
//                glm::vec3 start     = entity->position;
//                start.y             = 0.5f;
//                glm::vec3 end       = start + (lookAt * enemyJumpRange);
//                AABB      worldAABB = AABBToWorld(entityPtr->aabb, entityPtr->position);

//                if (AABBSegmentIntersection(worldAABB, start, end))
//                {
//                    newEntityPosition = entity->position;
//                    // entity->flags |= EntityFlag_Idle;
//                    entity->flags |= EntityFlag_Climbing;
//                }
//            }
//        }
//    }

//    newEntityPosition += totalCorrection;
//    entity->position = newEntityPosition;
//}

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
        state->camera         = PushStruct(arena, Camera);
        state->planeBuffer    = PushStruct(arena, GPUBuffer);
        state->cubeBuffer     = PushStruct(arena, GPUBuffer);
        state->entityManager  = PushStruct(arena, EntityManager);
        state->renderer       = PushStruct(arena, Renderer);
        state->assets         = PushStruct(arena, Assets);
        state->ui             = PushStruct(arena, UI);
        state->mode           = GameMode_Play;
#ifdef BUILD_TYPE_DEBUG
        state->debug                    = PushStruct(arena, Debug);
        state->debug->state             = state;
        state->debug->selectedCellIndex = CELL_EMPTY;
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

        CameraInit(state->camera,          //
                   { 0.0f, 16.0f, 5.0f },  // Position
                   { 0.0f, -0.9f, -0.4f }, // Target
                   { 0.0f, 1.0f, 0.0f },   // Up
                   -68.0f,                 // Pitch
                   -90.0f,                 // Yaw
                   55.0f                   // Fov
        );

        state->pistolShot      = platform->AudioClipLoad("../data/pistol.wav", AudioClipType_Sfx);
        state->backgroundMusic = platform->AudioClipLoad("../data/background.wav", AudioClipType_Music);

        platform->AudioSetVolume(-35.0f, AudioClipType_Music);
        platform->AudioSetVolume(-3.0f, AudioClipType_Sfx);
        // platform->AudioClipPlay(state->backgroundMusic, AudioClipPlayFlag_Loop);

        LoadAssets(assets);

        Entity* player = EntitySpawn(state->entityManager, EntityType_Player, { 0.0f, 0.0f, 0.0f });
        // Entity* enemy       = EntitySpawn(state->entityManager, EntityType_Enemy, { 0.0f, 0.0f, -15.0f });
        // enemy->targetEntity = player;

#if 0
        EntityManager* manager    = state->entityManager;
        Model*         fenceModel = AssetsModelGet(assets, Model_Fence);

        f32 fenceWidth = fenceModel->aabb.max.x - fenceModel->aabb.min.x;
        f32 fenceDepth = fenceModel->aabb.max.z - fenceModel->aabb.min.z;

        u32 fenceSideCount = 5;

        f32 startX = (fenceWidth * fenceSideCount * -0.5f) + fenceWidth * 0.5f;
        f32 startZ = (fenceWidth * fenceSideCount * -0.5f) + fenceWidth * 0.5f;

        // Top
        f32 x = startX;
        f32 z = startZ;
        for (u32 i = 0; i < fenceSideCount; i++)
        {
            EntitySpawn(manager, EntityType_Obstacle, { x, 0.0f, z });
            x += fenceWidth;
        }

        // Left
        x -= fenceWidth * 0.5f;
        z = startZ + fenceWidth * 0.5f;
        for (u32 i = 0; i < fenceSideCount; i++)
        {
            Entity* fence = EntitySpawn(manager, EntityType_Obstacle, { x, 0.0f, z });
            BuildRotateObstacle(fence, true);
            z += fenceWidth;
        }

        // Right
        x = startX - fenceWidth * 0.5f;
        z = startZ + fenceWidth * 0.5f;
        for (u32 i = 0; i < fenceSideCount; i++)
        {
            Entity* fence = EntitySpawn(manager, EntityType_Obstacle, { x, 0.0f, z });
            BuildRotateObstacle(fence, true);
            z += fenceWidth;
        }

        // Bottom
        x = startX;
        z -= fenceWidth * 0.5f;
        for (u32 i = 0; i < fenceSideCount; i++)
        {
            EntitySpawn(manager, EntityType_Obstacle, { x, 0.0f, z });
            x += fenceWidth;
        }
#endif

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
    }
    // ----------------------------------------------------------------------------

    // ----------------------------------------------------------------------------
    // Update
    glm::uvec2     windowDim     = platform->WindowGetDimension();
    EntityManager* entityManager = state->entityManager;
    Entity*        player        = EntityGet(entityManager, 0);
    Camera*        camera        = state->camera;
    Assets*        assets        = state->assets;
    UI*            ui            = state->ui;

    // xxx
    camera->target = player->position;

    // TODO: Assign controller to player
    // for (u32 controllerIndex = 0; controllerIndex < ArrayCount(input->controllers); controllerIndex++)
    //{
    //    GameInputController* controller = GetController(input, controllerIndex);
    //}

    GameInputController* controller = GetController(input, CONTROLLER_KEYBOARD);

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
        if (ButtonIsPressed(controller->start))
        {
            // TODO: game mode transitions
            state->mode = GameMode_Pause;
        }

        for (u32 entityIndex = 0; entityIndex < entityManager->entityCount; entityIndex++)
        {
            Entity* entity = EntityGet(entityManager, entityIndex);
            UpdateEntity(entityManager, entity, platform, controller, camera, delta);
        }

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
    glm::mat4 projection = CameraGetProjection(camera, (f32)windowDim.x / (f32)windowDim.y);
    glm::mat4 view       = CameraGetView(camera);
    glm::mat4 viewProj   = projection * view;
    Renderer* renderer   = state->renderer;
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
        Texture* zombieTexture  = AssetsTextureGet(assets, Texture_Zombie);
        Texture* fenceTexture   = AssetsTextureGet(assets, Texture_Fence);

        // 2D
        {
            glm::vec2 crosshairSpriteSize{ 128.0f, 128.0f };
            glm::vec2 cursorSize{ 32.0f, 32.0f };

            // Draw scope
            DrawRect(renderer, { (f32)mouse->pos.x, (f32)mouse->pos.y }, cursorSize, crosshairAtlas,
                     { 2074.0f, 142.0f }, crosshairSpriteSize);
        }

        // 3D
        {
            gl->ActiveTexture(GL_TEXTURE0);
            gl->BindTexture(GL_TEXTURE_2D, fenceTexture->id);
            gl->ActiveTexture(GL_TEXTURE1);
            gl->BindTexture(GL_TEXTURE_2D, zombieTexture->id);

            // Floor
            {
                glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, { 0.0f, 0.0f, 0.0f });
                glm::mat4 scale     = glm::scale(glm::mat4{ 1.0f }, glm::vec3{ 50.0f });
                glm::mat4 model     = translate * scale;

                PushRenderProgramUse(renderer, state->program->id);
                PushRenderUploadUniformInt(renderer, state->program->id, "hasDiffuse", 0);
                PushRenderUploadUniformMat4x4(renderer, state->program->id, "mvp", viewProj * model);
                PushRenderUploadUniformVec4(renderer, state->program->id, "color", { 1.0f, 1.0f, 1.0f, 0.5f });
                PushRenderDrawBuffer(renderer, state->planeBuffer);
            }

            // Shoot
            {
                if (ammoRound->type != UNKNOWN)
                {
                    glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, ammoRound->particle.position);
                    glm::mat4 scale     = glm::scale(glm::mat4{ 1.0f }, glm::vec3{ 0.15f });
                    glm::mat4 model     = translate * scale;

                    PushRenderUploadUniformMat4x4(renderer, state->program->id, "mvp", viewProj * model);
                    PushRenderUploadUniformVec4(renderer, state->program->id, "color", color_red);
                    PushRenderDrawBuffer(renderer, state->cubeBuffer);
                }
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
                        glm::vec4 tintColor = color_white;

                        if (entity->flags & (EntityFlag_InvalidPosition))
                        {
                            tintColor = color_red;
                        }
                        else if (entity->flags & (EntityFlag_Positioning | EntityFlag_Snapping))
                        {
                            tintColor = color_green;
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
                        PushRenderUploadUniformVec4(renderer, state->program->id, "color", color_green);
                        PushRenderDrawBuffer(renderer, entityModel->gpuBuffer);
                    }
                    else if (entity->type == EntityType_Enemy)
                    {
                        PushRenderProgramUse(renderer, state->programSkinned->id);
                        PushRenderUploadUniformInt(renderer, state->programSkinned->id, "diffuseMap", 1);

                        Skeleton* skeleton = entity->skeleton;

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

                        if (optionIndex == 0)
                        {
                            // TODO: game mode transitions
                            state->mode = GameMode_Play;
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
    return 0;
}