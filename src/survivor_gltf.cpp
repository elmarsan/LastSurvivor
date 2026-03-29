#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#define MESH_NONE     -1
#define MATERIAL_NONE -1

inline size_t GetParentPathLength(const char* path)
{
    const char* last_slash = strrchr(path, '/');
    if (!last_slash)
    {
        return 0;
    }

    return (last_slash - path) + 1;
}

GLTFModel GLTFParse(char* gltfFilename, PlatformAPI* platform)
{
    GLTFModel result;

    platform->Logf("------------------------------------------------------------");
    platform->Logf("Reading .gltf file: '%s'", gltfFilename);

    FileReadResult gltfFile = platform->FileReadEntire(gltfFilename);
    FileReadResult binFile  = { 0 };

    if (!gltfFile.content)
    {
        platform->Logf("Unable to parse .gltf file '%s' not found", gltfFilename);
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

            platform->Logf("Reading .bin file: '%s'", binFilename);

            binFile = platform->FileReadEntire(binFilename);
            if (!binFile.content)
            {
                platform->Logf("Unable to parse .bin file '%s'", binFilename);
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
            platform->Logf("Invalid gltf '%s'", gltfFilename);
            Assert(0);
        }

        cgltf_scene* scene = cgltfData->scene;
        if (cgltfData->scenes_count > 1)
        {
            platform->Logf("More than one scene %zu", cgltfData->scenes_count);
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
                glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
                glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

                if (cgltfNode->has_translation)
                {
                    memcpy(&translation.x, cgltfNode->translation, 3 * sizeof(float));
                }
                if (cgltfNode->has_rotation)
                {
                    glm::vec4 rotationVec;
                    memcpy(&rotationVec.x, cgltfNode->rotation, 4 * sizeof(float));

                    // rotation = glm::quat{ rotationVec.w, rotationVec.x, rotationVec.y, rotationVec.z };
                    rotation = glm::quat{ rotationVec.x, rotationVec.y, rotationVec.z, rotationVec.w };
                }
                if (cgltfNode->has_scale)
                {
                    memcpy(&scale.x, cgltfNode->scale, 3 * sizeof(float));
                }
                if (cgltfNode->has_matrix)
                {
                    glm::mat4 baseLocalTransform;
                    memcpy(&baseLocalTransform[0][0], cgltfNode->matrix, 16 * sizeof(float));

                    glm::vec3 skew;
                    glm::vec4 perspective;

                    glm::decompose(baseLocalTransform, scale, rotation, translation, skew, perspective);
                }

                node.bindTranslation = node.localTranslation = translation;
                node.bindRotation = node.localRotation = rotation;
                node.bindScale = node.localScale = scale;

                node.inverseBindMatrix = glm::mat4{ 1.0f };

                node.childrenIndexes.resize(cgltfNode->children_count);
                for (cgltf_size childIndex = 0; childIndex < cgltfNode->children_count; childIndex++)
                {
                    node.childrenIndexes[childIndex] =
                        (int)cgltf_node_index(cgltfData, cgltfNode->children[childIndex]);
                }

                if (cgltfNode->mesh)
                {
                    node.meshIndex = (int)cgltf_mesh_index(cgltfData, cgltfNode->mesh);
                }
                else
                {
                    node.meshIndex = MESH_NONE;
                }
                if (cgltfNode->skin)
                {
                    result.skeleton.meshNodeIndex = (int)cgltf_skin_index(cgltfData, cgltfNode->skin);
                }
                else
                {
                    // node.skinIndex = SKIN_NONE;
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
                            platform->Logf("Unsupported primitive attribute: %s", cgltfAttribute->name);
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
                        meshPrimitive.materialIndex = MATERIAL_NONE;
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
                    platform->Logf("Model has %zu skins, only first one is processed", cgltfData->skins_count);
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
                platform->Logf("Material count %zu", cgltfData->materials_count);

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
                            material.albedoTextureIndex = (int)textureIndex;
                        }
                    }
                    else
                    {
                        platform->Logf("Unsupported material, index %zu", materialIndex);
                    }
                }
            }
        }

        // Parse textures
        {
            if (cgltfData->textures_count > 0)
            {
                platform->Logf("Texture  count %zu", cgltfData->textures_count);

                result.textures.resize(cgltfData->textures_count);
                for (cgltf_size textureIndex = 0; textureIndex < cgltfData->textures_count; textureIndex++)
                {
                    cgltf_texture* cgltfTexture = &cgltfData->textures[textureIndex];
                    Assert(cgltfTexture->image && cgltfTexture->image->uri);

                    char textureFilename[256];
                    memcpy(textureFilename, gltfFilename, directoryLen);
                    strcpy(textureFilename + directoryLen, cgltfTexture->image->uri);

                    platform->Logf("Texture %zu '%s'", textureIndex, textureFilename);
                }
            }
        }
    }
    else
    {
        platform->Logf("Unable to parse gltf '%s'", gltfFilename);
        Assert(0);
    }

    platform->Logf("------------------------------------------------------------");
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

    platform->Logf("------------------------------------------------------------");
    platform->Logf("Reading .gltf animations: '%s'", gltfFilename);

    FileReadResult gltfFile = platform->FileReadEntire(gltfFilename);
    FileReadResult binFile  = { 0 };

    if (!gltfFile.content)
    {
        platform->Logf("Unable to parse .gltf file '%s' not found", gltfFilename);
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

            platform->Logf("Reading .bin file: '%s'", binFilename);

            binFile = platform->FileReadEntire(binFilename);
            if (!binFile.content)
            {
                platform->Logf("Unable to parse .bin file '%s'", binFilename);
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
                    AnimationChannel&        channel      = animation.channels[channelIndex];

                    channel.nodeIndex    = (int)cgltf_node_index(cgltfData, cgltfChannel->target_node);
                    channel.samplerIndex = (int)cgltf_animation_sampler_index(cgltfAnimation, cgltfChannel->sampler);

                    switch (cgltfChannel->target_path)
                    {
                    case cgltf_animation_path_type_translation:
                    {
                        channel.path = AnimationChannelPath_Translation;
                        break;
                    }
                    case cgltf_animation_path_type_rotation:
                    {
                        channel.path = AnimationChannelPath_Rotation;
                        break;
                    }
                    case cgltf_animation_path_type_scale:
                    {
                        channel.path = AnimationChannelPath_Scale;
                        break;
                    }
                    default:
                    {
                        platform->Logf("Unsupported animation target path");
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

                        AnimationSampler& sampler = animation.samplers[samplerIndex];
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
                            sampler.interpolation = AnimationSamplerInterpolation_Linear;
                            break;
                        }
                        case cgltf_interpolation_type_step:
                        {
                            sampler.interpolation = AnimationSamplerInterpolation_Step;
                            break;
                        }
                        case cgltf_interpolation_type_cubic_spline:
                        {
                            sampler.interpolation = AnimationSamplerInterpolation_CubicSpline;
                            break;
                        }
                        default:
                        {
                            platform->Logf("Unknown animation sampler interpolation");
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
        platform->Logf("Unable to parse .gltf animations '%s'", gltfFilename);
        Assert(0);
    }

    platform->Logf("------------------------------------------------------------");
    platform->FileFree(gltfFile.content);
    if (binFile.content)
    {
        platform->FileFree(binFile.content);
    }

    return result;
}