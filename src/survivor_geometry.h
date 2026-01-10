// clang-format off
internal f32 quad2DVertexs[] = {
    1.0f,   1.0f, 1.0f, 1.0f, // top-right
    1.0f,  -1.0f, 1.0f, 0.0f, // bottom-right
    -1.0f, -1.0f, 0.0f, 0.0f, // bottom-left
    -1.0f,  1.0f, 0.0f, 1.0f  // top-left
}; 

internal u32 quad2DIndices[] = {
    0, 1, 2, 0, 2, 3
};
// clang-format on