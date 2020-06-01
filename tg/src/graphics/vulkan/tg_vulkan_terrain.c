#include "tg_terrain.h"
#include "graphics/vulkan/tg_vulkan_terrain.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_entity.h"
#include <string.h> // TODO: implement memset yourself


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

	while (p_chunk->p_transvoxel_chunk->coordinates.x != x || p_chunk->p_transvoxel_chunk->coordinates.y != y || p_chunk->p_transvoxel_chunk->coordinates.z != z)
	{
		hash_index = hash_index + 1 == TG_TERRAIN_HASHMAP_COUNT ? 0 : ++hash_index;
		p_chunk = terrain_h->pp_chunks_hashmap[hash_index];

		if (!p_chunk || hash_index == original_hashed_index)
		{
			return TG_NULL;
		}
	}

	return p_chunk;
}

b32 tg_terrain_internal_store_chunk(tg_terrain_h terrain_h, i32 x, i32 y, i32 z, tg_terrain_chunk* p_chunk)
{
	b32 result = TG_FALSE;

	if (terrain_h->chunk_count + 1 < TG_TERRAIN_HASHMAP_COUNT)
	{
		u32 hash_index = TG_TERRAIN_HASH(x, y, z);
		while (terrain_h->pp_chunks_hashmap[hash_index])
		{
			hash_index = hash_index + 1 == TG_TERRAIN_HASHMAP_COUNT ? 0 : ++hash_index;
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
				const f32 n_hills = n_hills0 + 0.01f * n_hills1;

				const f32 s_caves = 0.06f;
				const f32 n_caves = tgm_f32_clamp(tgm_noise(s_caves * base_x, s_caves * base_y, s_caves * base_z), -1.0f, 0.0f);

				const f32 n = n_hills;
				f32 noise = (n * 64.0f) - ((f32)cy + (f32)y * 16.0f);
				noise += 15.0f * n_caves;
				const f32 noise_clamped = tgm_f32_clamp(noise, -1.0f, 1.0f);
				const f32 f0 = (noise_clamped + 1.0f) * 0.5f;
				const f32 f1 = 254.0f * f0;
				i8 f2 = -(i8)(tgm_f32_round_to_i32(f1) - 127);
				TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, cx, cy, cz) = f2;
			}
		}
	}
}

u8 tg_terrain_internal_select_lod(v3 focal_point, i32 x, i32 y, i32 z)
{
	const v3 chunk_center = {
		(f32)(x * TG_TRANSVOXEL_CELLS + (TG_TRANSVOXEL_CELLS / 2)),
		(f32)(y * TG_TRANSVOXEL_CELLS + (TG_TRANSVOXEL_CELLS / 2)),
		(f32)(z * TG_TRANSVOXEL_CELLS + (TG_TRANSVOXEL_CELLS / 2))
	};
	const v3 delta = tgm_v3_subtract((v3){ focal_point.x, 0.0f, focal_point.z }, chunk_center);
	const f32 distance = tgm_v3_magnitude(delta);
	const u8 lod = (u8)tgm_u64_min((u64)(distance / 64.0f), 3);
	return lod;
}



