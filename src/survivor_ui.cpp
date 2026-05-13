// TODO
// - Autogenerate node ids
// - Styling
// - Text size in pixels
// - Check alignment

internal void     UI_DrawNode(UI* ui, UI_Node* node, glm::vec2 origin);
internal void     UI_NodeGrowChildElements(UI_Node* parent);
internal UI_Node* UI_CopyNode(Arena* arena, UI_Node* src);

void UI_Init(UI* ui, Arena* arena, Renderer* renderer, PlatformAPI* platform)
{
    size_t size = Kilobytes(128);
    SubArena(&ui->transientArena, arena, size);
    SubArena(&ui->cacheArena, arena, size);
    ui->renderer                 = renderer;
    ui->platform                 = platform;
    ui->gamepadSelectedNodeIndex = -1;
}

void UI_BeginFrame(UI* ui, GameController* controller)
{
    ArenaClear(&ui->transientArena);
    ui->controller = controller;

    if (ui->controller->type == ControllerType_Gamepad && ui->prevFrameRoot && ui->navCount > 0)
    {
        f32 stickTreshold = 0.4f;
        b32 leftStickDown = ui->gamepadLeftStickUntouched && ui->controller->gamepad.leftStick.y <= -stickTreshold;
        b32 leftStickUp   = ui->gamepadLeftStickUntouched && ui->controller->gamepad.leftStick.y >= stickTreshold;

        if (ButtonIsPressed(ui->controller->moveDown) || leftStickDown)
        {
            if (ui->gamepadSelectedNodeIndex == -1)
            {
                ui->gamepadSelectedNodeIndex = 0;
            }
            else if (++ui->gamepadSelectedNodeIndex >= ui->navCount)
            {
                ui->gamepadSelectedNodeIndex = 0;
            }

            ui->gamepadLeftStickUntouched = false;
        }
        else if (ButtonIsPressed(ui->controller->moveUp) || leftStickUp)
        {
            s32 lastNodeIndex = ui->navCount - 1;

            if (ui->gamepadSelectedNodeIndex == -1)
            {
                ui->gamepadSelectedNodeIndex = lastNodeIndex;
            }
            else if (--ui->gamepadSelectedNodeIndex < 0)
            {
                ui->gamepadSelectedNodeIndex = lastNodeIndex;
            }

            ui->gamepadLeftStickUntouched = false;
        }

        if (ui->controller->gamepad.leftStick.x == 0.0f && ui->controller->gamepad.leftStick.y == 0.0f)
        {
            ui->gamepadLeftStickUntouched = true;
        }
    }
}

internal UI_Node* UI_CopyNode(Arena* arena, UI_Node* src)
{
    if (!src)
    {
        return 0;
    }

    UI_Node* dst   = PushStruct(arena, UI_Node);
    dst->id        = StrCopy(arena, src->id);
    dst->width     = src->width;
    dst->height    = src->height;
    dst->navigable = src->navigable;
    dst->position  = src->position;

    // Process children
    dst->firstChild  = UI_CopyNode(arena, src->firstChild);
    dst->nextSibling = UI_CopyNode(arena, src->nextSibling);

    return dst;
}

internal UI_Node* FindNode(UI_Node* root, char* id)
{
    if (!root)
    {
        return 0;
    }

    if (StrEquals(root->id, id))
    {
        return root;
    }

    UI_Node* child = root->firstChild;
    while (child)
    {
        UI_Node* result = FindNode(child, id);
        if (result)
        {
            return result;
        }

        child = child->nextSibling;
    }

    return 0;
}

void UI_EndFrame(UI* ui)
{
    if (ui->root)
    {
        UI_DrawNode(ui, ui->root, { 0.0f, 0.0f });
    }

    // Copy last frame tree
    ArenaClear(&ui->cacheArena);
    ui->prevFrameRoot = UI_CopyNode(&ui->cacheArena, ui->root);
    ui->navCount      = 0;
    for (UI_Node* child = ui->prevFrameRoot->firstChild; child; child = child->nextSibling)
    {
        if (child->navigable)
        {
            ui->navNodes[ui->navCount++] = child;
        }
    }
}

