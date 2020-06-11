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

#define TG_TRANSVOXEL_VOXEL_MAP_AT(voxel_map, x, y, z)           ((voxel_map)[TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE_WITH_PADDING * TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE_WITH_PADDING * (z) + TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE_WITH_PADDING * (y) + (x)])
#define TG_TRANSVOXEL_VOXEL_MAP_AT_V3(voxel_map, v)              TG_TRANSVOXEL_VOXEL_MAP_AT(voxel_map, (v).x, (v).y, (v).z)



typedef struct tg_transvoxel_vertex
{
	v3    position;
	v3    normal;
	v4    extra;
} tg_transvoxel_vertex;

typedef struct tg_transvoxel_block
{
	v3i                      coordinates;
	u8                       lod;

	u16                      vertex_count;
	u16                      index_count;
	tg_transvoxel_vertex*    p_vertices;
	u16*                     p_indices;
	
	void*                    p_compressed_voxel_map;
} tg_transvoxel_block;



typedef struct tg_transvoxel_terrain
{
	tg_transvoxel_block    block;
	tg_entity              entity;
} tg_transvoxel_terrain;

#endif

#endif
