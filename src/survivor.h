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
#include "survivor_build.h"
#include "survivor_weapon.h"
#include "survivor_debug.h"

global_variable glm::vec4 green{ 0.2f, 1.0f, 0.0f, 1.0f };
global_variable glm::vec4 red{ 1.0f, 0.0f, 0.0f, 1.0f };
global_variable glm::vec4 blue{ 0.2f, 0.4f, 1.0f, 1.0f };
global_variable glm::vec4 white{ 1.0f, 1.0f, 1.0f, 1.0f };
global_variable glm::vec4 black{ 0.0f, 0.0f, 0.0f, 1.0f };
global_variable glm::vec4 magenta{ 1.0f, 0.0f, 1.0f, 1.0f };
global_variable glm::vec4 yellow{ 1.0f, 1.0f, 0.0f, 1.0f };
global_variable glm::vec4 orange{ 0.87f, 0.39f, 0.04f, 1.0f };

enum GameMode
{
    GameMode_Pause,
    GameMode_Build,
    GameMode_GameOver,
    GameMode_Round
};

struct GameState
{
    b32   initialized;
    Arena arena;

    Assets*        assets;
    GPUBuffer*     planeBuffer;
    AudioClip*     pistolShot;
    AudioClip*     backgroundMusic;
    Program*       program;
    Program*       programSkinned;
    Camera*        camera;
    EntityManager* entityManager;
    GameMode       mode;
    World*         world;
    Renderer*      renderer;

    // Round mode
    u32    roundEnemyCount;
    u32    roundMaxEnemy;
    u32    roundCount;
    time_t roundLastSpawnTime;
    f64    roundSpawnIntervalSec;

    // Build mode
    Entity* buildObstacle;
    f64     buildModeDurationSec;
    time_t  buildModeBeginTime;

#ifdef BUILD_TYPE_DEBUG
    Debug* debug;
#endif
};