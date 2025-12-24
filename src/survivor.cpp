#include "survivor.h"

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    PlatformAPI platform = memory->platform;

    if (input->keyboard.moveUp.isDown)
    {
        platform.Logf("Keyboard move up down");
    }
    else if (!input->keyboard.moveUp.isDown && input->keyboard.moveUp.wasDown)
    {
        platform.Logf("Keyboard move down released");
    }
    if (input->mouse.left.isDown)
    {
        memory->opengl.glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    }
    if (input->mouse.right.isDown)
    {
        memory->opengl.glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
    }
    if (input->mouse.middle.isDown)
    {
        platform.Logf("Mouse middle down");
    }
    if (input->gamepad.isConnected)
    {
        GameInputController* gamepad = &input->gamepad;

        if (gamepad->start.wasDown && !gamepad->start.isDown)
        {
            platform.Logf("Gamepad start released");
        }
        if (gamepad->moveUp.isDown)
        {
            platform.Logf("Dpad up down");
        }
        if (gamepad->rightTrigger.isDown)
        {
            platform.Logf("Gamepad shoting");
        }
        if (gamepad->leftTrigger.isDown)
        {
            platform.Logf("Gamepad aiming");
        }
        // memory->platform.Logf("Left %.2f %.2f     Right %.2f %.2f", gamepad->stickLeft.x, gamepad->stickLeft.y,
        //                       gamepad->stickRight.x, gamepad->stickRight.y);
    }

    memory->opengl.glClear(GL_COLOR_BUFFER_BIT);

    return 0;
}