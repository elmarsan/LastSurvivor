#pragma once

#define GRID_COLS         30
#define GRID_ROWS         30
#define GRID_CELLS        (GRID_COLS * GRID_ROWS)
#define GRID_RIGHT_LIMIT  (GRID_COLS * 0.5f)
#define GRID_LEFT_LIMIT   (-GRID_RIGHT_LIMIT)
#define GRID_BOTTOM_LIMIT (GRID_COLS * 0.5f)
#define GRID_TOP_LIMIT    (-GRID_BOTTOM_LIMIT)
#define GRID_MAX_ROW      (GRID_ROWS - 1)
#define GRID_MIN_ROW      0
#define GRID_MAX_COL      (GRID_COLS - 1)
#define GRID_MIN_COL      0

#define CELL_SIZE            1.0f
#define CELL_HALF            (CELL_SIZE * 0.5f)
#define CELL_ROW(index)      (index / GRID_ROWS)
#define CELL_COL(index)      (index % GRID_COLS)
#define CELL_INDEX(row, col) (col + ((row) * GRID_ROWS))

typedef u32 cell_index;

struct Entity;

struct GridCell
{
    Entity* entities[4];
    u32     entityCount;
};

// TODO: Rename this to coordinate space picking. Picking can be done with gamepad.
v3   WorldMousePicking(Camera* camera, mat4x4 projection, v2u windowDim, v2u mouse);
b32  WorldIsPositionInBounds(v3 position);
void GridAppendEntity(GridCell* grid, cell_index cellIndex, Entity* entity);
b32  GridIsValidCellForEntity(GridCell* grid, cell_index cellIndex, Entity* entity);

inline cell_index WorldPositionToGridCell(v3 position)
{
    s32 halfRows = (GRID_ROWS / 2);

    s32 col = (s32)floorf((position.x) - GRID_LEFT_LIMIT);
    s32 row = (s32)((-position.z) + halfRows);

    return CELL_INDEX(row, col);
}

inline v3 WorldGridCellToPosition(cell_index cellIndex)
{
    u32 row = CELL_ROW(cellIndex);
    u32 col = CELL_COL(cellIndex);

    f32 offsetX = GRID_LEFT_LIMIT;
    f32 offsetZ = (GRID_ROWS / 2);

    f32 x = (col * CELL_SIZE) + offsetX + CELL_HALF;
    f32 z = -((row * CELL_SIZE) - offsetZ + CELL_HALF);

    return v3{ x, 0.0f, z };
}

inline b32 WorldIsValidCellIndex(cell_index cellIndex) { return cellIndex >= 0 && cellIndex < GRID_CELLS; }