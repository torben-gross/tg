#include "tg_terrain.h"
#include "graphics/vulkan/tg_graphics_vulkan_terrain.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_entity.h"
#include <string.h> // TODO: implement yourself


#define TG_TERRAIN_HASH(x, y, z)    (((u32)(x) * 9 + (u32)(y) * 37 + (u32)(z) * 149) % TG_TERRAIN_HASHMAP_COUNT) // TODO: better hash function!!!



tg_terrain_chunk* tg_terrain_internal_get_chunk(tg_terrain_h terrain_h, i32 x, i32 y, i32 z)
{
	const u32 original_hashed_index = TG_TERRAIN_HASH(x, y, z);

	u32 hash_index = original_hashed_index;
	tg_terrain_chunk* p_chunk = terrain_h->pp_chunks_hashmap[hash_index];

	if (!p_chunk)
	{
		return TG_NULL;
	}

	while (p_chunk->x != x || p_chunk->y != y || p_chunk->z != z)
	{
		hash_index = hash_index + 1 == TG_TERRAIN_HASHMAP_COUNT ? 0 : hash_index++;
		p_chunk = terrain_h->pp_chunks_hashmap[hash_index];

		if (!p_chunk || hash_index == original_hashed_index)
		{
			return TG_NULL;
		}
	}

	return p_chunk;
}

b32 tg_terrain_internal_set_chunk(tg_terrain_h terrain_h, i32 x, i32 y, i32 z, tg_terrain_chunk* p_chunk)
{
	b32 result = TG_FALSE;

	if (terrain_h->chunk_count + 1 < TG_TERRAIN_HASHMAP_COUNT)
	{
		u32 hash_index = TG_TERRAIN_HASH(x, y, z);
		while (terrain_h->pp_chunks_hashmap[hash_index])
		{
			hash_index = hash_index + 1 == TG_TERRAIN_HASHMAP_COUNT ? 0 : hash_index++;
		}
		terrain_h->pp_chunks_hashmap[hash_index] = p_chunk;
		terrain_h->chunk_count++;
	}

	return result;
}

