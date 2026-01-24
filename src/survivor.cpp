#include "survivor.h"

#include "survivor_renderer_opengl.cpp"
#include "survivor_debug_geometry.cpp"
#include "survivor_debug.cpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

global_variable u32 rectIndices[]   = { 0, 1, 2, 0, 2, 3 };
global_variable u32 rectVertexCount = 4;
global_variable u32 rectIndexCount  = 6;
global_variable v4  green{ 0.0f, 1.0f, 0.0f, 1.0f };
global_variable v4  red{ 1.0f, 0.0f, 0.0f, 1.0f };
global_variable v4  blue{ 0.2f, 0.4f, 1.0f, 1.0f };
global_variable v4  white{ 1.0f, 1.0f, 1.0f, 1.0f };

// TODO
/*
- (Batch) Review batch buffer size: Ideally, should be large enough to handle a single render call per
  frame. Otherwise, I'm not sure how to send multiple draw calls in the same frame using batching approach.
- (Batch) Texture index assignation (remove index parameter in batch function and hardcoded values)
- (Renderer) Rethink TextureAlloc. See how to alloc simple textures as the white one. Check for different parameters
  (swizzle, min/mag filters, etc)
- (Renderer) Rethink DrawBuffer render command. (Is not enough flexible for batching and is not easy to change the
primitive type)
- (Audio) Make easy to tweak volumes (ignore db conversion)
- (Misc): Temporal arenas
- (Game): gamepad controller
*/

inline void BatchRect(BatchBuffer* batch, v2 topLeft, v2 bottomRight, v4 color)
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
    batch->vertexBufferPtr->position     = { topLeft.x + bottomRight.x, topLeft.y };
    batch->vertexBufferPtr->color        = color;
    batch->vertexBufferPtr->textureIndex = 0;
    batch->vertexBufferPtr++;
    // Bottom-right
    batch->vertexBufferPtr->position     = { topLeft.x + bottomRight.x, topLeft.y + bottomRight.y };
    batch->vertexBufferPtr->color        = color;
    batch->vertexBufferPtr->textureIndex = 0;
    batch->vertexBufferPtr++;
    // Bottom-left
    batch->vertexBufferPtr->position     = { topLeft.x, topLeft.y + bottomRight.y };
    batch->vertexBufferPtr->color        = color;
    batch->vertexBufferPtr->textureIndex = 0;
    batch->vertexBufferPtr++;
    // Top-left
    batch->vertexBufferPtr->position     = { topLeft.x, topLeft.y };
    batch->vertexBufferPtr->color        = color;
    batch->vertexBufferPtr->textureIndex = 0;
    batch->vertexBufferPtr++;

    batch->vertexCount += 4;
}

inline void BatchTextureRect(BatchBuffer* batch, v2 topLeft, v2 bottomRight, Texture* texture, u32 textureIndex = 1)
{
    if ((batch->vertexCount + rectVertexCount > batch->maxVertexCount) ||
        (batch->indexCount + rectIndexCount > batch->maxIndexCount))
    {
        Assert(0);
    }

    BatchVertex* vertexBufferPtr = batch->vertexBufferPtr;
    BatchRect(batch, topLeft, bottomRight, { 1.0f, 1.0f, 1.0f, 1.0f });

    // Top-right
    vertexBufferPtr->uv           = { 1.0f, 1.0f };
    vertexBufferPtr->textureIndex = textureIndex;
    vertexBufferPtr++;
    // Bottom-right
    vertexBufferPtr->uv           = { 1.0f, 0.0f };
    vertexBufferPtr->textureIndex = textureIndex;
    vertexBufferPtr++;
    // Bottom-left
    vertexBufferPtr->uv           = { 0.0f, 0.0f };
    vertexBufferPtr->textureIndex = textureIndex;
    vertexBufferPtr++;
    // Top-left
    vertexBufferPtr->uv           = { 0.0f, 1.0f };
    vertexBufferPtr->textureIndex = textureIndex;
}

