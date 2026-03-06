internal AABB AABBFromCorners(EntityWorldCorners corners, AABB local)
{
    AABB world = { 0 };

    world.min = corners.corners[0];
    world.max = corners.corners[0];

    for (u32 i = 1; i < 4; i++)
    {
        v3 p = corners.corners[i];

        if (p.x < world.min.x)
        {
            world.min.x = p.x;
        }
        if (p.z < world.min.z)
        {
            world.min.z = p.z;
        }
        if (p.x > world.max.x)
        {
            world.max.x = p.x;
        }
        if (p.z > world.max.z)
        {
            world.max.z = p.z;
        }
    }

    world.min.y = local.min.y;
    world.max.y = local.max.y;

    return world;
}

b32 EntitiesIntersect(Entity* a, Entity* b, AABB* intersection = 0)
{
    AABB aWorldAABB = AABBFromCorners(EntityGetWorldCorners(a), a->aabb);
    AABB bWorldAABB = AABBFromCorners(EntityGetWorldCorners(b), b->aabb);

    return AABBIntersection(aWorldAABB, bWorldAABB, intersection);
}