#include "tg_terrain.h"
#include "graphics/vulkan/tg_graphics_vulkan_terrain.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_entity.h"
#include "tg_transvoxel.h"
#include <string.h> // TODO: implement yourself


void tg_terrain_fill_isolevels(i32 x, i32 y, i32 z, tg_transvoxel_isolevels* p_isolevels)
{
	TG_ASSERT(p_isolevels);

	const f32 offset_x = 16.0f * (f32)x;
	const f32 offset_y = 16.0f * (f32)y;
	const f32 offset_z = 16.0f * (f32)z;

	for (i32 cz = -1; cz < 18; cz++)
	{
		for (i32 cy = -1; cy < 18; cy++)
		{
			for (i32 cx = -1; cx < 18; cx++)
			{
				const f32 bias = 1024.0f; // TODO: why do it need bias? or rather, why do negative values for noise_x or noise_y result in cliffs?
				const f32 base_x = 16.0f * (f32)x + bias + (f32)cx;
				const f32 base_y = 16.0f * (f32)y + bias + (f32)cy;
				const f32 base_z = 16.0f * (f32)z + bias + (f32)cz;

				const f32 n_hills0 = tgm_noise(base_x * 0.008f, 0.0f, base_z * 0.008f);
				const f32 n_hills1 = tgm_noise(base_x * 0.2f, 0.0f, base_z * 0.2f);
				const f32 n_hills = n_hills0 + 0.01f * n_hills1;

				const f32 s_caves = 0.06f;
				const f32 n_caves = tgm_noise(s_caves * base_x, s_caves * base_y, s_caves * base_z);

				const f32 n = n_hills;
				f32 noise = (n * 64.0f) - ((f32)cy + (f32)y * 16.0f);
				noise += 5.0f * n_caves;
				const f32 noise_clamped = tgm_f32_clamp(noise, -1.0f, 1.0f);
				const f32 f0 = (noise_clamped + 1.0f) * 0.5f;
				const f32 f1 = 254.0f * f0;
				i8 f2 = -(i8)(tgm_f32_round_to_i32(f1) - 127);
				TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, cx, cy, cz) = f2;

				if (x == 3 && y == -1 && z == -4)
				{
					int dd = 0;
					if (cx == 9 && cy == 13 && cz == 5)
					{
						dd = 0;
					}
					else if (cx == 10 && cy == 13 && cz == 5)
					{
						dd = 0;
					}
					else if (cx == 9 && cy == 14 && cz == 5)
					{
						dd = 0;
					}
					else if (cx == 10 && cy == 14 && cz == 5)
					{
						dd = 0;
					}
					else if (cx == 9 && cy == 13 && cz == 6)
					{
						dd = 0;
					}
					else if (cx == 10 && cy == 13 && cz == 6)
					{
						dd = 0;
					}
					else if (cx == 9 && cy == 14 && cz == 6)
					{
						dd = 0;
					}
					else if (cx == 10 && cy == 14 && cz == 6)
					{
						dd = 0;
					}
				}
			}
		}
	}
}

f32 tg_terrain_internal_chunk_distance_to_focal_point(const v3* p_focal_point, const tg_terrain_chunk* p_terrain_chunk)
{
	const v3 chunk_center = TG_TERRAIN_CHUNK_CENTER(*p_terrain_chunk);
	const v3 v = tgm_v3_subtract_v3(p_focal_point, &chunk_center);
	const f32 d = tgm_v3_magnitude_squared(&v);
	return d;
}