inline void BatchTextureSubRect(BatchBuffer* batch, v2 topLeft, v2 bottomRight, Texture* texture, v2 textureTopLeft,
                                v2 textureBottomRight, u32 textureIndex = 1)
{
    if ((batch->vertexCount + rectVertexCount > batch->maxVertexCount) ||
        (batch->indexCount + rectIndexCount > batch->maxIndexCount))
    {
        Assert(0);
    }

    f32 textureW = (1.0f / texture->width) * textureBottomRight.x;
    f32 textureH = (1.0f / texture->height) * textureBottomRight.y;
    f32 textureX = (1.0f / texture->width) * textureTopLeft.x;
    f32 textureY = (1.0f / texture->height) * textureTopLeft.y;

    BatchVertex* vertexBufferPtr = batch->vertexBufferPtr;
    BatchRect(batch, topLeft, bottomRight, { 1.0f, 1.0f, 1.0f, 1.0f });

    // Top-right
    vertexBufferPtr->uv           = { textureX + textureW, textureY };
    vertexBufferPtr->textureIndex = textureIndex;
    vertexBufferPtr++;
    // Bottom-right
    vertexBufferPtr->uv           = { textureX + textureW, textureY + textureH };
    vertexBufferPtr->textureIndex = textureIndex;
    vertexBufferPtr++;
    // Bottom-left
    vertexBufferPtr->uv           = { textureX, textureY + textureH };
    vertexBufferPtr->textureIndex = textureIndex;
    vertexBufferPtr++;
    // Top-left
    vertexBufferPtr->uv           = { textureX, textureY };
    vertexBufferPtr->textureIndex = textureIndex;
}

inline void BatchText(GameState* state, BatchBuffer* batch, char* text, v2 position, v4 color, f32 scale = 1.0f)
{
    size_t textLength      = strlen(text);
    u32    textVertexCount = (u32)textLength * rectVertexCount;
    u32    textIndexCount  = (u32)textLength * rectIndexCount;

    if ((batch->vertexCount + textVertexCount > batch->maxVertexCount) ||
        (batch->indexCount + textIndexCount > batch->maxIndexCount))
    {
        Assert(0);
    }

    v2 rectTopLeft{ 0.0f, 0.0f };
    rectTopLeft += position;

    while (*text)
    {
        TTFGlyph* ttfChar = &state->ttfChars[*text++ - TTF_FIRST_GLYPH_OFFSET];

        rectTopLeft.x += (ttfChar->xoff * scale);
        rectTopLeft.y = position.y + (ttfChar->yoff * scale);
        v2 rectBottomRight{ ((f32)ttfChar->x1 - (f32)ttfChar->x0) * scale,
                            ((f32)ttfChar->y1 - (f32)ttfChar->y0) * scale };

        v2 subrectTopLeft{ (f32)ttfChar->x0 + ttfChar->s0, (f32)ttfChar->y0 + ttfChar->t0 };
        v2 subrectBottomRight{ ((f32)ttfChar->x1 - (f32)ttfChar->x0) - ttfChar->s1,
                               ((f32)ttfChar->y1 - (f32)ttfChar->y0) - ttfChar->t1 };

        BatchVertex* verterBufferPtr = batch->vertexBufferPtr;
        BatchTextureSubRect(batch, rectTopLeft, rectBottomRight, state->glyphAtlas, subrectTopLeft, subrectBottomRight,
                            2);

        verterBufferPtr->color = color;
        verterBufferPtr++;
        verterBufferPtr->color = color;
        verterBufferPtr++;
        verterBufferPtr->color = color;
        verterBufferPtr++;
        verterBufferPtr->color = color;
        verterBufferPtr++;

        rectTopLeft.x += (ttfChar->xadvance * scale);
    }
}

