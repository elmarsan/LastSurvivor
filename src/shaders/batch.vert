#version 330 core

smooth out vec2  uv;
smooth out vec4  color;
flat   out int   textureIndex;

layout (location = 0) in vec3  aWorldPos;
layout (location = 1) in vec2  aUv;
layout (location = 2) in vec4  aColor;
layout (location = 3) in int   aTextureIndex;

uniform mat4 viewProj;

void main()
{
    uv           = aUv;
    color        = aColor;
    textureIndex = aTextureIndex;
    gl_Position  = viewProj * vec4(aWorldPos, 1.0f);
}