#version 330 core

smooth in vec2  uv;
smooth in vec4  color;
flat   in int   textureIndex;

uniform sampler2D textureArray[2];

out vec4 FragColor;

void main()
{
    vec4 texelColor = texture(textureArray[textureIndex], uv);
    FragColor       = texelColor * color;
}