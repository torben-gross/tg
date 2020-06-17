#if 0

#ifndef TG_GRAPHICS_VULKAN_TERRAIN_H
#define TG_GRAPHICS_VULKAN_TERRAIN_H

#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "math/tg_math.h"
#include "tg_entity.h"
#include "tg_flat_transvoxel.h"



#define TG_TERRAIN_VIEW_DISTANCE_CHUNKS      3
#define TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y    3
#define TG_TERRAIN_MAX_CHUNK_COUNT           262144
#define TG_TERRAIN_HASHMAP_COUNT             262144

#define TG_TERRAIN_CHUNK_CENTER(x, y, z)     ((v3){ \
	(f32)(TG_TRANSVOXEL_CELLS * (x) + (TG_TRANSVOXEL_CELLS / 2)), \
	(f32)(TG_TRANSVOXEL_CELLS * (y) + (TG_TRANSVOXEL_CELLS / 2)), \
	(f32)(TG_TRANSVOXEL_CELLS * (z) + (TG_TRANSVOXEL_CELLS / 2)) })



typedef enum tg_terrain_flags
{
	TG_TERRAIN_FLAG_REGENERATE    = (1 << 0)
} tg_terrain_flags;

typedef struct tg_terrain_chunk
{
	u32                     flags;
	tg_transvoxel_chunk*    p_transvoxel_chunk;
	tg_entity               entity;
} tg_terrain_chunk;

typedef struct tg_terrain
{
	u32                     flags;
	tg_camera_h             focal_point;
	v3                      last_focal_point_position;
	tg_vertex_shader_h      h_vertex_shader;
	tg_fragment_shader_h    h_fragment_shader;
	tg_material_h           h_material;

	tg_terrain_chunk        p_memory_blocks[TG_TERRAIN_MAX_CHUNK_COUNT];
	// TODO: free-list?

	u32                     chunk_count;
	tg_terrain_chunk*       pp_chunks_hashmap[TG_TERRAIN_HASHMAP_COUNT];
} tg_terrain;

#endif

#endif

#endif
