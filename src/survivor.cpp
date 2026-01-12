#include "survivor.h"

#include "survivor_renderer_opengl.cpp"
#include "survivor_debug_geometry.cpp"
#include "survivor_debug.cpp"

internal u32 rectIndices[]   = { 0, 1, 2, 0, 2, 3 };
internal u32 rectVertexCount = 4;
internal u32 rectIndexCount  = 6;

inline void BatchRect(BatchBuffer* batch, v2 position, v2 size, v4 color)
{
    if ((batch->vertexCount + rectVertexCount > batch->maxVertexCount) ||
        (batch->indexCount + rectIndexCount > batch->maxIndexCount))
    {
        Assert(0);
    }

    for (u32 index = 0; index < rectIndexCount; index++)
    {
        *batch->indexBufferPtr = rectIndices[index] + batch->vertexCount;
        batch->indexBufferPtr++;

        batch->indexCount++;
    }

    // Top-right
    batch->vertexBufferPtr->position     = { position.x + size.w, position.y };
    batch->vertexBufferPtr->color        = color;
    batch->vertexBufferPtr->textureIndex = 0;
    batch->vertexBufferPtr++;
    // Bottom-right
    batch->vertexBufferPtr->position     = { position.x + size.w, position.y + size.h };
    batch->vertexBufferPtr->color        = color;
    batch->vertexBufferPtr->textureIndex = 0;
    batch->vertexBufferPtr++;
    // Bottom-left
    batch->vertexBufferPtr->position     = { position.x, position.y + size.h };
    batch->vertexBufferPtr->color        = color;
    batch->vertexBufferPtr->textureIndex = 0;
    batch->vertexBufferPtr++;
    // Top-left
    batch->vertexBufferPtr->position     = { position.x, position.y };
    batch->vertexBufferPtr->color        = color;
    batch->vertexBufferPtr->textureIndex = 0;
    batch->vertexBufferPtr++;

    batch->vertexCount += 4;
}

// TODO: Texture index assignation
inline void BatchTextureRect(BatchBuffer* batch, v2 position, v2 size, Texture* texture)
{
    if ((batch->vertexCount + rectVertexCount > batch->maxVertexCount) ||
        (batch->indexCount + rectIndexCount > batch->maxIndexCount))
    {
        Assert(0);
    }

    BatchVertex* vertexBufferPtr = batch->vertexBufferPtr;
    BatchRect(batch, position, size, { 1.0f, 1.0f, 1.0f, 1.0f });

    // Top-right
    vertexBufferPtr->uv           = { 1.0f, 1.0f };
    vertexBufferPtr->textureIndex = 1;
    vertexBufferPtr++;
    // Bottom-right
    vertexBufferPtr->uv           = { 1.0f, 0.0f };
    vertexBufferPtr->textureIndex = 1;
    vertexBufferPtr++;
    // Bottom-left
    vertexBufferPtr->uv           = { 0.0f, 0.0f };
    vertexBufferPtr->textureIndex = 1;
    vertexBufferPtr++;
    // Top-left
    vertexBufferPtr->uv           = { 0.0f, 1.0f };
    vertexBufferPtr->textureIndex = 1;
}

