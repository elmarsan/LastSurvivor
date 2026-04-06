#pragma once

// Limitations
// Only one scene
// Only one skeleton
// Only one .bin buffer
// Materials (only pbr albedo textures)

enum GLTFAnimationChannelPath
{
    GLTFAnimationChannelPath_Translation,
    GLTFAnimationChannelPath_Rotation,
    GLTFAnimationChannelPath_Scale
};

enum GLTFAnimationSamplerInterpolation
{
    GLTFAnimationSamplerInterpolation_Linear,
    GLTFAnimationSamplerInterpolation_Step,
    GLTFAnimationSamplerInterpolation_CubicSpline
};

struct GLTFAnimationChannel
{
    int                      samplerIndex;
    int                      nodeIndex;
    GLTFAnimationChannelPath path;
};

struct GLTFAnimationSampler
{
    std::vector<f32>                  times;
    std::vector<glm::vec4>            transformations;
    GLTFAnimationSamplerInterpolation interpolation;
};

struct GLTFAnimation
{
    char                              name[256];
    f32                               duration;
    std::vector<GLTFAnimationChannel> channels;
    std::vector<GLTFAnimationSampler> samplers;
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
    glm::vec3        translation;
    glm::quat        rotation;
    glm::vec3        scale;
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
Skeleton*                  GLTFConvertSkeleton(GLTFModel* model, Arena* arena);
Animation*                 GLTFConvertAnimation(GLTFModel* model, GLTFAnimation* animation, Arena* arena);