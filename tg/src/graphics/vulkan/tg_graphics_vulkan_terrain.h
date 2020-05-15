#ifndef TG_GRAPHICS_VULKAN_TERRAIN_H
#define TG_GRAPHICS_VULKAN_TERRAIN_H

#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "math/tg_math.h"
#include "tg_entity.h"



#define TG_TERRAIN_VIEW_DISTANCE_CHUNKS           3
#define TG_TERRAIN_MAX_CHUNK_COUNT                512
#define TG_TERRAIN_CHUNK_CENTER(terrain_chunk)    ((v3){ 16.0f * (f32)(terrain_chunk).x + 8.0f, 16.0f * (f32)(terrain_chunk).y + 8.0f, 16.0f * (f32)(terrain_chunk).z + 8.0f })



typedef struct tg_terrain_chunk
{
	i32          x;
	i32          y;
	i32          z;
	u8           lod;
	u8           transitions;
	u32          triangle_count;
	tg_entity    entity;
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
	tg_terrain_chunk*       pp_chunks[TG_TERRAIN_MAX_CHUNK_COUNT];
} tg_terrain;

#endif

#endif