// TODO: Texture index assignation
inline void BatchTextureSubRect(BatchBuffer* batch, v2 rectPosition, v2 rectSize, Texture* texture, v2 textPosition,
                                v2 textSize)
{
    if ((batch->vertexCount + rectVertexCount > batch->maxVertexCount) ||
        (batch->indexCount + rectIndexCount > batch->maxIndexCount))
    {
        Assert(0);
    }

    f32 textureW = (1.0f / texture->width) * textSize.w;
    f32 textureH = (1.0f / texture->height) * textSize.h;
    f32 textureX = (1.0f / texture->width) * textPosition.x;
    f32 textureY = (1.0f / texture->height) * textPosition.y;

    BatchVertex* vertexBufferPtr = batch->vertexBufferPtr;
    BatchRect(batch, rectPosition, rectSize, { 1.0f, 1.0f, 1.0f, 1.0f });

    // Top-right
    vertexBufferPtr->uv           = { textureX + textureW, textureY };
    vertexBufferPtr->textureIndex = 1;
    vertexBufferPtr++;
    // Bottom-right
    vertexBufferPtr->uv           = { textureX + textureW, textureY + textureH };
    vertexBufferPtr->textureIndex = 1;
    vertexBufferPtr++;
    // Bottom-left
    vertexBufferPtr->uv           = { textureX, textureY + textureH };
    vertexBufferPtr->textureIndex = 1;
    vertexBufferPtr++;
    // Top-left
    vertexBufferPtr->uv           = { textureX, textureY };
    vertexBufferPtr->textureIndex = 1;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(GameState) <= memory->permanentStorageSize);

    GameState*  state    = (GameState*)memory->permanentStorage;
    PlatformAPI platform = memory->platform;
    OpenGL*     opengl   = &memory->opengl;
    Arena*      arena    = &state->arena;

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
        ProgramAttachShader(opengl, state->program, (char*)vertexSourceFile.content, vertexSourceFile.contentSize,
                            GL_VERTEX_SHADER);
        ProgramAttachShader(opengl, state->program, (char*)fragmentSourceFile.content, fragmentSourceFile.contentSize,
                            GL_FRAGMENT_SHADER);
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

        // Texture loading
        {
            state->whiteTexture   = PushStruct(arena, Texture);
            state->crosshairAtlas = PushStruct(arena, Texture);

            FileReadResult imageReadResult = platform.FileReadEntire("../data/crosshairs.png");
            if (imageReadResult.contentSize > 0)
            {
                TextureAlloc(opengl, state->crosshairAtlas, imageReadResult.content, imageReadResult.contentSize);
                platform.FileFree(imageReadResult.content);
            }
            else
            {
                Assert(0);
            }

            // TODO: Have a similar function to TextureAlloc and remove this opengl function calls!
            u32 pixels = 0xFFFFFFFF;
            opengl->glGenTextures(1, &state->whiteTexture->id);
            opengl->glBindTexture(GL_TEXTURE_2D, state->whiteTexture->id);
            opengl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels);
        }

        // 2D batching
        {
            state->batchBuffer = PushStruct(arena, BatchBuffer);

            BatchBuffer* batch = state->batchBuffer;

            batch->maxVertexCount   = 1000;
            batch->maxIndexCount    = batch->maxVertexCount * 6;
            batch->vertexCount      = 0;
            batch->indexCount       = 0;
            batch->vertexBufferBase = PushArray(&state->arena, batch->maxVertexCount, BatchVertex);
            batch->indexBufferBase  = PushArray(&state->arena, batch->maxIndexCount, u32);
            batch->vertexBufferPtr  = batch->vertexBufferBase;
            batch->indexBufferPtr   = batch->indexBufferBase;

            Program* program = &state->batchBuffer->program;

            FileReadResult vsFile = platform.FileReadEntire("../src/shaders/batch.vert");
            FileReadResult fsFile = platform.FileReadEntire("../src/shaders/batch.frag");

            ProgramInit(opengl, program);
            ProgramAttachShader(opengl, program, (char*)vsFile.content, vsFile.contentSize, GL_VERTEX_SHADER);
            ProgramAttachShader(opengl, program, (char*)fsFile.content, fsFile.contentSize, GL_FRAGMENT_SHADER);
            ProgramBuild(opengl, program);

            platform.FileFree(vsFile.content);
            platform.FileFree(fsFile.content);

            size_t vertexSize = sizeof(BatchVertex);

            GeometryBuffer* buffer = &batch->buffer;

            GeometryBufferInit(opengl, buffer, GL_TRIANGLES);
            GeometryBufferVBOAlloc(opengl, buffer, 0, vertexSize * batch->maxVertexCount, vertexSize, GL_DYNAMIC_DRAW);
            GeometryBufferEBOAlloc(opengl, buffer, 0, sizeof(u32) * batch->maxIndexCount, sizeof(u32), GL_DYNAMIC_DRAW);
            GeometryBufferVertexAttrib(opengl, buffer, 0, 3, GL_FLOAT, vertexSize, offsetof(BatchVertex, position));
            GeometryBufferVertexAttrib(opengl, buffer, 1, 2, GL_FLOAT, vertexSize, offsetof(BatchVertex, uv));
            GeometryBufferVertexAttrib(opengl, buffer, 2, 4, GL_FLOAT, vertexSize, offsetof(BatchVertex, color));
            GeometryBufferVertexAttrib(opengl, buffer, 3, 1, GL_INT, vertexSize, offsetof(BatchVertex, textureIndex));
        }

        v3  position = { 0.0f, 16.0f, 5.0f };
        v3  target   = { 0.0f, -0.9f, -0.4f };
        v3  up       = { 0.0f, 1.0f, 0.0f };
        f32 pitch    = -68.0f;
        f32 yaw      = -90.0f;

        CameraInit(state->camera, position, target, up, pitch, yaw, 45.0f);

        state->pistolShot      = platform.AudioClipLoad("../data/pistol.wav", AudioClipType_Sfx);
        state->backgroundMusic = platform.AudioClipLoad("../data/background.wav", AudioClipType_Music);

        platform.AudioSetVolume(-35.0f, AudioClipType_Music);
        platform.AudioSetVolume(-3.0f, AudioClipType_Sfx);
        platform.AudioClipPlay(state->backgroundMusic, AudioClipPlayFlag_Loop);

        state->player->position = { 0.0f, 0.0f, 0.0f };
        state->player->velocity = { 0.0f, 0.0f, 0.0f };
        state->player->yaw      = -90.0f;
    }
    // ----------------------------------------------------------------------------

    // ----------------------------------------------------------------------------
    // Update
    v2u     windowDim  = platform.WindowGetDimension();
    Player* player     = state->player;
    Camera* camera     = state->camera;
    mat4x4  projection = Perspective(Radians(45.0f), (f32)windowDim.w / (f32)windowDim.h, 0.1f, 100.0f);
    mat4x4  view       = CameraView(camera);

    v3 inputDirection{ 0.0f, 0.0f, 0.0f };
    // TODO: Tweak movement mechanics
    f32 maxSpeed         = 7.0f;
    f32 frictionForce    = 25.0f;
    f32 moveAcceleration = 40.0f;
    v3  cameraOffset{ 0.0f, 16.0f, 5.0f };

    for (u32 controllerIndex = 0; controllerIndex < ArrayCount(input->controllers); controllerIndex++)
    {
        GameInputController* controller = GetController(input, controllerIndex);

        // TODO: Controller gameplay
        if (controller->isAnalog)
        {
            if (controller->rightTrigger.isDown && !controller->rightTrigger.wasDown)
            {
                platform.AudioClipPlay(state->pistolShot, 0);
            }
            if (controller->leftTrigger.isDown)
            {
                platform.Logf("Gamepad aiming");
            }
        }
        else
        {
            Mouse* mouse = &input->mouse;

            state->cursor = mouse->pos;

            v3 playerDirection = { 0 };

            if (controller->moveUp.isDown)
            {
                playerDirection.z -= 1.0f;
            }
            if (controller->moveDown.isDown)
            {
                playerDirection.z += 1.0f;
            }
            if (controller->moveLeft.isDown)
            {
                playerDirection.x = -1.0f;
            }
            if (controller->moveRight.isDown)
            {
                playerDirection.x = 1.0f;
            }
            playerDirection = Norm(playerDirection);

            v3  newPlayerVelocity = player->velocity;
            f32 playerSpeed       = Length(player->velocity);

            // Deceleration
            if (playerSpeed > 0.0f && Length(playerDirection) == 0.0f)
            {
                f32 decelerationStep = frictionForce * delta;

                if (playerSpeed <= decelerationStep)
                {
                    newPlayerVelocity = v3{ 0, 0, 0 };
                }
                else
                {
                    newPlayerVelocity -= (newPlayerVelocity / playerSpeed) * decelerationStep;
                }

                player->velocity = newPlayerVelocity;
            }
            // Acceleration
            else if (Length(playerDirection) > 0.0f)
            {
                v3 acceleration = playerDirection * moveAcceleration;
                player->velocity += acceleration * delta;
                if (playerSpeed > maxSpeed)
                {
                    player->velocity = Norm(player->velocity) * maxSpeed;
                }
            }

            player->position += player->velocity * delta;
            camera->position = player->position + cameraOffset;

            // Player rotation
            {
                f32 screenWidth  = (f32)windowDim.w;
                f32 screenHeight = (f32)windowDim.h;
                f32 mouseX       = (f32)mouse->pos.x;
                f32 mouseY       = (f32)mouse->pos.y;

                mat4x4 inverseProjection = Inverse(projection);
                mat4x4 inverseView       = Inverse(view);

                // Viewport -> NDC
                v3 rayNdc = { 0 };
                rayNdc.x  = (2.0f * mouseX) / screenWidth - 1.0f;
                rayNdc.y  = 1.0f - (2.0f * mouseY) / screenHeight;

                // NDC -> Clip
                v4 rayClip{ rayNdc.x, rayNdc.y, -1.0f, 1.0f };

                // Clip -> View
                v4 rayView = inverseProjection * rayClip;
                rayView.z  = -1.0f;
                rayView.w  = 0;

                // View -> World
                v4 rayWorld4 = inverseView * rayView;
                v3 rayWorld{ rayWorld4.x, rayWorld4.y, rayWorld4.z };
                rayWorld = Norm(rayWorld);

                // Intersection with world plane
                float t     = -(camera->position.y / rayWorld.y);
                v3    point = camera->position + rayWorld * t;
                // Direction from the player to the point
                v3 dir = point - player->position;

                f32 targetYaw = -atan2f(dir.x, -dir.z);
                f32 deltaYaw  = targetYaw - player->yaw;
                // Wrap to [-Pi, Pi]
                deltaYaw          = fmodf(deltaYaw + Pi, 2.0f * Pi) - Pi;
                f32 rotationSpeed = 0.05f;
                player->yaw += deltaYaw * rotationSpeed;
            }

            if (mouse->left.isDown && !mouse->left.wasDown)
            {
                platform.AudioClipPlay(state->pistolShot, 0);
            }
        }
    }
    // ----------------------------------------------------------------------------

    // ----------------------------------------------------------------------------
    // Draw
    RenderCommandQueue* commandQueue = RendererFrameBegin(opengl);

    FramebufferClear* framebufferClear = PushRenderCommand(commandQueue, FramebufferClear);
    framebufferClear->color.r          = 0.2f;
    framebufferClear->color.g          = 0.3f;
    framebufferClear->color.b          = 0.5f;

    BatchBuffer* batch     = state->batchBuffer;
    batch->vertexBufferPtr = batch->vertexBufferBase;
    batch->indexBufferPtr  = batch->indexBufferBase;
    batch->vertexCount     = 0;
    batch->indexCount      = 0;

    // TODO: Find a better way to handle textures
    int textureArray[] = { 0, 1 };
    opengl->glActiveTexture(GL_TEXTURE0);
    opengl->glBindTexture(GL_TEXTURE_2D, state->whiteTexture->id);
    opengl->glActiveTexture(GL_TEXTURE1);
    opengl->glBindTexture(GL_TEXTURE_2D, state->crosshairAtlas->id);

    // TODO: This kind of operations with render commands
    opengl->glEnable(GL_BLEND);
    opengl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 2D
    {
        mat4x4 view2D = Orthographic(0, (f32)windowDim.w, (f32)windowDim.h, 0);

        mat4x4 translate = Translate(Identity(), { (f32)state->cursor.x, (f32)state->cursor.y, 0.0f });
        mat4x4 scale     = Scale(Identity(), { 25.0f, 25.0f, 0.0f });
        mat4x4 model     = translate * scale;

        BatchRect(batch, { 5.0f, 5.0f }, { 50.0f, 50.0f }, { 1.0f, 0.0f, 0.2f, 1.0f });
        BatchRect(batch, { 0.0f, 300.0f }, { 50.0f, 50.0f }, { 0.0f, 0.6f, 0.0f, 1.0f });
        BatchTextureRect(batch, { 100.0f, 100.0f }, { 100.0f, 100.0f }, state->crosshairAtlas);
        BatchTextureSubRect(batch, { 300.0f, 300.0f }, { 250.0f, 250.0f }, state->crosshairAtlas, { 965.0f, 0.0f },
                            { 128.0f, 128.0f });
        // 828
        // 965

        // Batch
        if (batch->vertexCount > 0)
        {
            // TODO: This is ugly, rethink DrawBuffer render command. (It might take index/primitive count)
            batch->buffer.indexCount  = batch->indexCount;
            batch->buffer.vertexCount = batch->vertexCount;

            GeometryBufferVBOSubdata(opengl, &batch->buffer, batch->vertexBufferBase,
                                     sizeof(BatchVertex) * batch->vertexCount);
            GeometryBufferEBOSubdata(opengl, &batch->buffer, batch->indexBufferBase, sizeof(u32) * batch->indexCount);

            PushRenderProgramUse(commandQueue, batch->program.id);
            PushRenderUploadUniformMat4x4(commandQueue, batch->program.id, "viewProj", view2D);
            PushRenderUploadUniformIntArray(commandQueue, batch->program.id, "textureArray", textureArray,
                                            ArrayCount(textureArray));

            PushRenderDrawBuffer(commandQueue, &batch->buffer);

            batch->vertexBufferPtr = batch->vertexBufferBase;
            batch->indexBufferPtr  = batch->indexBufferBase;
            batch->vertexCount     = 0;
            batch->indexCount      = 0;
        }
    }

    // 3D
    {
        mat4x4 translate = Translate(Identity(), player->position);
        mat4x4 rotate    = Rotate(Identity(), player->yaw, { 0.0f, 1.0f, 0.0f });
        mat4x4 scale     = Scale(Identity(), 0.5f);
        mat4x4 model     = translate * rotate * scale;

        PushRenderProgramUse(commandQueue, state->program->id);
        PushRenderUploadUniformMat4x4(commandQueue, state->program->id, "mvp", projection * view * model);
        PushRenderDrawBuffer(commandQueue, state->cubeBuffer);
    }

    RendererFrameEnd(opengl);

#ifdef BUILD_TYPE_DEBUG
    v3 green{ 0.0f, 1.0f, 0.0f };
    v3 red{ 1.0f, 0.0f, 0.0f };
    v3 blue{ 0.2f, 0.4f, 1.0f };
    v3 white{ 1.0f, 1.0f, 1.0f };

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
                x += 0.2f;
            }

            z -= 2.0f;
            z -= 0.2f;
        }

        v3 playerTarget{ sinf(player->yaw), 0.0f, cosf(player->yaw) };

        DebugDrawLine(state->debug, opengl, player->position, player->position + (playerTarget * 1.5f), blue);
    }
    DebugFrameEnd(state->debug, opengl);
#endif
    return 0;
}