u8 tg_terrain_internal_select_lod(const v3* p_focal_point, i32 x, i32 y, i32 z)
{
	const v3 v = { p_focal_point->x - (f32)x, p_focal_point->y - (f32)y, p_focal_point->z - (f32)z };
	const f32 d = tgm_v3_magnitude_squared(&v);
	const u8 lod = (u8)tgm_u64_min((u64)(d / 32.0f), 4);
	return lod;
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

	u8 matid = 0;
	for (i32 z = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; z < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; z++)
	{
		for (i32 y = -2; y < 3; y++)
		{
			for (i32 x = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; x < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; x++)
			{
				tg_terrain_chunk* p_chunk = &terrain_h->p_memory_blocks[terrain_h->next_memory_block_index++];
				p_chunk->x = x;
				p_chunk->y = y;
				p_chunk->z = z;
				p_chunk->lod = tg_terrain_internal_select_lod(&fp, x, y, z);
				p_chunk->transitions = TG_TRANSVOXEL_TRANSITION_FACES(
					p_chunk->lod > tg_terrain_internal_select_lod(&fp, x - 1, y, z),
					p_chunk->lod > tg_terrain_internal_select_lod(&fp, x + 1, y, z),
					p_chunk->lod > tg_terrain_internal_select_lod(&fp, x, y - 1, z),
					p_chunk->lod > tg_terrain_internal_select_lod(&fp, x, y + 1, z),
					p_chunk->lod > tg_terrain_internal_select_lod(&fp, x, y, z - 1),
					p_chunk->lod > tg_terrain_internal_select_lod(&fp, x, y, z + 1)
				);
				p_chunk->entity = tg_entity_create(TG_NULL, terrain_h->material_h);
				matid++;
				if (matid == 3)
				{
					matid = 0;
				}
				terrain_h->pp_chunks[terrain_h->chunk_count++] = p_chunk;
			}
		}
	}

	tg_transvoxel_triangle* p_triangles = TG_MEMORY_ALLOC(5 * 16 * 16 * 16 * sizeof(*p_triangles));
	tg_transvoxel_isolevels isolevels = { 0 };
	for (u32 i = 0; i < terrain_h->chunk_count; i++)
	{
		tg_terrain_chunk* p_terrain_chunk = terrain_h->pp_chunks[i];
		tg_terrain_fill_isolevels(p_terrain_chunk->x, p_terrain_chunk->y, p_terrain_chunk->z, &isolevels);
		p_terrain_chunk->triangle_count = tg_transvoxel_create_chunk(p_terrain_chunk->x, p_terrain_chunk->y, p_terrain_chunk->z, &isolevels, p_terrain_chunk->lod, p_terrain_chunk->transitions, p_triangles);
		if (p_terrain_chunk->triangle_count)
		{
			tg_entity_set_mesh(&p_terrain_chunk->entity, tg_mesh_create(3 * p_terrain_chunk->triangle_count, (v3*)p_triangles, TG_NULL, TG_NULL, TG_NULL, 0, TG_NULL), 0);
		}
	}
	TG_MEMORY_FREE(p_triangles);

	return terrain_h;
}

void tg_terrain_update(tg_terrain_h terrain_h)
{
	TG_ASSERT(terrain_h);

	const v3 focal_point = tg_camera_get_position(terrain_h->focal_point);
	for (u32 i = 0; i < terrain_h->chunk_count; i++)
	{
		const u32 x = terrain_h->pp_chunks[i]->x;
		const u32 y = terrain_h->pp_chunks[i]->y;
		const u32 z = terrain_h->pp_chunks[i]->z;

		const u8 lod = tg_terrain_internal_select_lod(&focal_point, x, y, z);
		const u8 transitions = TG_TRANSVOXEL_TRANSITION_FACES(
			lod > tg_terrain_internal_select_lod(&focal_point, x - 1, y, z),
			lod > tg_terrain_internal_select_lod(&focal_point, x + 1, y, z),
			lod > tg_terrain_internal_select_lod(&focal_point, x, y - 1, z),
			lod > tg_terrain_internal_select_lod(&focal_point, x, y + 1, z),
			lod > tg_terrain_internal_select_lod(&focal_point, x, y, z - 1),
			lod > tg_terrain_internal_select_lod(&focal_point, x, y, z + 1)
		);

		if (terrain_h->pp_chunks[i]->lod != lod || terrain_h->pp_chunks[i]->transitions != transitions)
		{
			// TODO: regenerate...
		}
		terrain_h->pp_chunks[i]->lod = lod;
		terrain_h->pp_chunks[i]->transitions = transitions;
	}
}

void tg_terrain_destroy(tg_terrain_h terrain_h)
{
	TG_ASSERT(terrain_h);

	TG_MEMORY_FREE(terrain_h);
	TG_ASSERT(TG_FALSE);
}

#endif
