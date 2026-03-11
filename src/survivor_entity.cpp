internal AABB AABBFromCorners(EntityWorldCorners corners, AABB local)
{
    AABB world = { 0 };

    world.min = corners.corners[0];
    world.max = corners.corners[0];

    for (u32 cornerIndex = 1; cornerIndex < ArrayCount(corners.corners); cornerIndex++)
    {
        v3 p = corners.corners[cornerIndex];

        world.min.x = Min(world.min.x, p.x);
        world.min.z = Min(world.min.z, p.z);
        world.max.x = Max(world.max.x, p.x);
        world.max.z = Max(world.max.z, p.z);
    }

    world.min.y = local.min.y;
    world.max.y = local.max.y;

    return world;
}

b32 EntitiesIntersect(Entity* a, Entity* b, AABB* intersection = 0)
{
    AABB aWorldAABB = AABBFromCorners(EntityGetWorldCorners(a), a->aabb);
    AABB bWorldAABB = AABBFromCorners(EntityGetWorldCorners(b), b->aabb);

    if (a->type == EntityType_Obstacle && b->type == EntityType_Obstacle)
    {
        Rect aRect = { 0 };
        Rect bRect = { 0 };

        // Convert to top-left coordinates system
        aRect.x = aWorldAABB.min.x + 15.0f;
        aRect.w = (aWorldAABB.max.x + 15.0f) - (aWorldAABB.min.x + 15.0f);
        aRect.y = aWorldAABB.min.z + 15.0f;
        aRect.h = (aWorldAABB.max.z + 15.0f) - (aWorldAABB.min.z + 15.0f);

        bRect.x = bWorldAABB.min.x + 15.0f;
        bRect.w = (bWorldAABB.max.x + 15.0f) - (bWorldAABB.min.x + 15.0f);
        bRect.y = bWorldAABB.min.z + 15.0f;
        bRect.h = (bWorldAABB.max.z + 15.0f) - (bWorldAABB.min.z + 15.0f);

        //      aRect.x = aWorldAABB.min.x;
        //      aRect.w = Abs(aWorldAABB.max.x - aWorldAABB.min.x);
        //      aRect.y = aWorldAABB.min.z;
        //      aRect.h = Abs(aWorldAABB.max.z - aWorldAABB.min.z);

        //      bRect.x = bWorldAABB.min.x;
        //      bRect.w = Abs(bWorldAABB.max.x - bWorldAABB.min.x);
        //      bRect.y = bWorldAABB.min.z;
        //      bRect.h = Abs(bWorldAABB.max.z - bWorldAABB.min.z);

        if (RectIntersection(aRect, bRect))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    return AABBIntersection(aWorldAABB, bWorldAABB, intersection);
}