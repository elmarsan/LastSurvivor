#pragma once

// TODO: (min and max) vs (center and halfsize)
// https://www.yosoygames.com.ar/wp/2013/07/good-bye-axisalignedbox-hello-aabb/
struct AABB
{
    glm::vec3 min;
    glm::vec3 max;
};

struct Ray
{
    glm::vec3 origin;
    glm::vec3 dir;
};

struct Rect
{
    f32 x, y;
    f32 w, h;
};

struct Cylinder
{
    glm::vec3 start;
    glm::vec3 end;
    f32       radius;
};

// TODO: Fix 0.0f axis direction
// https://tavianator.com/2022/ray_box_boundary.html#fast-branchless-raybounding-box-intersections-part-3-boundaries
inline b32 AABBRayIntersection(AABB aabb, Ray ray)
{
    f64 tmin = 0.0;
    f64 tmax = INFINITY;

    for (int d = 0; d < 3; d++)
    {
        if (ray.dir[d] == 0.0f)
        {
            // Ray is parallel to slab
            if (ray.origin[d] < aabb.min[d] || ray.origin[d] > aabb.max[d])
            {
                return false;
            }
        }
        else
        {
            f64 invD = 1.0 / ray.dir[d];

            f64 t1 = (aabb.min[d] - ray.origin[d]) * invD;
            f64 t2 = (aabb.max[d] - ray.origin[d]) * invD;

            tmin = Max(tmin, Min(t1, t2));
            tmax = Min(tmax, Max(t1, t2));

            if (tmin > tmax)
                return false;
        }
    }

    return true;
}

inline b32 AABBSegmentIntersection(AABB aabb, glm::vec3 p0, glm::vec3 p1)
{
    glm::vec3 dir = p1 - p0;

    f64 tmin = 0.0;
    f64 tmax = 1.0;

    for (int d = 0; d < 3; d++)
    {
        if (dir[d] == 0.0)
        {
            // Segment parallel to slab
            if (p0[d] < aabb.min[d] || p0[d] > aabb.max[d])
            {
                return false;
            }
        }
        else
        {
            f64 invD = 1.0 / dir[d];

            f64 t1 = (aabb.min[d] - p0[d]) * invD;
            f64 t2 = (aabb.max[d] - p0[d]) * invD;

            f64 tNear = Min(t1, t2);
            f64 tFar  = Max(t1, t2);

            tmin = Max(tmin, tNear);
            tmax = Min(tmax, tFar);

            if (tmin > tmax)
                return false;
        }
    }

    return true;
}

inline b32 AABBOverlaps(AABB a, AABB b, AABB* overlap)
{
    b32 overlaps = false;

    b32 x = (a.max.x >= b.min.x) && (a.min.x <= b.max.x);
    b32 y = (a.max.y >= b.min.y) && (a.min.y <= b.max.y);
    b32 z = (a.max.z >= b.min.z) && (a.min.z <= b.max.z);
    if (x && y && z)
    {
        overlaps = true;

        if (overlap)
        {
            overlap->min = { Max(a.min.x, b.min.x), Max(a.min.y, b.min.y), Max(a.min.z, b.min.z) };
            overlap->max = { Min(a.max.x, b.max.x), Min(a.max.y, b.max.y), Min(a.max.z, b.max.z) };
        }
    }

    return overlaps;
}

inline b32 AABBIntersectionXZ(AABB a, AABB b)
{
    if (a.max.x <= b.min.x)
    {
        return false;
    }
    if (b.max.x <= a.min.x)
    {
        return false;
    }
    if (a.max.z <= b.min.z)
    {
        return false;
    }
    if (b.max.z <= a.min.z)
    {
        return false;
    }

    return true;
}

inline AABB AABBToWorld(AABB local, glm::vec3 pos)
{
    AABB world;
    world.min = local.min + pos;
    world.max = local.max + pos;
    return world;
}

inline AABB AABBExpandXZ(AABB aabb, f32 radius)
{
    AABB expanded = aabb;
    expanded.min.x -= radius;
    expanded.min.z -= radius;
    expanded.max.x += radius;
    expanded.max.z += radius;
    return expanded;
}

inline b32 RectIntersection(Rect a, Rect b)
{
    return !(a.x + a.w <= b.x || b.x + b.w <= a.x || a.y + a.h <= b.y || b.y + b.h <= a.y);
}

inline b32 LineSegmentCylinder(glm::vec3 segP0, glm::vec3 segP1, Cylinder cylinder)
{
    //
}