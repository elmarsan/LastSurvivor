#pragma once

struct GameState;

struct Debug
{
    GameState* state;
    cell_index selectedCellIndex;
};

void DebugDraw(Debug* debug, GameInput* input, PlatformAPI* platform, glm::mat4 viewProj);