inline b32 BbboxInsertecs(Bbox* a, Bbox* b)
{
    bool x = (a->max.x >= b->min.x) && (a->min.x <= b->max.x);
    bool y = (a->max.y >= b->min.y) && (a->min.y <= b->max.y);
    bool z = (a->max.z >= b->min.z) && (a->min.z <= b->max.z);

    return (x && y && z);
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

        // Plane
        {
            state->planeBuffer = PushStruct(arena, GeometryBuffer);

            GeometryBufferInit(opengl, state->planeBuffer, GL_TRIANGLES);
            GeometryBufferVBOAlloc(opengl, state->planeBuffer, planeVertexs, sizeof(planeVertexs), sizeof(Vertex),
                                   GL_STATIC_DRAW);
            GeometryBufferEBOAlloc(opengl, state->planeBuffer, planeIndices, ArrayCount(planeIndices) * sizeof(u32),
                                   sizeof(u32), GL_STATIC_DRAW);
            GeometryBufferVertexAttrib(opengl, state->planeBuffer, 0, 3, GL_FLOAT, sizeof(Vertex),
                                       offsetof(Vertex, position));
            GeometryBufferVertexAttrib(opengl, state->planeBuffer, 1, 3, GL_FLOAT, sizeof(Vertex),
                                       offsetof(Vertex, normal));
            GeometryBufferVertexAttrib(opengl, state->planeBuffer, 2, 2, GL_FLOAT, sizeof(Vertex),
                                       offsetof(Vertex, uv));
        }

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

        state->player->position = { 0.0f, 1.0f, 0.0f };
        state->player->velocity = { 0.0f, 0.0f, 0.0f };
        state->player->yaw      = -90.0f;
        state->player->height   = 0.50f;

        // Font loading
        {
            FileReadResult fontFile = platform.FileReadEntire("c:\\windows\\fonts\\calibri.ttf");
            if (fontFile.contentSize > 0)
            {
                stbtt_fontinfo fontInfo   = { 0 };
                u8*            fontBuffer = (u8*)fontFile.content;

                if (stbtt_InitFont(&fontInfo, fontBuffer, 0))
                {
                    int fontAtlasWidth  = 1024;
                    int fontAtlasHeight = 1024;
                    f32 fontSize        = 64.0f;
                    // TODO: (Temporal arenas) Free bitmap after allocating texture
                    u8* bitmapFontBuffer = PushArray(arena, fontAtlasWidth * fontAtlasHeight, u8);

                    stbtt_pack_context packCtx;
                    stbtt_packedchar   packedChars[TTF_GLYPH_COUNT];

                    stbtt_PackBegin(&packCtx, bitmapFontBuffer, fontAtlasWidth, fontAtlasHeight, 0, 1, 0);
                    stbtt_PackFontRange(&packCtx, fontBuffer, 0, fontSize, TTF_FIRST_GLYPH_OFFSET, TTF_GLYPH_COUNT,
                                        packedChars);
                    stbtt_PackEnd(&packCtx);

                    for (u32 charIndex = 0; charIndex < TTF_GLYPH_COUNT; charIndex++)
                    {
                        float x, y;

                        stbtt_aligned_quad alignedQuad;
                        stbtt_GetPackedQuad(packedChars, fontAtlasWidth, fontAtlasHeight, (int)charIndex, &x, &y,
                                            &alignedQuad, 0);

                        TTFGlyph* ttfChar = &state->ttfChars[charIndex];
                        ttfChar->x0       = packedChars[charIndex].x0;
                        ttfChar->y0       = packedChars[charIndex].y0;
                        ttfChar->x1       = packedChars[charIndex].x1;
                        ttfChar->y1       = packedChars[charIndex].y1;
                        ttfChar->xoff     = packedChars[charIndex].xoff;
                        ttfChar->yoff     = packedChars[charIndex].yoff;
                        ttfChar->xadvance = packedChars[charIndex].xadvance;
                        ttfChar->s0       = alignedQuad.s0;
                        ttfChar->t0       = alignedQuad.t0;
                        ttfChar->s1       = alignedQuad.s1;
                        ttfChar->t1       = alignedQuad.t1;
                    }

                    platform.FileFree(fontFile.content);

                    state->glyphAtlas = PushStruct(arena, Texture);

                    opengl->glGenTextures(1, &state->glyphAtlas->id);
                    opengl->glBindTexture(GL_TEXTURE_2D, state->glyphAtlas->id);
                    opengl->glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, (GLsizei)fontAtlasWidth, (GLsizei)fontAtlasHeight, 0,
                                         GL_RED, GL_UNSIGNED_BYTE, (void*)bitmapFontBuffer);
                    GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
                    opengl->glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
                    opengl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    opengl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    state->glyphAtlas->width  = (u32)fontAtlasWidth;
                    state->glyphAtlas->height = (u32)fontAtlasHeight;
                }
                else
                {
                    platform.Logf("Unable to init .ttf font");
                    Assert(0);
                }
            }
            else
            {
                platform.Logf("Unable to load font");
                Assert(0);
            }
        }
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
    f32 frictionForce    = 20.0f;
    f32 moveAcceleration = 40.0f;
    v3  cameraOffset{ 0.0f, 16.0f, 5.0f };
    CameraSetPitch(camera, -68.0f);

