#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

out vec2 UV;

uniform mat4 world;
uniform mat4 viewProj;

void main()
{
    UV          = aUV;
    gl_Position = viewProj * world * vec4(aPos, 1.0f);
}