void tg_terrain_internal_fill_isolevels(i32 x, i32 y, i32 z, tg_transvoxel_isolevels* p_isolevels)
{
	TG_ASSERT(p_isolevels);

	const f32 offset_x = 16.0f * (f32)x;
	const f32 offset_y = 16.0f * (f32)y;
	const f32 offset_z = 16.0f * (f32)z;

	for (i32 cz = 0; cz < 17; cz++)
	{
		for (i32 cy = 0; cy < 17; cy++)
		{
			for (i32 cx = 0; cx < 17; cx++)
			{
				const f32 bias = 1024.0f; // TODO: why do it need bias? or rather, why do negative values for noise_x or noise_y result in cliffs?
				const f32 base_x = 16.0f * (f32)x + bias + (f32)cx;
				const f32 base_y = 16.0f * (f32)y + bias + (f32)cy;
				const f32 base_z = 16.0f * (f32)z + bias + (f32)cz;

				const f32 n_hills0 = tgm_noise(base_x * 0.008f, 0.0f, base_z * 0.008f);
				const f32 n_hills1 = tgm_noise(base_x * 0.2f, 0.0f, base_z * 0.2f);
				const f32 n_hills = n_hills0;// +0.01f * n_hills1;

				const f32 s_caves = 0.06f;
				const f32 n_caves = tgm_f32_clamp(tgm_noise(s_caves * base_x, s_caves * base_y, s_caves * base_z), -1.0f, 0.0f);

				const f32 n = n_hills;
				f32 noise = (n * 64.0f) - ((f32)cy + (f32)y * 16.0f);
				//noise += 5.0f * n_caves;
				const f32 noise_clamped = tgm_f32_clamp(noise, -1.0f, 1.0f);
				const f32 f0 = (noise_clamped + 1.0f) * 0.5f;
				const f32 f1 = 254.0f * f0;
				i8 f2 = -(i8)(tgm_f32_round_to_i32(f1) - 127);
				TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, cx, cy, cz) = f2;
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
	const u8 lod = (u8)tgm_u64_min((u64)(d / 8.0f), 4);
	return lod;
}



tg_terrain_h tg_terrain_create(tg_camera_h focal_point)
{
	tg_terrain_h terrain_h = TG_MEMORY_ALLOC(sizeof(*terrain_h));

	terrain_h->focal_point = focal_point;
	terrain_h->vertex_shader_h = tg_vertex_shader_create("shaders/deferred.vert");
	terrain_h->fragment_shader_h = tg_fragment_shader_create("shaders/deferred.frag");
	terrain_h->material_h = tg_material_create_deferred(terrain_h->vertex_shader_h, terrain_h->fragment_shader_h, 0, TG_NULL);

	terrain_h->next_memory_block_index = 0;
	memset(terrain_h->pp_chunks_hashmap, 0, TG_TERRAIN_MAX_CHUNK_COUNT * sizeof(*terrain_h->pp_chunks_hashmap));

	const v3 fp = tg_camera_get_position(focal_point);

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
				tg_terrain_internal_fill_isolevels(x, y, z, &p_chunk->isolevels);
				p_chunk->lod = tg_terrain_internal_select_lod(&fp, x, y, z);
				p_chunk->transitions = TG_TRANSVOXEL_TRANSITION_FACES(
					p_chunk->lod > tg_terrain_internal_select_lod(&fp, x - 1, y, z),
					p_chunk->lod > tg_terrain_internal_select_lod(&fp, x + 1, y, z),
					p_chunk->lod > tg_terrain_internal_select_lod(&fp, x, y - 1, z),
					p_chunk->lod > tg_terrain_internal_select_lod(&fp, x, y + 1, z),
					p_chunk->lod > tg_terrain_internal_select_lod(&fp, x, y, z - 1),
					p_chunk->lod > tg_terrain_internal_select_lod(&fp, x, y, z + 1)
				);
				p_chunk->triangle_count = 0;

				TG_ASSERT(tgm_i32_abs(p_chunk->lod - tg_terrain_internal_select_lod(&fp, x - 1, y, z)) <= 1);
				TG_ASSERT(tgm_i32_abs(p_chunk->lod - tg_terrain_internal_select_lod(&fp, x + 1, y, z)) <= 1);
				TG_ASSERT(tgm_i32_abs(p_chunk->lod - tg_terrain_internal_select_lod(&fp, x, y - 1, z)) <= 1);
				TG_ASSERT(tgm_i32_abs(p_chunk->lod - tg_terrain_internal_select_lod(&fp, x, y + 1, z)) <= 1);
				TG_ASSERT(tgm_i32_abs(p_chunk->lod - tg_terrain_internal_select_lod(&fp, x, y, z - 1)) <= 1);
				TG_ASSERT(tgm_i32_abs(p_chunk->lod - tg_terrain_internal_select_lod(&fp, x, y, z + 1)) <= 1);

				p_chunk->entity = tg_entity_create(TG_NULL, terrain_h->material_h);
				p_chunk->isolevels;
				tg_terrain_internal_set_chunk(terrain_h, x, y, z, p_chunk);
			}
		}
	}

	u32 index = 0;
	const u64 triangles_size = (TG_MAX_TRIANGLES_PER_CHUNK + TG_MAX_TRIANGLES_FOR_TRANSITIONS) * sizeof(tg_transvoxel_triangle);
	tg_transvoxel_triangle* p_triangles = TG_MEMORY_ALLOC(triangles_size);
	for (i32 z = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; z < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; z++)
	{
		for (i32 y = -2; y < 3; y++)
		{
			for (i32 x = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; x < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; x++)
			{
				tg_terrain_chunk* p_terrain_chunk = tg_terrain_internal_get_chunk(terrain_h, x, y, z);
				
				TG_ASSERT(p_terrain_chunk);

				tg_terrain_chunk* pp_connecting_chunks[27] = { 0 };
				tg_transvoxel_isolevels* pp_connecting_isolevels[27] = { 0 };
				for (i8 iz = -1; iz < 2; iz++)
				{
					for (i8 iy = -1; iy < 2; iy++)
					{
						for (i8 ix = -1; ix < 2; ix++)
						{
							const u8 i = 9 * (iz + 1) + 3 * (iy + 1) + (ix + 1);
							pp_connecting_chunks[i] = tg_terrain_internal_get_chunk(terrain_h, x + ix, y + iy, z + iz);
							pp_connecting_isolevels[i] = pp_connecting_chunks[i] ? &pp_connecting_chunks[i]->isolevels : TG_NULL;
						}
					}
				}

				p_terrain_chunk->triangle_count = tg_transvoxel_create_chunk(p_terrain_chunk->x, p_terrain_chunk->y, p_terrain_chunk->z, &p_terrain_chunk->isolevels, pp_connecting_isolevels, p_terrain_chunk->lod, p_terrain_chunk->transitions, p_triangles);
				if (p_terrain_chunk->triangle_count)
				{
					tg_entity_set_mesh(&p_terrain_chunk->entity, tg_mesh_create2(3 * p_terrain_chunk->triangle_count, (tg_vertex_3d*)p_triangles), 0);
				}
			}
		}
	}
	TG_MEMORY_FREE(p_triangles);

	return terrain_h;
}

void tg_terrain_update(tg_terrain_h terrain_h)
{
	TG_ASSERT(terrain_h);

	const v3 focal_point = tg_camera_get_position(terrain_h->focal_point);
	for (i32 z = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; z < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; z++)
	{
		for (i32 y = -2; y < 3; y++)
		{
			for (i32 x = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; x < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; x++)
			{
				const u8 lod = tg_terrain_internal_select_lod(&focal_point, x, y, z);
				const u8 transitions = TG_TRANSVOXEL_TRANSITION_FACES(
					lod > tg_terrain_internal_select_lod(&focal_point, x - 1, y, z),
					lod > tg_terrain_internal_select_lod(&focal_point, x + 1, y, z),
					lod > tg_terrain_internal_select_lod(&focal_point, x, y - 1, z),
					lod > tg_terrain_internal_select_lod(&focal_point, x, y + 1, z),
					lod > tg_terrain_internal_select_lod(&focal_point, x, y, z - 1),
					lod > tg_terrain_internal_select_lod(&focal_point, x, y, z + 1)
				);

				tg_terrain_chunk* p_chunk = tg_terrain_internal_get_chunk(terrain_h, x, y, z);

				if (p_chunk)
				{
					if (p_chunk->lod != lod || p_chunk->transitions != transitions)
					{
						// TODO: regenerate...
					}
					p_chunk->lod = lod;
					p_chunk->transitions = transitions;
				}
			}
		}
	}
}

void tg_terrain_destroy(tg_terrain_h terrain_h)
{
	TG_ASSERT(terrain_h);

	TG_MEMORY_FREE(terrain_h);
	TG_ASSERT(TG_FALSE);
}

#endif
