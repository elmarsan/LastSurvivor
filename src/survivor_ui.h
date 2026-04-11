#pragma once

#define MAX_WIDGETS 32

#define ui_color_gray   glm::vec4{ 0x77 / 255.0f, 0x77 / 255.0f, 0x77 / 255.0f, 1.0f }
#define ui_color_border glm::vec4{ 0xA8 / 255.0f, 0xA8 / 255.0f, 0xA8 / 255.0f, 1.0f }

enum UI_SizeMode
{
    UI_SizeMode_Fixed,
    UI_SizeMode_Fit,
    UI_SizeMode_Grow,
    UI_SizeMode_Percentage
};

#define UI_FIXED(v)      { ((f32)v), UI_SizeMode_Fixed }
#define UI_FIT()         { 0.0f, UI_SizeMode_Fit }
#define UI_GROW()        { 0.0f, UI_SizeMode_Grow }
#define UI_PERCENTAGE(v) { (f32)v, UI_SizeMode_Percentage }

enum UI_Direction
{
    UI_Direction_LeftToRight,
    UI_Direction_TopToBottom
};

enum UI_Align
{
    UI_Align_Left,
    UI_Align_Center,
    UI_Align_Right,
};

struct UI_Padding
{
    f32 top;
    f32 bottom;
    f32 left;
    f32 right;
};

struct UI_Border
{
    f32 top;
    f32 bottom;
    f32 left;
    f32 right;
};

struct UI_Size
{
    f32         value;
    UI_SizeMode mode;
};

struct UI_TextElement
{
    char* ptr;
    f32   scale;
};

struct UI_Node
{
    UI_Node* parent;
    UI_Node* firstChild;
    UI_Node* lastChild;
    UI_Node* nextSibling;

    char* id;

    glm::vec4       bgColor     = color_transparent;
    UI_Padding      padding     = { 0.0f, 0.0f, 0.0f, 0.0f };
    UI_Border       borderSize  = { 0.0f, 0.0f, 0.0f, 0.0f };
    glm::vec4       borderColor = color_white;
    UI_Size         width       = UI_FIT();
    UI_Size         height      = UI_FIT();
    UI_Direction    direction   = UI_Direction_LeftToRight;
    f32             childGap    = 0.0f;
    UI_TextElement* text;
    glm::vec2       position;
    UI_Align        alignX = UI_Align_Left;
    UI_Align        alignY = UI_Align_Left;

    b32 navigable;
};

struct UI
{
    Arena                cacheArena;
    Arena                transientArena;
    Renderer*            renderer;
    PlatformAPI*         platform;
    UI_Node*             root;
    UI_Node*             current;
    UI_Node*             prevFrameRoot;
    GameInputController* controller;
    UI_Node*             navNodes[MAX_WIDGETS];
    s32                  navCount;
    s32                  gamepadSelectedNodeIndex;
    b32                  gamepadLeftStickUntouched;
};

void      UI_Init(UI* ui, Arena* arena, Renderer* renderer, PlatformAPI* platform);
void      UI_BeginFrame(UI* ui, GameInputController* controller);
void      UI_EndFrame(UI* ui);
UI_Node*  UI_BeginNode(UI* ui, char* id);
void      UI_EndNode(UI* ui);
void      UI_Text(UI* ui, char* text, glm::vec4 color, f32 scale);
b32       UI_Button(UI* ui, char* id, char* text, f32 scale);
glm::vec2 UI_GetTextSize(UI* ui, char* text, f32 scale);