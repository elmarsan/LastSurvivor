Entity* EntitySpawn(EntityManager* manager, EntityType type, glm::vec3 position)
{
    Assert(manager->entityCount < ArrayCount(manager->entities));

    // Arena* arena          = &manager->arena;
    Arena* transientArena = &manager->transientArena;

    Entity* entity   = &manager->entities[manager->entityCount++];
    entity->type     = type;
    entity->health   = maxHealth;
    entity->position = position;
    entity->forward  = { 0.0f, 0.0f, 0.0f };

    switch (entity->type)
    {
    case EntityType_Player:
    {
        // Player do not use assetID, is not rendered
        entity->scale = glm::vec3{ 1.0f, 1.0f, 1.0f };
        Model* model  = AssetsModelGet(manager->assets, entity->assetID);
        // entity->aabb  = model->aabb;
        //  TODO: Push player skeleton to permanent arena
        break;
    }
    case EntityType_Enemy:
    {
        u32    min        = Model_ZombieMaleA;
        u32    max        = Model_ZombieFemaleA;
        Asset  assetID    = (Asset)(rand() % (max + 1 - min) + min);
        Model* model      = AssetsModelGet(manager->assets, assetID);
        u32    jointCount = model->skeleton->jointCount;

        entity->assetID                       = assetID;
        entity->scale                         = ZOMBIE_SCALE;
        entity->skeleton                      = PushStruct(transientArena, Skeleton);
        entity->skeleton->joints              = PushArray(transientArena, jointCount, Joint);
        entity->skeleton->jointMatrices       = PushArray(transientArena, jointCount, glm::mat4);
        entity->skeleton->jointIndexBindOrder = PushArray(transientArena, jointCount - 1, u32);
        entity->skeleton->jointCount          = jointCount;

        memcpy(entity->skeleton->joints, model->skeleton->joints, sizeof(Joint) * jointCount);
        memcpy(entity->skeleton->jointIndexBindOrder, model->skeleton->jointIndexBindOrder,
               sizeof(u32) * jointCount - 1);

        break;
    }
    }

    return entity;
}

internal void EntityRemove(EntityManager* manager, Entity* entity)
{
    u32 index = 0;
    for (u32 entityIndex = 0; entityIndex < manager->entityCount; entityIndex++)
    {
        if (entity == &manager->entities[entityIndex])
        {
            index = entityIndex;
            break;
        }
    }

    while (index < manager->entityCount)
    {
        manager->entities[index] = manager->entities[index + 1];
        index++;
    }

    manager->entityCount--;
}