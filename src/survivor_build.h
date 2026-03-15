#pragma once

struct SnapCandidate
{
    Entity* entity;
    v3      from;
    v3      to;
};

void          ObstacleDrag(Entity* obstacle, Camera* camera, mat4x4 projection, v2u windowDim, v2u screenCoordPos);
void          ObstacleRotate(Entity* obstacle, b32 counterclockwise);
SnapCandidate ObstacleFindNearestSnap(GridCell* grid, Entity* obstacle);
void          ObstaclesSnap(GridCell* grid, Entity* a, SnapCandidate* snapCandidate);
void          ObstaclePlace(GridCell* grid, GridCellV2* gridV2, Entity* entity);
b32           ObstacleIsValidPosition(Entity* entities, u32 entityCount, GridCell* grid, Entity* entity);