#pragma once

struct SnapCandidate
{
    Entity* entity;
    v3      from;
    v3      to;
};

void          BuildDragObstacle(Entity* obstacle, Camera* camera, mat4x4 projection, v2u windowDim, v2u screenCoordPos);
void          BuildRotateObstacle(Entity* obstacle, b32 counterclockwise);
void          BuildSnapObstacles(World* world, Entity* a, SnapCandidate* snapCandidate);
SnapCandidate BuildFindSnapCandidate(World* world, Entity* obstacle);
b32           BuildIsObstacleValidPosition(World* world, EntityManager* entityManager, Entity* entity);
void          BuildPlaceObstacle(World* world, EntityManager* entityManager, Entity* entity);