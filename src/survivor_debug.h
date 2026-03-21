#pragma once

struct GameState;

struct Debug
{
    GameState* state;
    cell_index selectedCellIndex;
};

void DebugDraw(Debug* debug, GameInput* input, PlatformAPI* platform, mat4x4 viewProj);