// Position calculations
internal void UI_DrawNode(UI* ui, UI_Node* node, glm::vec2 origin)
{
    Renderer* renderer = ui->renderer;

    node->position = origin;
    glm::vec2 nodeSize{ node->width.value, node->height.value };
    if (node->text)
    {
        DrawText(renderer, node->text->ptr, node->position, node->bgColor, node->text->scale);
    }
    else
    {
        DrawRect(renderer, node->position, nodeSize, node->bgColor);

        if (node->borderSize.top >= 0.0f)
        {
            DrawRect(renderer, node->position, { nodeSize.x, node->borderSize.top }, node->borderColor);
        }
        if (node->borderSize.bottom >= 0.0f)
        {
            DrawRect(renderer, { node->position.x, node->position.y + nodeSize.y - node->borderSize.bottom },
                     { nodeSize.x, node->borderSize.bottom }, node->borderColor);
        }
        if (node->borderSize.left >= 0.0f)
        {
            DrawRect(renderer, node->position, { node->borderSize.left, nodeSize.y }, node->borderColor);
        }
        if (node->borderSize.right >= 0.0f)
        {
            DrawRect(renderer, { node->position.x + nodeSize.x - node->borderSize.right, node->position.y },
                     { node->borderSize.right, nodeSize.y }, node->borderColor);
        }
    }

    UI_NodeGrowChildElements(node);

    origin.x += node->padding.left;
    origin.y += node->padding.top;

    // -----------------------------------------------
    // X Alignment
    if (node->alignX == UI_Align_Center)
    {
        f32 remainingWidth = node->width.value - (node->padding.left + node->padding.right);

        if (node->direction == UI_Direction_LeftToRight)
        {
            UI_Node* child = node->firstChild;
            while (child)
            {
                remainingWidth -= child->width.value;

                child = child->nextSibling;
            }
        }

        if (node->direction == UI_Direction_TopToBottom)
        {
            remainingWidth -= node->firstChild->width.value;
        }

        origin.x += remainingWidth / 2.0f;
    }
    // -----------------------------------------------

    // -----------------------------------------------
    // Y Alignment
    if (node->alignY == UI_Align_Center)
    {
        f32 remainingHeight = node->height.value - (node->padding.top + node->padding.bottom);

        if (node->direction == UI_Direction_TopToBottom)
        {
            UI_Node* child = node->firstChild;
            while (child)
            {
                remainingHeight -= child->height.value;
                if (child->nextSibling)
                {
                    remainingHeight -= node->childGap;
                }

                child = child->nextSibling;
            }
        }

        if (node->direction == UI_Direction_LeftToRight)
        {
            // Only one row → take tallest child
            f32 maxHeight = 0.0f;

            UI_Node* child = node->firstChild;
            while (child)
            {
                maxHeight = Max(maxHeight, child->height.value);
                child     = child->nextSibling;
            }

            remainingHeight -= maxHeight;
        }

        origin.y += remainingHeight / 2.0f;
    }

    UI_Node* child = node->firstChild;
    while (child)
    {
        UI_DrawNode(ui, child, origin);

        if (node->direction == UI_Direction_LeftToRight)
        {
            origin.x += child->width.value;

            // Clamp to next line
            if ((origin.x + child->width.value) > nodeSize.x)
            {
                origin.y += child->height.value + node->childGap;
                origin.x = node->position.x;
            }
        }
        else if (node->direction == UI_Direction_TopToBottom)
        {
            origin.y += child->height.value;

            // Clamp to the right
            if ((origin.y + child->height.value) > nodeSize.y)
            {
                origin.x += child->width.value + node->childGap;
                origin.y = node->position.y;
            }
        }

        if (child->nextSibling)
        {
            if (node->direction == UI_Direction_LeftToRight)
            {
                origin.x += node->childGap;
            }
            else
            {
                origin.y += node->childGap;
            }
        }

        child = child->nextSibling;
    }
}

