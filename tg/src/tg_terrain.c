#include "tg_terrain.h"

#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"
#include "tg_entity.h"
#include "tg_transvoxel.h"
#include "util/tg_hashmap.h"
#include <string.h> // TODO: implement yourself



#define TG_TERRAIN_VIEW_DISTANCE_CHUNKS           3
#define TG_TERRAIN_MAX_CHUNK_COUNT                512
#define TG_TERRAIN_CHUNK_CENTER(terrain_chunk)    ((v3){ 16.0f * (f32)(terrain_chunk).x + 8.0f, 16.0f * (f32)(terrain_chunk).y + 8.0f, 16.0f * (f32)(terrain_chunk).z + 8.0f })



typedef struct tg_terrain_chunk
{
	i32          x;
	i32          y;
	i32          z;
	u8           lod;
	tg_entity    entity_h;
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





f32 tg_terrain_internal_chunk_distance_to_focal_point(const v3* p_focal_point, const tg_terrain_chunk* p_terrain_chunk)
{
	const v3 chunk_center = TG_TERRAIN_CHUNK_CENTER(*p_terrain_chunk);
	const v3 d = tgm_v3_subtract_v3(p_focal_point, &chunk_center);
	const f32 m = tgm_v3_magnitude_squared(&d);
	return m;
}

u8 tg_terrain_internal_select_lod(const v3* p_focal_point, i32 x, i32 y, i32 z)
{
	return 0; // TODO: see what m can be and select accordingly!
}

void tg_terrain_internal_qsort_swap(tg_terrain_chunk** pp_chunk0, tg_terrain_chunk** pp_chunk1)
{
	tg_terrain_chunk* p_temp = *pp_chunk0;
	*pp_chunk0 = *pp_chunk1;
	*pp_chunk1 = p_temp;
}

i32 tg_terrain_internal_qsort_partition(tg_terrain_h terrain_h, i32 low, i32 high)
{
	const tg_terrain_chunk* p_pivot = terrain_h->pp_chunks[high];
	const v3 focal_point = tg_camera_get_position(terrain_h->focal_point);
	const f32 distance_pivot = tg_terrain_internal_chunk_distance_to_focal_point(&focal_point, p_pivot);

	i32 i = low - 1;

	for (i32 j = low; j <= high - 1; j++)
	{
		const tg_terrain_chunk* p_chunk = terrain_h->pp_chunks[j];
		const f32 distance_chunk = tg_terrain_internal_chunk_distance_to_focal_point(&focal_point, p_chunk);
		if (distance_chunk < distance_pivot)
		{
			i++;
			tg_terrain_internal_qsort_swap(&terrain_h->pp_chunks[i], &terrain_h->pp_chunks[j]);
		}
	}
	tg_terrain_internal_qsort_swap(&terrain_h->pp_chunks[i + 1], &terrain_h->pp_chunks[high]);

	return i + 1;
}

void tg_terrain_internal_qsort(tg_terrain_h terrain_h, i32 low, i32 high)
{
	if (low < high)
	{
		const i32 pi = tg_terrain_internal_qsort_partition(terrain_h, low, high);
		tg_terrain_internal_qsort(terrain_h, low, pi - 1);
		tg_terrain_internal_qsort(terrain_h, pi + 1, high);
	}
}



tg_terrain_h tg_terrain_create(tg_camera_h focal_point)
{
	tg_terrain_h terrain_h = TG_MEMORY_ALLOC(sizeof(*terrain_h));

	terrain_h->focal_point = focal_point;
	terrain_h->vertex_shader_h = tg_vertex_shader_create("shaders/deferred.vert");
	terrain_h->fragment_shader_h = tg_fragment_shader_create("shaders/deferred.frag");
	terrain_h->material_h = tg_material_create_deferred(terrain_h->vertex_shader_h, terrain_h->fragment_shader_h, 0, TG_NULL);

	terrain_h->next_memory_block_index = 0;
	memset(terrain_h->p_memory_blocks, 0, TG_TERRAIN_MAX_CHUNK_COUNT * sizeof(*terrain_h->p_memory_blocks)); // TODO: do we need to set this to zero?
	terrain_h->chunk_count = 0;
	memset(terrain_h->pp_chunks, 0, TG_TERRAIN_MAX_CHUNK_COUNT * sizeof(*terrain_h->pp_chunks));

	const v3 fp = tg_camera_get_position(focal_point);

	for (i32 z = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; z < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; z++)
	{
		for (i32 y = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; y < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; y++)
		{
			for (i32 x = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; x < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; x++)
			{
				tg_terrain_chunk* p_chunk = &terrain_h->p_memory_blocks[terrain_h->next_memory_block_index++];
				p_chunk->x = x;
				p_chunk->y = y;
				p_chunk->z = z;
				p_chunk->lod = tg_terrain_internal_select_lod(&fp, x, y, z);
				p_chunk->entity_h = tg_entity_create(TG_NULL, terrain_h->material_h);
				terrain_h->pp_chunks[terrain_h->chunk_count++] = p_chunk;
			}
		}
	}
	tg_terrain_internal_qsort(terrain_h, 0, terrain_h->chunk_count - 1);

	tg_transvoxel_triangle* p_triangles = TG_MEMORY_ALLOC(5 * 16 * 16 * 16 * sizeof(*p_triangles));
	tg_transvoxel_isolevels isolevels = { 0 };
	for (u32 i = 0; i < terrain_h->chunk_count; i++)
	{
		tg_terrain_chunk* p_terrain_chunk = terrain_h->pp_chunks[i];
		tg_transvoxel_fill_isolevels(p_terrain_chunk->x, p_terrain_chunk->y, p_terrain_chunk->z, &isolevels);
		const u32 triangle_count = tg_transvoxel_create_chunk(p_terrain_chunk->x, p_terrain_chunk->y, p_terrain_chunk->z, &isolevels, 0, TG_TRANSVOXEL_TRANSITION_FACES(0, 0, 0, 0, 0, 0), p_triangles);
		// TODO: create mesh and actually render this
	}
	TG_MEMORY_FREE(p_triangles);

	return terrain_h;
}

void tg_terrain_update(tg_terrain_h terrain_h)
{
	TG_ASSERT(terrain_h);
}

void tg_terrain_render(tg_terrain_h terrain_h, tg_camera_h camera_h)
{
	TG_ASSERT(terrain_h);
}

void tg_terrain_destroy(tg_terrain_h terrain_h)
{
	TG_ASSERT(terrain_h);

	TG_MEMORY_FREE(terrain_h);
	TG_ASSERT(TG_FALSE);
}
