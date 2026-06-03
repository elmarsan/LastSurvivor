Entity* Entity_Spawn(EntityManager* manager, EntityType type, glm::vec3 position)
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
        entity->aabb.min = { -0.5f, -0.5f, -0.5f };
        entity->aabb.max = { 0.5f, 0.5f, 0.5f };
        break;
    }
    case EntityType_Enemy:
    {
        // u32    min        = Model_ZombieMaleA;
        // u32    max        = Model_ZombieFemaleA;
        // Asset  assetID    = (Asset)(rand() % (max + 1 - min) + min);
        Asset  assetID    = Model_ZombieMaleA;
        Model* model      = Assets_GetModel(manager->assets, assetID);
        u32    jointCount = model->skeleton->jointCount;

        entity->assetID                       = assetID;
        entity->skeleton                      = PushStruct(transientArena, Skeleton);
        entity->skeleton->joints              = PushArray(transientArena, jointCount, Joint);
        entity->skeleton->jointSkinMatrices   = PushArray(transientArena, jointCount, glm::mat4);
        entity->skeleton->jointGlobalMatrices = PushArray(transientArena, jointCount, glm::mat4);
        entity->skeleton->jointIndexBindOrder = PushArray(transientArena, jointCount, u32);
        entity->skeleton->jointCount          = jointCount;

        memcpy(entity->skeleton->joints, model->skeleton->joints, sizeof(Joint) * jointCount);
        memcpy(entity->skeleton->jointIndexBindOrder, model->skeleton->jointIndexBindOrder, sizeof(u32) * jointCount);

        break;
    }
    }

    return entity;
}

void Entity_SpawnCollider(EntityManager* manager, WorldCollider worldCollider)
{
    Assert(manager->entityCount < ArrayCount(manager->entities));

    Entity* entity   = &manager->entities[manager->entityCount++];
    entity->type     = EntityType_Collider;
    entity->position = worldCollider.position;
    entity->aabb     = worldCollider.aabb;
}

// internal void EntityRemove(EntityManager* manager, Entity* entity)
//{
//     u32 index = 0;
//     for (u32 entityIndex = 0; entityIndex < manager->entityCount; entityIndex++)
//     {
//         if (entity == &manager->entities[entityIndex])
//         {
//             index = entityIndex;
//             break;
//         }
//     }

//    while (index < manager->entityCount)
//    {
//        manager->entities[index] = manager->entities[index + 1];
//        index++;
//    }

//    manager->entityCount--;
//}