internal void UI_NodeGrowChildElements(UI_Node* parent)
{
    f32 remainingWidth  = parent->width.value;
    f32 remainingHeight = parent->height.value;
    remainingWidth -= parent->padding.left + parent->padding.right;
    remainingHeight -= parent->padding.top + parent->padding.bottom;

    UI_Node* child         = parent->firstChild;
    u32      childrenCount = 0;
    while (child)
    {
        childrenCount++;
        child = child->nextSibling;
    }

    if (parent->direction == UI_Direction_LeftToRight)
    {
        remainingWidth -= (childrenCount - 1) * parent->childGap;
    }
    else if (parent->direction == UI_Direction_TopToBottom)
    {
        remainingHeight -= (childrenCount - 1) * parent->childGap;
    }

    child = parent->firstChild;
    while (child)
    {
        if (child->width.mode == UI_SizeMode_Grow)
        {
            child->width.value += remainingWidth;
        }
        if (child->height.mode == UI_SizeMode_Grow)
        {
            child->height.value += remainingHeight;
        }

        if (child->width.mode == UI_SizeMode_Percentage)
        {
            f32 width          = (remainingWidth * child->width.value) / 100.0f;
            child->width.value = width;
            remainingWidth -= width;
        }

        if (child->height.mode == UI_SizeMode_Percentage)
        {
            if (child->direction == UI_Direction_LeftToRight)
            {
                f32 height          = (remainingHeight * child->height.value) / 100.0f;
                child->height.value = height;
                remainingHeight -= height;
            }
            else if (child->direction == UI_Direction_TopToBottom)
            {
                child->height.value = (remainingHeight * child->height.value) / 100.0f;
            }
        }

        child = child->nextSibling;
    }
}

UI_Node* UI_BeginNode(UI* ui, char* id)
{
    Arena* arena = &ui->transientArena;

    UI_Node* element = PushStruct(arena, UI_Node);
    UI_Node* parent  = ui->current;

    element->parent      = parent;
    element->firstChild  = 0;
    element->lastChild   = 0;
    element->nextSibling = 0;
    element->id          = StrCopy(arena, id);

    if (parent)
    {
        if (!parent->firstChild)
        {
            parent->firstChild = element;
            parent->lastChild  = element;
        }
        else
        {
            parent->lastChild->nextSibling = element;
            parent->lastChild              = element;
        }
    }

    if (!ui->root)
    {
        ui->root = element;
    }

    ui->current = element;

    return element;
}

// Size calculations
void UI_EndNode(UI* ui)
{
    Assert(ui->current);
    UI_Node* element = ui->current;
    UI_Node* parent  = element->parent;

    UI_Padding padding = element->padding;
    element->width.value += padding.left + padding.right;
    element->height.value += padding.top + padding.bottom;

    if (parent)
    {
        f32 childGap = 0.0f;
        if (parent->firstChild != element)
        {
            childGap = parent->childGap;
        }

        // Fit width
        if (parent->width.mode == UI_SizeMode_Fit)
        {
            if (parent->direction == UI_Direction_LeftToRight)
            {
                parent->width.value += element->width.value + childGap;

                // TODO: Hack ?
                if (parent->parent)
                {
                    parent->width.value += parent->parent->padding.right + parent->parent->padding.left;
                }
            }
            else if (parent->direction == UI_Direction_TopToBottom)
            {
                parent->width.value = Max(element->width.value, parent->width.value);
            }
        }

        // Fit height
        if (parent->height.mode == UI_SizeMode_Fit)
        {
            if (parent->direction == UI_Direction_LeftToRight)
            {
                parent->height.value = Max(element->height.value, parent->height.value);

                // TODO: Hack ?
                if (parent->parent)
                {
                    parent->height.value += parent->parent->padding.bottom + parent->parent->padding.top;
                }
            }
            else if (parent->direction == UI_Direction_TopToBottom)
            {
                parent->height.value += element->height.value + childGap;
            }
        }

        if (element->width.mode == UI_SizeMode_Fixed)
        {
            if (element->width.value >= parent->width.value)
            {
                element->width.value = parent->width.value;
            }
        }
    }

    ui->current = parent ? parent : 0;
}

