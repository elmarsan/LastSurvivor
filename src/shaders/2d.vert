#version 330 core

out vec2 uv;

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUv;

uniform mat4 mvp;

void main()
{
	uv = aUv;
	gl_Position = mvp * vec4(aPos, 0.0f, 1.0f);
}