#pragma once

#include "survivor_types.h"
#include "survivor_memory.h"
#include "survivor_string.h"
#include "survivor_math.h"
#include "survivor_physics.h"
#include "survivor_opengl.h"
#include "survivor_platform.h"
#include "survivor_renderer_opengl.h"
#include "survivor_camera.h"
#include "survivor_assets.h"
#include "survivor_world.h"
#include "survivor_entity.h"
// #include "survivor_build.h"
#include "survivor_weapon.h"
#include "survivor_ui.h"
#if BUILD_TYPE_DEBUG
#include "survivor_debug.h"
#endif

enum GameMode
{
    GameMode_Pause,
    GameMode_GameOver,
    GameMode_Play
};

#define GRAVITY glm::vec3{ 0.0f, -9.81f, 0.0f }

struct Particle
{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 forceAccum;
    glm::vec3 acceleration;
    f32       damping;

    // TODO: Review inverse mass concept
    //
    /**
     * Holds the inverse of the mass of the particle. It
     * is more useful to hold the inverse mass because
     * integration is simpler, and because in real time
     * simulation it is more useful to have objects with
     * infinite mass (immovable) than zero mass
     * (completely unstable in numerical simulation).
     */
    f32 inverseMass;
};

// TODO: Integrate concept
void Particle_Integrate(Particle* particle, f32 delta)
{
    // We don't integrate things with zero mass.
    if (particle->inverseMass <= 0.0f)
    {
        return;
    }

    // Update linear position
    particle->position += particle->velocity * delta;

    // Work out the acceleration from the force
    glm::vec3 resultingAcc = particle->acceleration;
    resultingAcc += particle->forceAccum * particle->inverseMass;

    // Update linear velocity from acceleration
    particle->velocity += resultingAcc * delta;

    // Impose drag
    // TODO: Review alternative equation 3.5
    // Note: First law trick
    particle->velocity *= pow(particle->damping, delta);

    // Clear accumulator
    particle->forceAccum = glm::vec3{ 0.0f, 0.0f, 0.0f };
}

void Particle_SetMass(Particle* particle, f32 mass)
{
    Assert(mass != 0);
    particle->inverseMass = (1.0f) / mass;
}

enum AmmoRoundType
{
    UNKNOWN,
    PISTOL,
    ARTILLERY,
    GRENADE
};

struct AmmoRound
{
    Particle      particle;
    AmmoRoundType type = UNKNOWN;
};

struct GameState
{
    b32   initialized;
    Arena arena;

    Assets*        assets;
    GPUBuffer*     planeBuffer;
    GPUBuffer*     cubeBuffer;
    AudioClip*     pistolShot;
    AudioClip*     backgroundMusic;
    Program*       program;
    Program*       programSkinned;
    Camera*        camera;
    EntityManager* entityManager;
    GameMode       mode;
    Renderer*      renderer;
    UI*            ui;
    AmmoRound      ammoRound;

#ifdef BUILD_TYPE_DEBUG
    Debug* debug;
#endif
};