glm::vec2 UI_GetTextSize(UI* ui, char* text, f32 scale)
{
    Renderer* renderer = ui->renderer;

    glm::vec2 size{ 0.0f, 0.0f };

    char* textPtr = text;
    while (*text)
    {
        TTFGlyph* ttfChar = &renderer->ttfChars[*text++ - TTF_FIRST_GLYPH_OFFSET];

        size.y = Max(size.y, (ttfChar->y1 - ttfChar->y0) * scale);
        size.x += (ttfChar->xadvance * scale) + (ttfChar->xoff * scale);
    }
    text = textPtr;

    return size;
}

void UI_Text(UI* ui, char* text, glm::vec4 color, f32 scale)
{
    glm::vec2 textSize = UI_GetTextSize(ui, text, scale);

    UI_Node* textNode = UI_BeginNode(ui, text);
    {
        textNode->bgColor     = color;
        textNode->width       = UI_FIXED(textSize.x);
        textNode->height      = UI_FIXED(textSize.y);
        textNode->text        = PushStruct(&ui->transientArena, UI_TextElement);
        textNode->text->ptr   = StrCopy(&ui->transientArena, text);
        textNode->text->scale = scale;
    }
    UI_EndNode(ui);
}

b32 UI_Button(UI* ui, char* id, char* text, f32 scale, glm::vec2 size)
{
    b32 pressed = false;
    b32 hover   = false;

    UI_Node* lastFrameBtn = FindNode(ui->prevFrameRoot, id);
    if (lastFrameBtn)
    {
        f32 x      = lastFrameBtn->position.x;
        f32 y      = lastFrameBtn->position.y;
        f32 width  = lastFrameBtn->width.value;
        f32 height = lastFrameBtn->height.value;

        switch (ui->controller->type)
        {
        case ControllerType_Keyboard:
        {
            Mouse* mouse = &ui->controller->mouse;

            f32 cursorX = (f32)mouse->pos.x;
            f32 cursorY = (f32)mouse->pos.y;

            if (cursorX >= x && cursorX <= (x + width) && cursorY >= y && cursorY <= (y + height))
            {
                hover   = true;
                pressed = ButtonIsPressed(mouse->left);
            }
            break;
        }
        case ControllerType_Gamepad:
        {
            if (ui->gamepadSelectedNodeIndex != -1)
            {
                UI_Node* selectedWidget = ui->navNodes[ui->gamepadSelectedNodeIndex];
                if (StrEquals(selectedWidget->id, id))
                {
                    hover   = true;
                    pressed = ButtonIsPressed(ui->controller->actionDown);
                }
            }
            break;
        }
            InvalidDefaultCase;
        }
    }

    UI_Border borderSize{ 1.0f, 1.0f, 1.0f, 1.0f };
    if (!hover)
    {
        borderSize = { 0.0f, 0.0f, 0.0f, 0.0f };
    }

    UI_Node* btnRect     = UI_BeginNode(ui, id);
    btnRect->padding     = { 12.0f, 12.0f, 12.0f, 12.0f };
    btnRect->borderSize  = borderSize;
    btnRect->borderColor = ui_color_border;
    btnRect->width       = UI_FIXED(size.x);
    btnRect->height      = UI_FIXED(size.y);
    btnRect->alignX      = UI_Align_Center;
    btnRect->alignY      = UI_Align_Center;
    btnRect->navigable   = true;
    {
        UI_Text(ui, text, hover ? color_white : ui_color_gray, scale);
    }
    UI_EndNode(ui);

    return pressed;
}