/*
Vertex indices:

      4 _____________ 5
       /|           /|
      / |          / |
   7 /__|_________/6 |
    |   |        |   |
    |  0|________|___|1
    |  /         |  /
    | /          | /
    |/___________|/
   3              2

Source: http://paulbourke.net/geometry/polygonise/
*/
#ifndef TG_MARCHING_CUBES_H
#define TG_MARCHING_CUBES_H



#include "math/tg_math.h"
#include "tg_common.h"



typedef struct tg_marching_cubes_triangle
{
    v3    positions[3];
} tg_marching_cubes_triangle;

typedef struct tg_marching_cubes_grid_cell
{
    v3     positions[8];
    f32    values[8];
} tg_marching_cubes_grid_cell;



u32    tg_marching_cubes_polygonise(const tg_marching_cubes_grid_cell* p_grid_cell, f32 isolevel, tg_marching_cubes_triangle* p_triangles);
v3     tg_marching_cubes_vertex_interpolate(f32 isolevel, const v3* p_position0, const v3* p_position1, f32 value0, f32 value1);

#endif
