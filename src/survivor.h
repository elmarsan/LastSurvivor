#pragma once

#include "survivor_types.h"
#include "survivor_memory.h"
#include "survivor_string.h"
#include "survivor_math.h"
#include "survivor_physics.h"
#include "survivor_opengl.h"
#include "survivor_platform.h"
#include "survivor_renderer_opengl.h"
#include "survivor_assets.h"
#include "survivor_entity.h"
#include "survivor_weapon.h"
#include "survivor_ui.h"
#if BUILD_TYPE_DEBUG
#include "survivor_debug.h"
#endif

// TODO: Remove grid macros
#define GRID_COLS         30
#define GRID_ROWS         30
#define GRID_CELLS        (GRID_COLS * GRID_ROWS)
#define GRID_RIGHT_LIMIT  (GRID_COLS * 0.5f)
#define GRID_LEFT_LIMIT   (-GRID_RIGHT_LIMIT)
#define GRID_BOTTOM_LIMIT (GRID_COLS * 0.5f)
#define GRID_TOP_LIMIT    (-GRID_BOTTOM_LIMIT)
#define GRID_MAX_ROW      (GRID_ROWS - 1)
#define GRID_MIN_ROW      0
#define GRID_MAX_COL      (GRID_COLS - 1)
#define GRID_MIN_COL      0

#define CELL_SIZE            1.0f
#define CELL_HALF            (CELL_SIZE * 0.5f)
#define CELL_ROW(index)      (index / GRID_ROWS)
#define CELL_COL(index)      (index % GRID_COLS)
#define CELL_INDEX(row, col) (col + ((row) * GRID_ROWS))
#define CELL_EMPTY           0xFFFFFFFF

typedef u32 cell_index;

enum GameMode
{
    GameMode_Pause,
    GameMode_GameOver,
    GameMode_Play
};

// #define GRAVITY glm::vec3{ 0.0f, -9.81f, 0.0f }

// struct Particle
//{
//     glm::vec3 position;
//     glm::vec3 velocity;
//     glm::vec3 forceAccum;
//     glm::vec3 acceleration;
//     f32       damping;

//    // TODO: Review inverse mass concept
//    //
//    /**
//     * Holds the inverse of the mass of the particle. It
//     * is more useful to hold the inverse mass because
//     * integration is simpler, and because in real time
//     * simulation it is more useful to have objects with
//     * infinite mass (immovable) than zero mass
//     * (completely unstable in numerical simulation).
//     */
//    f32 inverseMass;
//};

//// TODO: Integrate concept
// void Particle_Integrate(Particle* particle, f32 delta)
//{
//     // We don't integrate things with zero mass.
//     if (particle->inverseMass <= 0.0f)
//     {
//         return;
//     }

//    // Update linear position
//    particle->position += particle->velocity * delta;

//    // Work out the acceleration from the force
//    glm::vec3 resultingAcc = particle->acceleration;
//    resultingAcc += particle->forceAccum * particle->inverseMass;

//    // Update linear velocity from acceleration
//    particle->velocity += resultingAcc * delta;

//    // Impose drag
//    // TODO: Review alternative equation 3.5
//    // Note: First law trick
//    particle->velocity *= pow(particle->damping, delta);

//    // Clear accumulator
//    particle->forceAccum = glm::vec3{ 0.0f, 0.0f, 0.0f };
//}

// void Particle_SetMass(Particle* particle, f32 mass)
//{
//     Assert(mass != 0);
//     particle->inverseMass = (1.0f) / mass;
// }

// enum AmmoRoundType
//{
//     UNKNOWN,
//     PISTOL,
//     ARTILLERY,
//     GRENADE
// };

// struct AmmoRound
//{
//     Particle      particle;
//     AmmoRoundType type = UNKNOWN;
// };

struct GameState
{
    b32   initialized;
    Arena arena;

    Assets*        assets;
    GPUBuffer*     cubeBuffer;
    Program*       program;
    Program*       programSkinned;
    EntityManager* entityManager;
    GameMode       mode;
    Renderer*      renderer;
    UI*            ui;
    // AmmoRound      ammoRound;
    // TODO: Move sprite and audio clips to assets
    Sprite2D*  pistolSprite;
    Sprite2D*  shotgunSprite;
    AudioClip* pistolShot;
    AudioClip* backgroundMusic;

#ifdef BUILD_TYPE_DEBUG
    Debug* debug;
#endif
};