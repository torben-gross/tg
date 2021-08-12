#ifndef TG_MARCHING_CUBES_H
#define TG_MARCHING_CUBES_H

#include "math/tg_math.h"
#include "tg_common.h"

#define TG_MARCHING_CUBES_MAX_VERTEX_COUNT(voxel_map_size)    (15 * ((voxel_map_size).x - 1) * ((voxel_map_size).y - 1) * ((voxel_map_size).z - 1))

u32 tg_marching_cubes_create_mesh(v3i voxel_map_size, const i8* p_voxel_map, v3 scale, u32 vertex_buffer_size, u32 vertex_position_offset, u32 vertex_stride, f32* p_vertex_buffer);

#endif
