#pragma once

#define DEBUG_COORDINATES 1
// #define DEBUG_AABB        0

struct GameState;

struct Debug
{
    GameState* state;
};

void DebugUpdateAndRender(Debug* debug, GameInput* input, PlatformAPI* platform, glm::mat4 viewProj);

// TODO: Mode to build mode
void BuildExit(GameState* state);