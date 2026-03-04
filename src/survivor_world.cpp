v3 WorldMousePicking(Camera* camera, mat4x4 projection, v2u windowDim, v2u mouse)
{
    f32 screenWidth  = (f32)windowDim.w;
    f32 screenHeight = (f32)windowDim.h;
    f32 mouseX       = (f32)mouse.x;
    f32 mouseY       = (f32)mouse.y;

    mat4x4 inverseProjection = Inverse(projection);
    mat4x4 inverseView       = Inverse(CameraView(camera));

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
    f32 t     = -(camera->position.y / rayWorld.y);
    v3  point = camera->position + rayWorld * t;

    return point;
}

b32 WorldIsPositionInBounds(v3 position)
{
    // Left limit
    if (position.x < -(GRID_ROWS * 0.5))
    {
        return false;
    }
    // Right limit
    if (position.x > (GRID_ROWS * 0.5))
    {
        return false;
    }
    // Top limit
    if (position.z < -(GRID_COLS * 0.5))
    {
        return false;
    }
    // Bottom limit
    if (position.z > (GRID_COLS * 0.5))
    {
        return false;
    }

    return true;
}