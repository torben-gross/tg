#ifndef TG_GRAPHICS_VULKAN_TERRAIN_H
#define TG_GRAPHICS_VULKAN_TERRAIN_H

#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "math/tg_math.h"
#include "tg_entity.h"
#include "tg_transvoxel.h"



#define TG_TERRAIN_VIEW_DISTANCE_CHUNKS           4
#define TG_TERRAIN_MAX_CHUNK_COUNT                65536
#define TG_TERRAIN_HASHMAP_COUNT                  65536
#define TG_TERRAIN_CHUNK_CENTER(terrain_chunk)    ((v3){ 16.0f * (f32)(terrain_chunk).x + 8.0f, 16.0f * (f32)(terrain_chunk).y + 8.0f, 16.0f * (f32)(terrain_chunk).z + 8.0f })

#define TG_MAX_TRIANGLES_PER_CHUNK                ((TG_TRANSVOXEL_MAX_TRIANGLES_PER_REGULAR_CELL) * 16 * 16 * 16)
#define TG_MAX_TRIANGLES_FOR_TRANSITIONS          ((TG_TRANSVOXEL_MAX_TRIANGLES_PER_TRANSITION_CELL) * 6 * 8 * 8)



typedef struct tg_terrain_chunk
{
	i32                        x;
	i32                        y;
	i32                        z;
	tg_transvoxel_isolevels    isolevels;
	u8                         lod;
	u8                         transitions;
	u32                        triangle_count;
	tg_entity                  entity;
} tg_terrain_chunk;

typedef struct tg_terrain
{
	tg_camera_h             focal_point;
	tg_vertex_shader_h      vertex_shader_h;
	tg_fragment_shader_h    fragment_shader_h;
	tg_material_h           material_h;

	u32                     next_memory_block_index;
	tg_terrain_chunk        p_memory_blocks[TG_TERRAIN_MAX_CHUNK_COUNT];
	// TODO: free-list?

	u32                     chunk_count;
	tg_terrain_chunk*       pp_chunks_hashmap[TG_TERRAIN_HASHMAP_COUNT];
} tg_terrain;

#endif

#endif
