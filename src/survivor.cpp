#include "survivor.h"

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    memory->opengl.glClear(GL_COLOR_BUFFER_BIT);
    memory->opengl.glClearColor(0.0f, 1.0f, 1.0f, 1.0f);

    return 0;
}