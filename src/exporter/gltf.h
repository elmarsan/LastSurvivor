#pragma once

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define DEBUG_TEXTURES 1
#if DEBUG_TEXTURES
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#define EMPTY -1

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
    int baseColorIndex;
};

struct GLTFTexture
{
    char            name[64];
    std::vector<u8> data;
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
    glm::mat4        transform;
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
Skeleton*                  ExtractGLTFSkeleton(GLTFModel* model, Arena* arena);
Animation*                 GLTFConvertAnimation(GLTFModel* model, GLTFAnimation* animation, Arena* arena);

GLTFModel GLTFParse(char* gltfFilename, PlatformAPI* platform)
{
    GLTFModel result;

    // Log("------------------------------------------------------------");
    // Log("Reading .gltf file: '%s'", gltfFilename);

    FileReadResult gltfFile = platform->FileReadEntire(gltfFilename);
    FileReadResult binFile  = { 0 };

    if (!gltfFile.content)
    {
        Log("Unable to parse .gltf file '%s' not found", gltfFilename);
        Assert(0);
    }

    cgltf_options options   = {};
    cgltf_data*   cgltfData = nullptr;

    cgltf_result cgltfResult = cgltf_parse(&options, gltfFile.content, gltfFile.contentSize, &cgltfData);
    if (cgltfResult == cgltf_result_success)
    {
        size_t directoryLen = GetParentPathLength(gltfFilename);
        char*  extension    = GetFilenameExtension(gltfFilename);

        // Parse buffers
        if (StrEquals(extension, "glb"))
        {
            cgltfResult = cgltf_load_buffers(&options, cgltfData, gltfFilename);
            if (cgltfResult != cgltf_result_success)
            {
                Log("Failed to load .glb buffers '%s'", gltfFilename);
                Assert(0);
            }

            // cgltf_buffer* buffer = &cgltfData->buffers[0];
            // buffer->data = gltfFile.content;
            // buffer->size = gltfFile.contentSize;
        }
        else if (cgltfData->buffers_count > 0) /* .gltf */
        {
            Assert(cgltfData->buffers_count == 1);
            cgltf_buffer* buffer = &cgltfData->buffers[0];

            char binFilename[256];
            memcpy(binFilename, gltfFilename, directoryLen);
            strcpy(binFilename + directoryLen, buffer->uri);

            // Log("Reading .bin file: '%s'", binFilename);

            binFile = platform->FileReadEntire(binFilename);
            if (!binFile.content)
            {
                Log("Unable to parse .bin file '%s'", binFilename);
                Assert(0);
            }
            else
            {
                buffer->data = binFile.content;
                buffer->size = binFile.contentSize;
            }
        }

        cgltfResult = cgltf_validate(cgltfData);
        if (cgltfResult != cgltf_result_success)
        {
            Log("Invalid gltf '%s'", gltfFilename);
            Assert(0);
        }

        cgltf_scene* scene = cgltfData->scene;
        if (cgltfData->scenes_count > 1)
        {
            Log("More than one scene %zu", cgltfData->scenes_count);
            assert(0);
        }

        // Parse root nodes indexes
        {
            result.rootNodeIndexes.resize(scene->nodes_count);
            for (cgltf_size rootIndex = 0; rootIndex < scene->nodes_count; rootIndex++)
            {
                result.rootNodeIndexes[rootIndex] = (int)cgltf_node_index(cgltfData, scene->nodes[rootIndex]);
            }
        }

        // Parse nodes
        {
            result.nodes.resize(cgltfData->nodes_count);
            result.nodeGlobalMatrices.resize(cgltfData->nodes_count);
            for (cgltf_size nodeIndex = 0; nodeIndex < cgltfData->nodes_count; nodeIndex++)
            {
                GLTFNode&   node      = result.nodes[nodeIndex];
                cgltf_node* cgltfNode = &cgltfData->nodes[nodeIndex];

                if (cgltfNode->name)
                {
                    strcpy(node.name, cgltfNode->name);
                }
                else
                {
                    sprintf(node.name, "%s", "Unnamed node");
                }

                glm::vec3 translation{ 0.0f, 0.0f, 0.0f };
                glm::quat rotation{ 0.0f, 0.0f, 0.0f, 0.0f };
                glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

                if (cgltfNode->has_translation)
                {
                    memcpy(&translation.x, cgltfNode->translation, 3 * sizeof(f32));
                }
                if (cgltfNode->has_rotation)
                {
                    glm::vec4 rotationVec;
                    memcpy(&rotationVec.x, cgltfNode->rotation, 4 * sizeof(f32));

                    rotation = glm::quat{ rotationVec.x, rotationVec.y, rotationVec.z, rotationVec.w };
                }
                if (cgltfNode->has_scale)
                {
                    memcpy(&scale.x, cgltfNode->scale, 3 * sizeof(f32));
                }
                if (cgltfNode->has_matrix)
                {
                    glm::mat4 localTransform;
                    memcpy(&localTransform[0][0], cgltfNode->matrix, 16 * sizeof(f32));

                    glm::vec3 skew;
                    glm::vec4 perspective;

                    glm::decompose(localTransform, scale, rotation, translation, skew, perspective);

                    node.transform = localTransform;
                }

                node.translation = translation;
                node.rotation    = rotation;
                node.scale       = scale;

                node.inverseBindMatrix = glm::mat4{ 1.0f };

                node.childrenIndexes.resize(cgltfNode->children_count);
                for (cgltf_size childIndex = 0; childIndex < cgltfNode->children_count; childIndex++)
                {
                    node.childrenIndexes[childIndex] =
                        (int)cgltf_node_index(cgltfData, cgltfNode->children[childIndex]);
                }

                // Mesh
                if (cgltfNode->mesh)
                {
                    node.meshIndex = (int)cgltf_mesh_index(cgltfData, cgltfNode->mesh);
                }
                else
                {
                    node.meshIndex = EMPTY;
                }
                // Skin
                if (cgltfNode->skin)
                {
                    result.skeleton.meshNodeIndex = (int)cgltf_skin_index(cgltfData, cgltfNode->skin);
                }
                // Parent
                if (cgltfNode->parent)
                {
                    node.parentIndex = (int)cgltf_node_index(cgltfData, cgltfNode->parent);
                }
                else
                {
                    node.parentIndex = EMPTY;
                }
            }
        }

        // Parse meshes
        {
            result.meshes.resize(cgltfData->meshes_count);
            for (cgltf_size meshIndex = 0; meshIndex < cgltfData->meshes_count; meshIndex++)
            {
                cgltf_mesh* cgltfMesh = &cgltfData->meshes[meshIndex];
                GLTFMesh&   mesh      = result.meshes[meshIndex];

                mesh.primitives.resize(cgltfMesh->primitives_count);
                for (cgltf_size primitiveIndex = 0; primitiveIndex < cgltfMesh->primitives_count; primitiveIndex++)
                {
                    cgltf_attribute* cgltfPosition = 0;
                    cgltf_attribute* cgltfNormal   = 0;
                    cgltf_attribute* cgltfTexCoord = 0;
                    cgltf_attribute* cgltfJoints   = 0;
                    cgltf_attribute* cgltfWeights  = 0;

                    cgltf_primitive*   cgltfPrimitive = &cgltfMesh->primitives[primitiveIndex];
                    GLTFMeshPrimitive& meshPrimitive  = mesh.primitives[primitiveIndex];

                    for (cgltf_size attributeIndex = 0; attributeIndex < cgltfPrimitive->attributes_count;
                         attributeIndex++)
                    {
                        cgltf_attribute* cgltfAttribute = &cgltfPrimitive->attributes[attributeIndex];

                        switch (cgltfAttribute->type)
                        {
                        case cgltf_attribute_type_position:
                        {
                            cgltfPosition = cgltfAttribute;
                            break;
                        }
                        case cgltf_attribute_type_normal:
                        {
                            cgltfNormal = cgltfAttribute;
                            break;
                        }
                        case cgltf_attribute_type_texcoord:
                        {
                            cgltfTexCoord = cgltfAttribute;
                            break;
                        }
                        case cgltf_attribute_type_joints:
                        {
                            cgltfJoints = cgltfAttribute;
                            break;
                        }
                        case cgltf_attribute_type_weights:
                        {
                            cgltfWeights = cgltfAttribute;
                            break;
                        }
                        default:
                        {
                            Log("Unsupported primitive attribute: %s", cgltfAttribute->name);
                            break;
                        }
                        }
                    }

                    // Geometry requires position attribute
                    Assert(cgltfPosition);

                    cgltf_size vertexCount  = cgltfPosition->data->count;
                    cgltf_size indicesCount = cgltfPrimitive->indices->count;

                    meshPrimitive.vertexs.resize(vertexCount);
                    for (cgltf_size vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
                    {
                        Vertex& vertex = meshPrimitive.vertexs[vertexIndex];

                        if (!cgltf_accessor_read_float(cgltfPosition->data, vertexIndex, &vertex.position.x, 3))
                        {
                            vertex.position = { 0.0f, 0.0f, 0.0f };
                        }
                        if (cgltfNormal &&
                            !cgltf_accessor_read_float(cgltfNormal->data, vertexIndex, &vertex.normal.x, 3))
                        {
                            vertex.normal = { 0.0f, 0.0f, 0.0f };
                        }
                        if (cgltfTexCoord &&
                            !cgltf_accessor_read_float(cgltfTexCoord->data, vertexIndex, &vertex.uv.x, 2))
                        {
                            vertex.uv = { 0.0f, 0.0f };
                        }
                        if (cgltfJoints)
                        {
                            if (!cgltf_accessor_read_uint(cgltfJoints->data, vertexIndex, &vertex.joints.x, 4))
                            {
                                Assert(0);
                            }
                        }
                        else
                        {
                            vertex.joints = { 999, 999, 999, 999 };
                        }
                        if (cgltfWeights &&
                            !cgltf_accessor_read_float(cgltfWeights->data, vertexIndex, &vertex.weights.x, 4))
                        {
                            vertex.weights = { 0.0f, 0.0f, 0.0f, 0.0f };
                        }
                    }

                    meshPrimitive.indices.resize(indicesCount);
                    for (cgltf_size index = 0; index < indicesCount; index++)
                    {
                        meshPrimitive.indices[index] =
                            (uint32_t)cgltf_accessor_read_index(cgltfPrimitive->indices, index);
                    }

                    if (cgltfPrimitive->material)
                    {
                        meshPrimitive.materialIndex = (int)cgltf_material_index(cgltfData, cgltfPrimitive->material);
                    }
                    else
                    {
                        meshPrimitive.materialIndex = EMPTY;
                    }
                }
            }
        }

        // Parse skeleton
        {
            if (cgltfData->skins_count)
            {
                if (cgltfData->skins_count > 1)
                {
                    Log("Model has %zu skins, only first one is processed", cgltfData->skins_count);
                }

                cgltf_skin*   cgltfSkin = &cgltfData->skins[0];
                GLTFSkeleton& skeleton  = result.skeleton;

                if (cgltfSkin->name)
                {
                    strcpy(skeleton.name, cgltfSkin->name);
                }
                else
                {
                    sprintf(skeleton.name, "%s", "skeleton_0");
                }

                skeleton.jointMatrices.resize(cgltfSkin->joints_count);
                skeleton.joints.resize(cgltfSkin->joints_count);
                for (cgltf_size jointIndex = 0; jointIndex < cgltfSkin->joints_count; jointIndex++)
                {
                    cgltf_node* cgltfNode = cgltfSkin->joints[jointIndex];
                    int         nodeIndex = (int)cgltf_node_index(cgltfData, cgltfNode);
                    GLTFNode&   node      = result.nodes[nodeIndex];

                    skeleton.joints[jointIndex] = nodeIndex;

                    if (!cgltf_accessor_read_float(cgltfSkin->inverse_bind_matrices, jointIndex,
                                                   &node.inverseBindMatrix[0][0], 16))
                    {
                        Assert(0);
                    }
                }
            }
        }

        // Parse materials
        {
            if (cgltfData->materials_count > 0)
            {
                Log("Material count %zu", cgltfData->materials_count);

                result.materials.resize(cgltfData->materials_count);
                for (cgltf_size materialIndex = 0; materialIndex < cgltfData->materials_count; materialIndex++)
                {
                    cgltf_material* cgltfMaterial = &cgltfData->materials[materialIndex];
                    GLTFMaterial&   material      = result.materials[materialIndex];

                    if (cgltfMaterial->has_pbr_metallic_roughness)
                    {
                        if (cgltfMaterial->pbr_metallic_roughness.base_color_texture.texture)
                        {
                            cgltf_texture* baseColor = cgltfMaterial->pbr_metallic_roughness.base_color_texture.texture;
                            cgltf_size     textureIndex = cgltf_texture_index(cgltfData, baseColor);
                            material.baseColorIndex     = (int)textureIndex;
                        }
                    }
                    else
                    {
                        Log("Unsupported material, index %zu", materialIndex);
                    }
                }
            }
        }

        // Parse textures
        {
            if (cgltfData->textures_count > 0)
            {
                Log("Texture count %zu", cgltfData->textures_count);

                result.textures.resize(cgltfData->textures_count);
                for (cgltf_size textureIndex = 0; textureIndex < cgltfData->textures_count; textureIndex++)
                {
                    cgltf_texture* cgltfTexture = &cgltfData->textures[textureIndex];
                    Assert(cgltfTexture->image);

                    GLTFTexture& texture = result.textures[textureIndex];

                    // .gltf
                    if (cgltfTexture->image->uri)
                    {
                        char textureFilename[256];
                        memcpy(textureFilename, gltfFilename, directoryLen);
                        strcpy(textureFilename + directoryLen, cgltfTexture->image->uri);

                        Log("Loading texture %zu '%s'", textureIndex, textureFilename);

                        FileReadResult textureFile = platform->FileReadEntire(textureFilename);
                        if (textureFile.content)
                        {
                            texture.data.resize(textureFile.contentSize);
                            memcpy(texture.data.data(), textureFile.content, textureFile.contentSize);
                            platform->FileFree(textureFile.content);
                        }
                        else
                        {
                            Assert(0);
                        }
                    }
                    else if (cgltfTexture->image->buffer_view) /* .glb */
                    {
                        Log("Loading texture %s", cgltfTexture->image->name);

                        cgltf_buffer_view* view   = cgltfTexture->image->buffer_view;
                        cgltf_buffer*      buffer = view->buffer;

                        u8*    data = (u8*)buffer->data + view->offset;
                        size_t size = view->size;

#if DEBUG_TEXTURES
                        int width;
                        int height;
                        int channelCount;
                        u8* imageData = stbi_load_from_memory(data, (int)size, &width, &height, &channelCount, 0);
                        if (imageData)
                        {
                            int ok =
                                stbi_write_png(cgltfTexture->image->name, width, height, channelCount, imageData, 0);
                            if (!ok)
                            {
                                Assert(0);
                            }
                        }
                        else
                        {
                            Assert(0);
                        }
#endif
                        strcpy(texture.name, cgltfTexture->image->name);
                        texture.data.resize(size);
                        memcpy(texture.data.data(), data, size);
                    }
                    else
                    {
                        // Unsupported texture source
                        InvalidCodePath;
                    }
                }
            }
        }
    }
    else
    {
        Log("Unable to parse gltf '%s'", gltfFilename);
        Assert(0);
    }

    // Log("------------------------------------------------------------");
    platform->FileFree(gltfFile.content);
    if (binFile.content)
    {
        platform->FileFree(binFile.content);
    }

    return result;
}

std::vector<GLTFAnimation> GLTFParseAnimations(char* gltfFilename, PlatformAPI* platform)
{
    std::vector<GLTFAnimation> result;

    // Log("------------------------------------------------------------");
    // Log("Reading .gltf animations: '%s'", gltfFilename);

    FileReadResult gltfFile = platform->FileReadEntire(gltfFilename);
    FileReadResult binFile  = { 0 };

    if (!gltfFile.content)
    {
        Log("Unable to parse .gltf file '%s' not found", gltfFilename);
        Assert(0);
    }

    cgltf_options options   = {};
    cgltf_data*   cgltfData = nullptr;

    cgltf_result cgltfResult = cgltf_parse(&options, gltfFile.content, gltfFile.contentSize, &cgltfData);
    if (cgltfResult == cgltf_result_success)
    {
        size_t directoryLen = GetParentPathLength(gltfFilename);

        // Parse buffers
        if (cgltfData->buffers_count > 0)
        {
            Assert(cgltfData->buffers_count == 1);
            cgltf_buffer* buffer = &cgltfData->buffers[0];

            char binFilename[256];
            memcpy(binFilename, gltfFilename, directoryLen);
            strcpy(binFilename + directoryLen, buffer->uri);

            // Log("Reading .bin file: '%s'", binFilename);

            binFile = platform->FileReadEntire(binFilename);
            if (!binFile.content)
            {
                Log("Unable to parse .bin file '%s'", binFilename);
                Assert(0);
            }
            else
            {
                buffer->data = binFile.content;
                buffer->size = binFile.contentSize;
            }
        }

        // Parse animations
        {
            result.resize(cgltfData->animations_count);
            for (cgltf_size animationIndex = 0; animationIndex < cgltfData->animations_count; animationIndex++)
            {
                cgltf_animation* cgltfAnimation = &cgltfData->animations[animationIndex];
                GLTFAnimation&   animation      = result[animationIndex];

                if (cgltfAnimation->name)
                {
                    strcpy(animation.name, cgltfAnimation->name);
                }
                else
                {
                    sprintf(animation.name, "animation_%zu", animationIndex);
                }

                // Channels
                animation.channels.resize(cgltfAnimation->channels_count);
                for (cgltf_size channelIndex = 0; channelIndex < cgltfAnimation->channels_count; channelIndex++)
                {
                    cgltf_animation_channel* cgltfChannel = &cgltfAnimation->channels[channelIndex];
                    GLTFAnimationChannel&    channel      = animation.channels[channelIndex];

                    channel.nodeIndex    = (int)cgltf_node_index(cgltfData, cgltfChannel->target_node);
                    channel.samplerIndex = (int)cgltf_animation_sampler_index(cgltfAnimation, cgltfChannel->sampler);

                    switch (cgltfChannel->target_path)
                    {
                    case cgltf_animation_path_type_translation:
                    {
                        channel.path = GLTFAnimationChannelPath_Translation;
                        break;
                    }
                    case cgltf_animation_path_type_rotation:
                    {
                        channel.path = GLTFAnimationChannelPath_Rotation;
                        break;
                    }
                    case cgltf_animation_path_type_scale:
                    {
                        channel.path = GLTFAnimationChannelPath_Scale;
                        break;
                    }
                    default:
                    {
                        Log("Unsupported animation target path");
                        Assert(0);
                    }
                    }

                    // Samplers
                    animation.duration = 0.0f;
                    animation.samplers.resize(cgltfAnimation->samplers_count);
                    for (cgltf_size samplerIndex = 0; samplerIndex < cgltfAnimation->samplers_count; samplerIndex++)
                    {
                        cgltf_animation_sampler* cgltfSampler = &cgltfAnimation->samplers[samplerIndex];
                        cgltf_accessor*          cgltfInput   = cgltfSampler->input;
                        cgltf_accessor*          cgltfOutput  = cgltfSampler->output;

                        GLTFAnimationSampler& sampler = animation.samplers[samplerIndex];
                        sampler.times.resize(cgltfInput->count);
                        sampler.transformations.resize(cgltfOutput->count);

                        Assert(cgltfInput->count == cgltfOutput->count);
                        for (cgltf_size accessorIndex = 0; accessorIndex < cgltfOutput->count; accessorIndex++)
                        {
                            if (!cgltf_accessor_read_float(cgltfInput, accessorIndex, &sampler.times[accessorIndex], 1))
                            {
                                Assert(0);
                            }
                            if (!cgltf_accessor_read_float(cgltfOutput, accessorIndex,
                                                           &sampler.transformations[accessorIndex].x, 4))
                            {
                                Assert(0);
                            }

                            if (sampler.times[accessorIndex] > animation.duration)
                            {
                                animation.duration = sampler.times[accessorIndex];
                            }
                        }

                        switch (cgltfSampler->interpolation)
                        {
                        case cgltf_interpolation_type_linear:
                        {
                            sampler.interpolation = GLTFAnimationSamplerInterpolation_Linear;
                            break;
                        }
                        case cgltf_interpolation_type_step:
                        {
                            sampler.interpolation = GLTFAnimationSamplerInterpolation_Step;
                            break;
                        }
                        case cgltf_interpolation_type_cubic_spline:
                        {
                            sampler.interpolation = GLTFAnimationSamplerInterpolation_CubicSpline;
                            break;
                        }
                        default:
                        {
                            Log("Unknown animation sampler interpolation");
                            Assert(0);
                        }
                        }
                    }
                }
            }
        }
    }
    else
    {
        Log("Unable to parse .gltf animations '%s'", gltfFilename);
        Assert(0);
    }

    // Log("------------------------------------------------------------");
    platform->FileFree(gltfFile.content);
    if (binFile.content)
    {
        platform->FileFree(binFile.content);
    }

    return result;
}

internal GLTFNode* JointGetMappedGLTFNode(Joint* joint, GLTFModel* gltfModel)
{
    for (u32 nodeIndex = 0; nodeIndex < gltfModel->nodes.size(); nodeIndex++)
    {
        GLTFNode* gltfNode = &gltfModel->nodes[nodeIndex];
        if (strcmp(joint->name, gltfNode->name) == 0)
        {
            return gltfNode;
        }
    }

    return 0;
}

internal Joint* GLTFNodeGetMappedJoint(Skeleton* skeleton, GLTFNode* gltfNode)
{
    for (u32 jointIndex = 0; jointIndex < skeleton->jointCount; jointIndex++)
    {
        Joint* joint = skeleton->joints + jointIndex;
        if (strcmp(joint->name, gltfNode->name) == 0)
        {
            return joint;
        }
    }

    return 0;
}

internal void JointInitFromGLTFNode(Joint* joint, GLTFNode* gltfNode)
{
    strcpy(joint->name, gltfNode->name);
    joint->inverseBindMatrix = gltfNode->inverseBindMatrix;
    joint->translation       = gltfNode->translation;
    joint->rotation          = gltfNode->rotation;
    joint->scale             = gltfNode->scale;
}

// TODO: Rename
Animation* GLTFConvertAnimation(GLTFModel* gltfModel, GLTFAnimation* gltfAnimation, Arena* arena)
{
    Skeleton* skeleton = ExtractGLTFSkeleton(gltfModel, arena);

    u32 samplerCount = (u32)gltfAnimation->samplers.size();
    u32 channelCount = (u32)gltfAnimation->channels.size();

    Animation* result = PushStruct(arena, Animation);
    result->samplers  = PushArray(arena, samplerCount, AnimationSampler);
    result->channels  = PushArray(arena, channelCount, AnimationChannel);

    strcpy(result->name, gltfAnimation->name);
    result->duration     = gltfAnimation->duration;
    result->samplerCount = samplerCount;
    result->channelCount = channelCount;

    for (u32 samplerIndex = 0; samplerIndex < samplerCount; samplerIndex++)
    {
        AnimationSampler*     sampler     = result->samplers + samplerIndex;
        GLTFAnimationSampler* gltfSampler = &gltfAnimation->samplers[samplerIndex];
        Assert(gltfSampler->times.size() == gltfSampler->transformations.size());

        u32 count                = (u32)gltfSampler->times.size();
        sampler->count           = count;
        sampler->times           = PushArray(arena, count, f32);
        sampler->transformations = PushArray(arena, count, glm::vec4);

        memcpy(sampler->times, gltfSampler->times.data(), sizeof(f32) * count);
        memcpy(sampler->transformations, gltfSampler->transformations.data(), sizeof(glm::vec4) * count);
    }

    memcpy(result->channels, gltfAnimation->channels.data(), sizeof(AnimationChannel) * channelCount);
    for (u32 channelIndex = 0; channelIndex < channelCount; channelIndex++)
    {
        AnimationChannel* channel = result->channels + channelIndex;

        GLTFNode* gltfTargetNode = &gltfModel->nodes[channel->jointIndex];
        Joint*    targetJoint    = GLTFNodeGetMappedJoint(skeleton, gltfTargetNode);
        s64       jointIndex     = targetJoint - skeleton->joints;
        channel->jointIndex      = (u32)jointIndex;
    }

    return result;
}