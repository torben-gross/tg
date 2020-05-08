#ifndef TG_GRAPHICS_VULKAN_TERRAIN_H
#define TG_GRAPHICS_VULKAN_TERRAIN_H

#include "graphics/vulkan/tg_graphics_vulkan.h"
#include "tg_terrain.h"

#ifdef TG_VULKAN

#define TG_VULKAN_TERRAIN_LOD_COUNT    4

#include "tg_entity.h"

typedef struct tg_terrain_chunk_entity_create_isolevel_ubo
{
	i32    chunk_index_x;
	i32    chunk_index_y;
	i32    chunk_index_z;
} tg_terrain_chunk_entity_create_isolevel_ubo;

typedef struct tg_terrain_chunk_entity_create_marching_cubes_ubo
{
	i32    chunk_index_x;
	i32    chunk_index_y;
	i32    chunk_index_z;
	u32    lod;
} tg_terrain_chunk_entity_create_marching_cubes_ubo;

typedef struct tg_terrain_chunk_entity
{
	tg_entity              entity;
	tg_storage_image_3d    isolevel_si3d;
	tg_mesh                p_lod_meshes[TG_VULKAN_MAX_LOD_COUNT]; // TODO: remove, they are in the graphics data ptr
} tg_terrain_chunk_entity;

#endif

#endif
