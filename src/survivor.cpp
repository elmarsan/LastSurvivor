#include "survivor.h"

#include "survivor_renderer_opengl.cpp"
#include "survivor_debug_geometry.cpp"
#include "survivor_debug.cpp"

struct Vertex
{
    v3 position;
    v3 normal;
    v2 uv;
};

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(GameState) <= memory->permanentStorageSize);

    GameState*  state    = (GameState*)memory->permanentStorage;
    PlatformAPI platform = memory->platform;
    OpenGL*     opengl   = &memory->opengl;
    Arena*      arena    = &state->arena;

    v3 green{ 0.0f, 1.0f, 0.0f };
    v3 red{ 1.0f, 0.0f, 0.0f };
    v3 blue{ 0.2f, 0.4f, 1.0f };
    v3 white{ 1.0f, 1.0f, 1.0f };

    // ----------------------------------------------------------------------------
    // Init
    if (!state->initialized)
    {
        platform.Logf("Initializing game state...");
        state->initialized = true;

        ArenaInit(arena, (size_t)memory->permanentStorageSize - sizeof(GameState),
                  (u8*)memory->permanentStorage + sizeof(GameState));

        state->program    = PushStruct(arena, Program);
        state->camera     = PushStruct(arena, Camera);
        state->player     = PushStruct(arena, Player);
        state->cubeBuffer = PushStruct(arena, GeometryBuffer);
#ifdef BUILD_TYPE_DEBUG
        state->debug   = PushStruct(arena, DebugState);
        void* permMem  = PushBlock(arena, Kilobytes(64));
        void* frameMem = PushBlock(arena, Kilobytes(128));

        ArenaInit(&state->debug->arena, Kilobytes(64), permMem);
        ArenaInit(&state->debug->frameArena, Kilobytes(128), frameMem);

        DebugInit(state->debug, opengl, &platform);
#endif
        FileReadResult vertexSourceFile   = platform.FileReadEntire("../src/shaders/basic.vert");
        FileReadResult fragmentSourceFile = platform.FileReadEntire("../src/shaders/basic.frag");

        ProgramInit(opengl, state->program);
        ProgramAttachShader(opengl, state->program, (const char*)vertexSourceFile.content, vertexSourceFile.contentSize,
                            GL_VERTEX_SHADER);
        ProgramAttachShader(opengl, state->program, (const char*)fragmentSourceFile.content,
                            fragmentSourceFile.contentSize, GL_FRAGMENT_SHADER);
        ProgramBuild(opengl, state->program);

        platform.FileFree(vertexSourceFile.content);
        platform.FileFree(fragmentSourceFile.content);

        GeometryBufferInit(opengl, state->cubeBuffer, GL_TRIANGLES);
        GeometryBufferVBOAlloc(opengl, state->cubeBuffer, cubeVertexs, sizeof(cubeVertexs), sizeof(Vertex),
                               GL_STATIC_DRAW);
        GeometryBufferVertexAttrib(opengl, state->cubeBuffer, 0, 3, GL_FLOAT, sizeof(Vertex),
                                   offsetof(Vertex, position));
        GeometryBufferVertexAttrib(opengl, state->cubeBuffer, 1, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, normal));
        GeometryBufferVertexAttrib(opengl, state->cubeBuffer, 2, 2, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, uv));

        v3  position = { 0.0f, 16.0f, 5.0f };
        v3  target   = { 0.0f, -0.9f, -0.4f };
        v3  up       = { 0.0f, 1.0f, 0.0f };
        f32 pitch    = -68.0f;
        f32 yaw      = -90.0f;

        CameraInit(state->camera, position, target, up, pitch, yaw, 45.0f);

        state->player->position = { 0.0f, 0.0f, 0.0f };
        state->player->target   = { 0.0f, 0.0f, -1.0f };

        state->pistolShot      = platform.AudioClipLoad("../data/pistol.wav", AudioClipType_Sfx);
        state->backgroundMusic = platform.AudioClipLoad("../data/background.wav", AudioClipType_Music);

        platform.AudioSetVolume(-35.0f, AudioClipType_Music);
        platform.AudioSetVolume(-3.0f, AudioClipType_Sfx);
        platform.AudioClipPlay(state->backgroundMusic, AudioClipPlayFlag_Loop);
    }
    // ----------------------------------------------------------------------------

    // ----------------------------------------------------------------------------
    // Update
    Player* player = state->player;
    Camera* camera = state->camera;

    if (input->keyboard.moveUp.isDown)
    {
        // platform.Logf("Keyboard move up down");
        // CameraMoveForward(camera, delta);

        f32 velocity = 1.8f * delta;
        player->position += player->target * velocity;
    }
    if (input->keyboard.moveDown.isDown)
    {
        // CameraMoveBackward(camera, delta);

        f32 velocity = 1.8f * delta;
        player->position -= player->target * velocity;
    }
    if (input->keyboard.moveLeft.isDown)
    {
        // CameraMoveLeft(camera, delta);

        f32 velocity = 1.8f * delta;
        v3  right    = Norm(Cross(player->target, { 0.0f, 1.0f, 0.0f }));
        player->position += right * velocity;
    }
    if (input->keyboard.moveRight.isDown)
    {
        // CameraMoveRight(camera, delta);

        f32 velocity = 1.8f * delta;
        v3  right    = Norm(Cross(player->target, { 0.0f, 1.0f, 0.0f }));
        player->position -= right * velocity;
    }
    if (input->mouse.left.isDown && !input->mouse.left.wasDown)
    {
        CameraSetPitch(camera, camera->pitch + 1.0f);
    }
    if (input->mouse.right.isDown && !input->mouse.right.wasDown)
    {
        CameraSetPitch(camera, camera->pitch - 1.0f);
    }

    if (input->mouse.middle.isDown && !input->mouse.middle.wasDown)
    {
        platform.AudioClipPlay(state->pistolShot, 0);
    }

    if (input->mouse.middle.isDown)
    {
        // camera->position.y += 0.01f;
        platform.Logf("Position %.2f %.2f %.2f", camera->position.x, camera->position.y, camera->position.z);
        platform.Logf("Target %.2f %.2f %.2f", camera->target.x, camera->target.y, camera->target.z);
        platform.Logf("Pitch %.2f", camera->pitch);
        platform.Logf("Yaw %.2f", camera->yaw);

        platform.Logf("Position %.2f %.2f %.2f", player->position.x, player->position.y, player->position.z);
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
    // ----------------------------------------------------------------------------

    // ----------------------------------------------------------------------------
    // Draw
    v2u windowDim = platform.WindowGetDimension();

    mat4x4 projection = Perspective(Radians(45.0f), (f32)windowDim.w / (f32)windowDim.h, 0.1f, 100.0f);
    mat4x4 view       = CameraView(camera);

    RenderCommandQueue* renderCommandQueue = RendererFrameBegin(opengl);

    FramebufferClear* framebufferClear = PushRenderCommand(renderCommandQueue, FramebufferClear);
    framebufferClear->color.r          = 0.0f;
    framebufferClear->color.g          = 0.0f;
    framebufferClear->color.b          = 0.0f;

    ProgramUse* programUse = PushRenderCommand(renderCommandQueue, ProgramUse);
    programUse->program.id = state->program->id;

    {
        mat4x4 translate = Translate(Identity(), player->position);
        mat4x4 scale     = Scale(Identity(), 0.5f);
        mat4x4 model     = translate * scale;

        ProgramUploadUniformMatrix4x4* uniform = PushRenderCommand(renderCommandQueue, ProgramUploadUniformMatrix4x4);
        sprintf(uniform->name, "%s", "mvp");
        uniform->program = *state->program;
        uniform->mat4x4  = projection * view * model;

        GeometryBufferDraw* draw = PushRenderCommand(renderCommandQueue, GeometryBufferDraw);
        draw->buffer             = *state->cubeBuffer;
    }

    RendererFrameEnd(opengl);

#ifdef BUILD_TYPE_DEBUG
    DebugFrameBegin(state->debug, opengl, projection * view);
    {
        u32 numCols = 20;
        u32 numRows = 20;
        f32 x       = 0.0f;
        f32 z       = 0.0f;

        for (u32 col = 0; col < numCols; col++)
        {
            x = -((float)numCols / 2.0f);

            for (u32 row = 0; row < numRows; row++)
            {
                DebugDrawPlane(state->debug, opengl, { x, 0.0f, z }, white);

                x += 2.0f;
            }

            z -= 2.0f;
        }

        DebugDrawLine(state->debug, opengl, player->position, player->position + (player->target), blue);
        DebugDrawLine(state->debug, opengl, player->position,
                      { player->position.x, player->position.y + 1.5f, player->position.z }, green);
        DebugDrawLine(state->debug, opengl, player->position,
                      { player->position.x + 1.5f, player->position.y, player->position.z }, red);
    }
    DebugFrameEnd(state->debug, opengl);
#endif
    return 0;
}