#if 0
    // v3 cameraOffset{ 0.0f, 4.0f, 8.0f };
    // CameraSetPitch(camera, -26.0f);

    v3 cameraOffset{ 0.0f, -0.5f, 8.0f };
    CameraSetPitch(camera, 5.0f);
#endif

    v3 boxPosition{ -3.0f, 0.0f, -3.0f };
    v3 testAABBPosition{ -8.0f, 3.0f, -1.0f };
    v3 testHalfExtend{ 1.0f, 3.0f, 1.0f };
    v3 playerHalfExtend{ 0.5f, 0.5f, 0.5f };

    for (u32 controllerIndex = 0; controllerIndex < ArrayCount(input->controllers); controllerIndex++)
    {
        GameInputController* controller = GetController(input, controllerIndex);

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

            v3 newPlayerPosition = player->position + (player->velocity * delta);

            // World limits
            f32 floorSize    = 30.0f;
            f32 playerCenter = player->height;

            // Left limit
            if (newPlayerPosition.x - playerCenter < -floorSize - 0.5f)
            {
                newPlayerPosition.x = player->position.x;
            }
            // Right limit
            if (newPlayerPosition.x + playerCenter > floorSize + 0.5f)
            {
                newPlayerPosition.x = player->position.x;
            }
            // Top limit
            if (newPlayerPosition.z - playerCenter < -floorSize - 0.5f)
            {
                newPlayerPosition.z = player->position.z;
            }
            // Bottom limit
            if (newPlayerPosition.z + playerCenter > floorSize + 0.5f)
            {
                newPlayerPosition.z = player->position.z;
            }

            Bbox playerBbbox;
            playerBbbox.min = newPlayerPosition - playerHalfExtend;
            playerBbbox.max = newPlayerPosition + playerHalfExtend;

            Bbox testBbbox;
            testBbbox.min = testAABBPosition - testHalfExtend;
            testBbbox.max = testAABBPosition + testHalfExtend;

            if (BbboxInsertecs(&playerBbbox, &testBbbox))
            {
                platform.Logf("Bbox intersection");
                // v3  r{ 1.0f, 0.0f, 0.0f };
                // f32 pushStrenght = 2.0f;
                // player->velocity += r * pushStrength;
            }
            else
            {
                platform.Logf("\n");
                player->position = newPlayerPosition;
                camera->position = player->position + cameraOffset;
            }

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
    framebufferClear->color.r          = 0.18f;
    framebufferClear->color.g          = 0.31f;
    framebufferClear->color.b          = 0.52f;

    BatchBuffer* batch     = state->batchBuffer;
    batch->vertexBufferPtr = batch->vertexBufferBase;
    batch->indexBufferPtr  = batch->indexBufferBase;
    batch->vertexCount     = 0;
    batch->indexCount      = 0;

    // TODO: Find a better way to handle textures
    int textureArray[] = { 0, 1, 2 };
    opengl->glActiveTexture(GL_TEXTURE0);
    opengl->glBindTexture(GL_TEXTURE_2D, state->whiteTexture->id);
    opengl->glActiveTexture(GL_TEXTURE1);
    opengl->glBindTexture(GL_TEXTURE_2D, state->crosshairAtlas->id);
    opengl->glActiveTexture(GL_TEXTURE2);
    opengl->glBindTexture(GL_TEXTURE_2D, state->glyphAtlas->id);

    // TODO: This kind of operations with render commands
    opengl->glDisable(GL_DEPTH_TEST);
    opengl->glEnable(GL_BLEND);
    opengl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Batch
    {
        mat4x4 view2D = Orthographic(0, (f32)windowDim.w, (f32)windowDim.h, 0);

        Mouse* mouse = &input->mouse;
        // Cursors: 828, 965
        BatchTextureSubRect(batch, { (f32)mouse->pos.x, (f32)mouse->pos.y }, { 32.0f, 32.0f }, state->crosshairAtlas,
                            { 965.0f, 0.0f }, { 128.0f, 128.0f });

        char coordBuffer[64];
        sprintf(coordBuffer, "%d %d %d", (int)player->position.x, (int)player->position.y, (int)player->position.z);
        BatchText(state, batch, coordBuffer, { 0.0f, 50.0f }, white);

#if 0
        BatchRect(batch, { 0.0f, 220.0f }, { (f32)windowDim.w, 50.0f }, { 1.0f, 0.0f, 0.2f, 1.0f });
        BatchText(state, batch, "The quick brown fox jumps over the lazy dog", { 0.0f, 250.0f },
                  { 0.5f, 1.0f, 1.0f, 1.0f }, 0.5f);

        BatchText(state, batch, "The quick brown fox jumps over the lazy dog", { 0.0f, 320.0f }, white, 0.7f);
        BatchText(state, batch, "The quick brown fox jumps over the lazy dog", { 0.0f, 390.0f }, red);

        BatchText(state, batch, "The quick brown fox jumps over the lazy dog", { 0.0f, 460.0f }, green, 1.2f);
        BatchText(state, batch, "The quick brown fox jumps over the lazy dog", { 0.0f, 540.0f },
                  { 1.0f, 0.0f, 1.0f, 1.0f }, 1.5f);
#endif

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
        mat4x4 viewProj = projection * view;

        PushRenderProgramUse(commandQueue, state->program->id);

        // Floor
        {
            mat4x4 translate = Translate(Identity(), { 0.0f, 0.0f, 0.0f });
            mat4x4 scale     = Scale(Identity(), 30.0f);
            mat4x4 model     = translate * scale;

            PushRenderUploadUniformMat4x4(commandQueue, state->program->id, "mvp", viewProj * model);
            PushRenderUploadUniformVec4(commandQueue, state->program->id, "color", { 1.0f, 1.0f, 1.0f, 0.5f });
            PushRenderDrawBuffer(commandQueue, state->planeBuffer);
        }

        // Player
        {
            mat4x4 translate = Translate(Identity(), { player->position.x, player->height, player->position.z });
            mat4x4 rotate    = Rotate(Identity(), player->yaw, { 0.0f, 1.0f, 0.0f });
            mat4x4 scale     = Scale(Identity(), player->height);

            mat4x4 model = translate * rotate * scale;

            PushRenderUploadUniformMat4x4(commandQueue, state->program->id, "mvp", viewProj * model);
            PushRenderUploadUniformVec4(commandQueue, state->program->id, "color", blue);
            PushRenderDrawBuffer(commandQueue, state->cubeBuffer);
        }
    }

    RendererFrameEnd(opengl);

#ifdef BUILD_TYPE_DEBUG
    DebugFrameBegin(state->debug, opengl, projection * view);
    {
        v3 playerTarget{ sinf(player->yaw), 0.0f, cosf(player->yaw) };

        DebugDrawLine(state->debug, opengl, v3{ player->position.x, player->height, player->position.z },
                      v3{ player->position.x, player->height, player->position.z } + (playerTarget * 1.5f), blue);

        DebugDrawAABB(state->debug, opengl, testAABBPosition, 0, -testHalfExtend, testHalfExtend, red);
        DebugDrawAABB(state->debug, opengl, { player->position.x, player->height, player->position.z }, player->yaw,
                      -playerHalfExtend, playerHalfExtend, green);
    }
    DebugFrameEnd(state->debug, opengl);
#endif
    return 0;
}