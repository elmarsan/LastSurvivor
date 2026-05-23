#version 330 core

layout (location = 0) in vec3  aPos;
layout (location = 1) in vec3  aNormal;
layout (location = 2) in vec2  aUV;
layout (location = 3) in uvec4 aJoints;
layout (location = 4) in vec4  aWeights;

out vec2 UV;

uniform mat4 world;
uniform mat4 viewProj;

#define MAX_JOINTS 100
uniform mat4 uJoints[MAX_JOINTS];

void main()
{
    UV = aUV;

    mat4 skinMatrix = aWeights.x * uJoints[aJoints.x]
        + aWeights.y * uJoints[aJoints.y]
        + aWeights.z * uJoints[aJoints.z]
        + aWeights.w * uJoints[aJoints.w];

    vec4 skinnedPos = skinMatrix * vec4(aPos, 1.0);
    gl_Position     = viewProj * world * skinnedPos;
}