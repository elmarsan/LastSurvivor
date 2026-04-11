#pragma once

#define DEBUG_COORDINATES 0

struct GameState;

struct Debug
{
    GameState* state;
    cell_index selectedCellIndex;
};

void DebugUpdateAndRender(Debug* debug, GameInput* input, PlatformAPI* platform, glm::mat4 viewProj);

// TODO: Mode to build mode
void BuildExit(GameState* state);