tg_terrain_h tg_terrain_create(tg_camera_h focal_point_camera_h)
{
	tg_terrain_h terrain_h = TG_MEMORY_ALLOC(sizeof(*terrain_h));

	terrain_h->focal_point = focal_point_camera_h;
	terrain_h->vertex_shader_h = tg_vertex_shader_create("shaders/deferred.vert");
	terrain_h->fragment_shader_h = tg_fragment_shader_create("shaders/deferred.frag");
	terrain_h->material_h = tg_material_create_deferred(terrain_h->vertex_shader_h, terrain_h->fragment_shader_h, 0, TG_NULL);

	terrain_h->next_memory_block_index = 0;
	memset(terrain_h->pp_chunks_hashmap, 0, TG_TERRAIN_MAX_CHUNK_COUNT * sizeof(*terrain_h->pp_chunks_hashmap));

	const v3 focal_point = tg_camera_get_position(focal_point_camera_h);

	for (i32 z = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; z < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; z++)
	{
		for (i32 y = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y; y < TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y + 1; y++)
		{
			for (i32 x = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; x < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; x++)
			{
				tg_terrain_chunk* p_terrain_chunk = &terrain_h->p_memory_blocks[terrain_h->next_memory_block_index++];

				p_terrain_chunk->p_transvoxel_chunk = TG_MEMORY_ALLOC(sizeof(*p_terrain_chunk->p_transvoxel_chunk));
				p_terrain_chunk->p_transvoxel_chunk->coordinates = (v3i){ x, y, z };
				p_terrain_chunk->p_transvoxel_chunk->lod = tg_terrain_internal_select_lod(focal_point, x, y, z);
				tg_terrain_internal_fill_isolevels(x, y, z, &p_terrain_chunk->p_transvoxel_chunk->isolevels);
				p_terrain_chunk->p_transvoxel_chunk->transition_faces_bitmap = TG_TRANSVOXEL_TRANSITION_FACES(
					p_terrain_chunk->p_transvoxel_chunk->lod > tg_terrain_internal_select_lod(focal_point, x - 1, y, z),
					p_terrain_chunk->p_transvoxel_chunk->lod > tg_terrain_internal_select_lod(focal_point, x + 1, y, z),
					p_terrain_chunk->p_transvoxel_chunk->lod > tg_terrain_internal_select_lod(focal_point, x, y - 1, z),
					p_terrain_chunk->p_transvoxel_chunk->lod > tg_terrain_internal_select_lod(focal_point, x, y + 1, z),
					p_terrain_chunk->p_transvoxel_chunk->lod > tg_terrain_internal_select_lod(focal_point, x, y, z - 1),
					p_terrain_chunk->p_transvoxel_chunk->lod > tg_terrain_internal_select_lod(focal_point, x, y, z + 1)
				);
				p_terrain_chunk->p_transvoxel_chunk->triangle_count = 0;
				p_terrain_chunk->p_transvoxel_chunk->p_triangles = TG_MEMORY_ALLOC(TG_TRANSVOXEL_CHUNK_MAX_TRIANGLES * sizeof(tg_transvoxel_triangle));

				TG_ASSERT(tgm_i32_abs(p_terrain_chunk->p_transvoxel_chunk->lod - tg_terrain_internal_select_lod(focal_point, x - 1, y, z)) <= 1);
				TG_ASSERT(tgm_i32_abs(p_terrain_chunk->p_transvoxel_chunk->lod - tg_terrain_internal_select_lod(focal_point, x + 1, y, z)) <= 1);
				TG_ASSERT(tgm_i32_abs(p_terrain_chunk->p_transvoxel_chunk->lod - tg_terrain_internal_select_lod(focal_point, x, y - 1, z)) <= 1);
				TG_ASSERT(tgm_i32_abs(p_terrain_chunk->p_transvoxel_chunk->lod - tg_terrain_internal_select_lod(focal_point, x, y + 1, z)) <= 1);
				TG_ASSERT(tgm_i32_abs(p_terrain_chunk->p_transvoxel_chunk->lod - tg_terrain_internal_select_lod(focal_point, x, y, z - 1)) <= 1);
				TG_ASSERT(tgm_i32_abs(p_terrain_chunk->p_transvoxel_chunk->lod - tg_terrain_internal_select_lod(focal_point, x, y, z + 1)) <= 1);

				tg_transvoxel_fill_chunk(p_terrain_chunk->p_transvoxel_chunk);

				p_terrain_chunk->entity = tg_entity_create(TG_NULL, terrain_h->material_h);
				tg_terrain_internal_store_chunk(terrain_h, x, y, z, p_terrain_chunk);
			}
		}
	}

	tg_transvoxel_connecting_chunks transvoxel_connecting_chunks = { 0 };
	for (i32 z = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; z < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; z++)
	{
		for (i32 y = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y; y < TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y + 1; y++)
		{
			for (i32 x = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; x < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; x++)
			{
				tg_terrain_chunk* p_terrain_chunk = tg_terrain_internal_get_chunk(terrain_h, x, y, z);
				TG_ASSERT(p_terrain_chunk);

				if (p_terrain_chunk->p_transvoxel_chunk->transition_faces_bitmap)
				{
					for (i8 iz = -1; iz < 2; iz++)
					{
						for (i8 iy = -1; iy < 2; iy++)
						{
							for (i8 ix = -1; ix < 2; ix++)
							{
								const u8 i = 9 * (iz + 1) + 3 * (iy + 1) + (ix + 1);
								tg_terrain_chunk* p_connecting_terrain_chunk = tg_terrain_internal_get_chunk(terrain_h, x + ix, y + iy, z + iz);
								if (p_connecting_terrain_chunk)
								{
									transvoxel_connecting_chunks.pp_chunks[i] = p_connecting_terrain_chunk->p_transvoxel_chunk;
								}
								else
								{
									transvoxel_connecting_chunks.pp_chunks[i] = TG_NULL;
								}
							}
						}
					}
					tg_transvoxel_fill_transitions(&transvoxel_connecting_chunks);
				}
				if (p_terrain_chunk->p_transvoxel_chunk->triangle_count)
				{
					tg_entity_set_mesh(&p_terrain_chunk->entity, tg_mesh_create2(3 * p_terrain_chunk->p_transvoxel_chunk->triangle_count, (tg_vertex*)p_terrain_chunk->p_transvoxel_chunk->p_triangles), 0);
				}
			}
		}
	}

	return terrain_h;
}

