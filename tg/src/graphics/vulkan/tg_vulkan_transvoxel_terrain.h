#ifndef TG_VULKAN_TRANSVOXEL_TERRAIN_H
#define TG_VULKAN_TRANSVOXEL_TERRAIN_H

#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "math/tg_math.h"
#include "tg_entity.h"
#include "tg_transvoxel_terrain.h"



#define TG_TRANSVOXEL_CELLS                               16
#define TG_TRANSVOXEL_CELLS_PER_CHUNK                     (TG_TRANSVOXEL_CELLS * TG_TRANSVOXEL_CELLS * TG_TRANSVOXEL_CELLS)
#define TG_TRANSVOXEL_TRIANGLES_PER_CHUNK                 (5 * TG_TRANSVOXEL_CELLS_PER_CHUNK)
#define TG_TRANSVOXEL_VERTICES_PER_CHUNK                  (15 * TG_TRANSVOXEL_CELLS_PER_CHUNK)
#define TG_TRANSVOXEL_VOXELS                              (TG_TRANSVOXEL_CELLS + 1)
#define TG_TRANSVOXEL_REQUIRED_VOXELS_PER_DIMENSION       (TG_TRANSVOXEL_CELLS + 3)
#define TG_TRANSVOXEL_REQUIRED_VOXELS_PER_CHUNK           (TG_TRANSVOXEL_REQUIRED_VOXELS_PER_DIMENSION * TG_TRANSVOXEL_REQUIRED_VOXELS_PER_DIMENSION * TG_TRANSVOXEL_REQUIRED_VOXELS_PER_DIMENSION)
#define TG_TRANSVOXEL_VOXEL_MAP_AT(voxel_map, x, y, z)    ((voxel_map).p_data[TG_TRANSVOXEL_REQUIRED_VOXELS_PER_DIMENSION * TG_TRANSVOXEL_REQUIRED_VOXELS_PER_DIMENSION * (z + 1) + TG_TRANSVOXEL_REQUIRED_VOXELS_PER_DIMENSION * (y + 1) + (x + 1)])
#define TG_TRANSVOXEL_VOXEL_MAP_AT_V3(voxel_map, v)       TG_TRANSVOXEL_VOXEL_MAP_AT(voxel_map, (v).x, (v).y, (v).z)



typedef struct tg_transvoxel_voxel_map
{
	i8    p_data[TG_TRANSVOXEL_REQUIRED_VOXELS_PER_CHUNK];
} tg_transvoxel_voxel_map;

typedef struct tg_transvoxel_vertex
{
	v3    position;
	v3    normal;
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



typedef struct tg_transvoxel_deck
{
	u16*    ppp_indices[TG_TRANSVOXEL_CELLS_PER_CHUNK][4];
} tg_transvoxel_deck;

typedef struct tg_transvoxel_history
{
	u16                    next_index;
	tg_transvoxel_deck*    p_deck;
	tg_transvoxel_deck*    p_preceding_deck;
} tg_transvoxel_history;

// TODO: we need 2 decks of 16x16 for history and ping pong between them, as z is incremented.


typedef struct tg_transvoxel_terrain
{
	tg_transvoxel_block    block;
	tg_entity              entity;
} tg_transvoxel_terrain;

#endif

#endif
