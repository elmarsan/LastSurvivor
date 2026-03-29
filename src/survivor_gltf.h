#pragma once

// Limitations
// Only one scene
// Only one skeleton
// Only one .bin buffer
// Materials (only pbr albedo textures)

enum AnimationChannelPath
{
    AnimationChannelPath_Translation,
    AnimationChannelPath_Rotation,
    AnimationChannelPath_Scale
};

enum AnimationSamplerInterpolation
{
    AnimationSamplerInterpolation_Linear,
    AnimationSamplerInterpolation_Step,
    AnimationSamplerInterpolation_CubicSpline
};

struct AnimationChannel
{
    int                  samplerIndex;
    int                  nodeIndex;
    AnimationChannelPath path;
};

struct AnimationSampler
{
    std::vector<float>            times;
    std::vector<glm::vec4>        transformations;
    AnimationSamplerInterpolation interpolation;
};

struct GLTFAnimation
{
    char                          name[256];
    f32                           duration;
    std::vector<AnimationChannel> channels;
    std::vector<AnimationSampler> samplers;
};

struct GLTFMaterial
{
    int albedoTextureIndex;
};

struct GLTFTexture
{
    char path[256];
};

struct GLTFMeshPrimitive
{
    std::vector<Vertex>   vertexs;
    std::vector<uint32_t> indices;
    int                   materialIndex;
};

struct GLTFMesh
{
    std::vector<GLTFMeshPrimitive> primitives;
};

struct GLTFNode
{
    char             name[256];
    int              meshIndex;
    int              parentIndex;
    std::vector<int> childrenIndexes;
    glm::mat4        inverseBindMatrix;
    glm::vec3        bindTranslation;
    glm::quat        bindRotation;
    glm::vec3        bindScale;
    glm::vec3        localTranslation;
    glm::quat        localRotation;
    glm::vec3        localScale;
};

struct GLTFSkeleton
{
    char                   name[256];
    std::vector<int>       joints;
    std::vector<glm::mat4> jointMatrices;
    int                    meshNodeIndex;
};

struct GLTFModel
{
    std::vector<int>          rootNodeIndexes;
    std::vector<GLTFNode>     nodes;
    std::vector<GLTFMesh>     meshes;
    std::vector<GLTFMaterial> materials;
    std::vector<GLTFTexture>  textures;
    GLTFSkeleton              skeleton;
    std::vector<glm::mat4>    nodeGlobalMatrices;
};

GLTFModel                  GLTFParse(char* gltfFilename, PlatformAPI* platform);
std::vector<GLTFAnimation> GLTFParseAnimations(char* gltfFilename, PlatformAPI* platform);