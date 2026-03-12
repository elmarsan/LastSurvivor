#pragma once

// TODO: (min and max) vs (center and halfsize)
// https://www.yosoygames.com.ar/wp/2013/07/good-bye-axisalignedbox-hello-aabb/
struct AABB
{
    v3 min;
    v3 max;
};

struct Ray
{
    v3 origin;
    v3 dir;
};

struct Rect
{
    f32 x, y;
    f32 w, h;
};

// TODO: Fix 0.0f axis direction
// https://tavianator.com/2022/ray_box_boundary.html#fast-branchless-raybounding-box-intersections-part-3-boundaries
inline b32 AABBRayIntersection(AABB aabb, Ray ray)
{
    f64 tmin = 0.0;
    f64 tmax = INFINITY;

    for (int d = 0; d < 3; d++)
    {
        if (ray.dir.e[d] == 0.0f)
        {
            // Ray is parallel to slab
            if (ray.origin.e[d] < aabb.min.e[d] || ray.origin.e[d] > aabb.max.e[d])
            {
                return false;
            }
        }
        else
        {
            f64 invD = 1.0 / ray.dir.e[d];

            f64 t1 = (aabb.min.e[d] - ray.origin.e[d]) * invD;
            f64 t2 = (aabb.max.e[d] - ray.origin.e[d]) * invD;

            tmin = Max(tmin, Min(t1, t2));
            tmax = Min(tmax, Max(t1, t2));

            if (tmin > tmax)
                return false;
        }
    }

    return true;
}

inline b32 AABBSegmentIntersection(AABB aabb, v3 p0, v3 p1)
{
    v3 dir = p1 - p0;

    f64 tmin = 0.0;
    f64 tmax = 1.0;

    for (int d = 0; d < 3; d++)
    {
        if (dir.e[d] == 0.0)
        {
            // Segment parallel to slab
            if (p0.e[d] < aabb.min.e[d] || p0.e[d] > aabb.max.e[d])
            {
                return false;
            }
        }
        else
        {
            f64 invD = 1.0 / dir.e[d];

            f64 t1 = (aabb.min.e[d] - p0.e[d]) * invD;
            f64 t2 = (aabb.max.e[d] - p0.e[d]) * invD;

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

inline b32 AABBIntersection(AABB a, AABB b, AABB* intersection)
{
    b32 overlaps = false;

    b32 x = (a.max.x >= b.min.x) && (a.min.x <= b.max.x);
    b32 y = (a.max.y >= b.min.y) && (a.min.y <= b.max.y);
    b32 z = (a.max.z >= b.min.z) && (a.min.z <= b.max.z);
    if (x && y && z)
    {
        overlaps = true;

        if (intersection)
        {
            intersection->min = { Max(a.min.x, b.min.x), Max(a.min.y, b.min.y), Max(a.min.z, b.min.z) };
            intersection->max = { Min(a.max.x, b.max.x), Min(a.max.y, b.max.y), Min(a.max.z, b.max.z) };
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

inline AABB AABBToWorld(AABB local, v3 pos)
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