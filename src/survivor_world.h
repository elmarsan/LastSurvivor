#pragma once

#define GRID_COLS 30
#define GRID_ROWS 30

#define CELL_SIZE 1.0f
#define CELL_HALF (CELL_SIZE * 0.5f)

#define CELL_ROW(index)      (index / GRID_ROWS)
#define CELL_COL(index)      (index % GRID_COLS)
#define CELL_INDEX(row, col) (col + ((row) * GRID_ROWS))

typedef u32 cell_index;

v3  WorldMousePicking(Camera* camera, mat4x4 projection, v2u windowDim, v2u mouse);
b32 WorldIsPositionInBounds(v3 position);

inline cell_index WorldPositionToGridCell(v3 position)
{
    s32 minCol = -(GRID_COLS / 2);
    s32 minRow = (GRID_ROWS / 2);

    s32 col = (s32)floorf((position.x) - minCol);
    s32 row = (s32)((-position.z) + minRow);

    return CELL_INDEX(row, col);
}

inline v3 WorldGridCellToPosition(cell_index cellIndex)
{
    u32 row = CELL_ROW(cellIndex);
    u32 col = CELL_COL(cellIndex);

    f32 offsetX = -(GRID_COLS / 2);
    f32 offsetZ = (GRID_ROWS / 2);

    f32 x = (col * CELL_SIZE) + offsetX + CELL_HALF;
    f32 z = -((row * CELL_SIZE) - offsetZ + CELL_HALF);

    return v3{ x, 0.0f, z };
}

inline b32 WorldIsValidCell(cell_index cell) { return cell >= 0 && cell < GRID_ROWS * GRID_COLS; }