#version 330 core

in  vec2 UV;
out vec4 FragColor;

uniform bool      hasDiffuse;
uniform sampler2D diffuseMap;
uniform vec4      color;

void main()
{
    if (hasDiffuse)
    {
        FragColor = texture(diffuseMap, UV);
    }
    else
    {
        FragColor = color;
    }
}