void tg_terrain_update(tg_terrain_h terrain_h)
{
	TG_ASSERT(terrain_h);
	
	const v3 focal_point = tg_camera_get_position(terrain_h->focal_point);

	for (i32 z = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; z < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; z++)
	{
		for (i32 y = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y; y < TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y + 1; y++)
		{
			for (i32 x = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; x < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; x++)
			{
				tg_terrain_chunk* p_terrain_chunk = tg_terrain_internal_get_chunk(terrain_h, x, y, z);
	
				if (p_terrain_chunk)
				{
					const u8 lod = tg_terrain_internal_select_lod(focal_point, x, y, z);

					const u8 transition_faces_bitmap = TG_TRANSVOXEL_TRANSITION_FACES(
						lod > tg_terrain_internal_select_lod(focal_point, x - 1, y, z),
						lod > tg_terrain_internal_select_lod(focal_point, x + 1, y, z),
						lod > tg_terrain_internal_select_lod(focal_point, x, y - 1, z),
						lod > tg_terrain_internal_select_lod(focal_point, x, y + 1, z),
						lod > tg_terrain_internal_select_lod(focal_point, x, y, z - 1),
						lod > tg_terrain_internal_select_lod(focal_point, x, y, z + 1)
					);

					if (lod != p_terrain_chunk->p_transvoxel_chunk->lod || transition_faces_bitmap != p_terrain_chunk->p_transvoxel_chunk->transition_faces_bitmap)
					{
						p_terrain_chunk->regenerate = TG_TRUE;
						p_terrain_chunk->p_transvoxel_chunk->lod = lod;
						p_terrain_chunk->p_transvoxel_chunk->transition_faces_bitmap = transition_faces_bitmap;
					}
				}
				else
				{
					// TODO: create new...
				}
			}
		}
	}

	for (i32 z = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; z < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; z++)
	{
		for (i32 y = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y; y < TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y + 1; y++)
		{
			for (i32 x = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; x < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; x++)
			{
				tg_terrain_chunk* p_terrain_chunk = tg_terrain_internal_get_chunk(terrain_h, x, y, z);

				if (p_terrain_chunk && p_terrain_chunk->regenerate)
				{
					p_terrain_chunk->p_transvoxel_chunk->triangle_count = 0;
					tg_transvoxel_fill_chunk(p_terrain_chunk->p_transvoxel_chunk);
				}
			}
		}
	}

	tg_transvoxel_connecting_chunks transvoxel_connecting_chunks = { 0 };
	for (i32 z = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; z < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; z++)
	{
		for (i32 y = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y; y < TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y + 1; y++)
		{
			for (i32 x = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; x < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; x++)
			{
				tg_terrain_chunk* p_terrain_chunk = tg_terrain_internal_get_chunk(terrain_h, x, y, z);

				if (p_terrain_chunk && p_terrain_chunk->regenerate)
				{
					p_terrain_chunk->regenerate = TG_FALSE;

					if (p_terrain_chunk->p_transvoxel_chunk->transition_faces_bitmap)
					{
						for (i8 iz = -1; iz < 2; iz++)
						{
							for (i8 iy = -1; iy < 2; iy++)
							{
								for (i8 ix = -1; ix < 2; ix++)
								{
									const u8 i = 9 * (iz + 1) + 3 * (iy + 1) + (ix + 1);
									tg_terrain_chunk* p_connecting_terrain_chunk = tg_terrain_internal_get_chunk(terrain_h, x + ix, y + iy, z + iz);
									if (p_connecting_terrain_chunk)
									{
										transvoxel_connecting_chunks.pp_chunks[i] = p_connecting_terrain_chunk->p_transvoxel_chunk;
									}
									else
									{
										transvoxel_connecting_chunks.pp_chunks[i] = TG_NULL;
									}
								}
							}
						}
						tg_transvoxel_fill_transitions(&transvoxel_connecting_chunks);
					}
					if (p_terrain_chunk->p_transvoxel_chunk->triangle_count)
					{
						if (p_terrain_chunk->entity.graphics_data_ptr_h->p_lod_meshes_h[0])
						{
							tg_mesh_recreate2(
								p_terrain_chunk->entity.graphics_data_ptr_h->p_lod_meshes_h[0],
								3 * p_terrain_chunk->p_transvoxel_chunk->triangle_count,
								(tg_vertex*)p_terrain_chunk->p_transvoxel_chunk->p_triangles
							);
							tg_entity_graphics_data_ptr_set_mesh(p_terrain_chunk->entity.graphics_data_ptr_h, p_terrain_chunk->entity.graphics_data_ptr_h->p_lod_meshes_h[0], 0);
						}
						else
						{
							tg_entity_set_mesh(&p_terrain_chunk->entity, tg_mesh_create2(3 * p_terrain_chunk->p_transvoxel_chunk->triangle_count, (tg_vertex*)p_terrain_chunk->p_transvoxel_chunk->p_triangles), 0);
						}
					}
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
