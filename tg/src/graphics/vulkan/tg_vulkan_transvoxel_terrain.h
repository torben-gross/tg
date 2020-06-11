#ifndef TG_VULKAN_TRANSVOXEL_TERRAIN_H
#define TG_VULKAN_TRANSVOXEL_TERRAIN_H

#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "math/tg_math.h"
#include "tg_entity.h"
#include "tg_transvoxel_terrain.h"



#define TG_TRANSVOXEL_MIN_PADDING                                1
#define TG_TRANSVOXEL_MAX_PADDING                                1

#define TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE                       16
#define TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING          (TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE + TG_TRANSVOXEL_MIN_PADDING + TG_TRANSVOXEL_MAX_PADDING)
#define TG_TRANSVOXEL_CELLS_PER_BLOCK                            (TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE * TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE * TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE)
#define TG_TRANSVOXEL_CELLS_PER_BLOCK_WITH_PADDING               (TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING * TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING * TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING)

#define TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE                      17
#define TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE_WITH_PADDING         (TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE + TG_TRANSVOXEL_MIN_PADDING + TG_TRANSVOXEL_MAX_PADDING)
#define TG_TRANSVOXEL_VOXELS_PER_BLOCK                           (TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE * TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE * TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE)
#define TG_TRANSVOXEL_VOXELS_PER_BLOCK_WITH_PADDING              (TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE_WITH_PADDING * TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE_WITH_PADDING * TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE_WITH_PADDING)



#define TG_TRANSVOXEL_MAX_TRIANGLES_PER_BLOCK                    (5 * TG_TRANSVOXEL_CELLS_PER_BLOCK)
#define TG_TRANSVOXEL_MAX_VERTICES_PER_BLOCK                     (15 * TG_TRANSVOXEL_CELLS_PER_BLOCK)

#define TG_TRANSVOXEL_VOXEL_MAP_AT(voxel_map, x, y, z)           ((voxel_map).p_data[TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE_WITH_PADDING * TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE_WITH_PADDING * (z) + TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE_WITH_PADDING * (y) + (x)])
#define TG_TRANSVOXEL_VOXEL_MAP_AT_V3(voxel_map, v)              TG_TRANSVOXEL_VOXEL_MAP_AT(voxel_map, (v).x, (v).y, (v).z)

#define TG_TRANSVOXEL_REUSE_CELL_AT_V3(reuse_cells, position)    ((reuse_cells).pp_decks[(position).z & 1][(position).y * TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING + (position).x])



typedef struct tg_transvoxel_voxel_map
{
	i8    p_data[TG_TRANSVOXEL_VOXELS_PER_BLOCK_WITH_PADDING];
} tg_transvoxel_voxel_map;

typedef struct tg_transvoxel_vertex
{
	v3    position;
	v3    normal;
	v4    extra;
} tg_transvoxel_vertex;

typedef struct tg_transvoxel_block
{
	v3i                        coordinates;
	u8                         lod;

	u16                        vertex_count;
	tg_transvoxel_vertex*      p_vertices;

	u16                        index_count;
	u16*                       p_indices;
	
	tg_transvoxel_voxel_map    voxel_map;
} tg_transvoxel_block;



typedef struct tg_transvoxel_reuse_cell
{
	u16    p_indices[4];
} tg_transvoxel_reuse_cell;

typedef struct tg_transvoxel_reuse_cells
{
	tg_transvoxel_reuse_cell    pp_decks[2][TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING * TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING * TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING];
} tg_transvoxel_reuse_cells;



typedef struct tg_transvoxel_terrain
{
	tg_transvoxel_block    block;
	tg_entity              entity;
} tg_transvoxel_terrain;

#endif

#endif
