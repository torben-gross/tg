#include "tg_terrain.h"
#include "graphics/vulkan/tg_vulkan_terrain.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_entity.h"


#define TG_TERRAIN_HASH(x, y, z)          (((u32)(x) * 9 + (u32)(y) * 37 + (u32)(z) * 149) % TG_TERRAIN_HASHMAP_COUNT) // TODO: better hash function! also, mod
#define TG_TERRAIN_HASH_V3(v)             TG_TERRAIN_HASH((v).x, (v).y, (v).z)
#define TG_TERRAIN_INVALID_CHUNK_INDEX    TG_U32_MAX
#define TG_TERRAIN_CHUNKS_PER_LOD         2
#define TG_TERRAIN_MAX_LOD                3



v3i tg_terrain_internal_convert_to_chunk_coordinates(v3 v)
{
	const i32 x = (i32)((v.x < 0.0f ? (v.x - 1) : v.x) / (f32)TG_TRANSVOXEL_CELLS);
	const i32 y = (i32)((v.y < 0.0f ? (v.y - 1) : v.y) / (f32)TG_TRANSVOXEL_CELLS);
	const i32 z = (i32)((v.z < 0.0f ? (v.z - 1) : v.z) / (f32)TG_TRANSVOXEL_CELLS);
	const v3i result = { x, y, z };
	return result;
}

u32 tg_terrain_internal_find_chunk_index(tg_terrain_h terrain_h, tg_terrain_chunk* p_terrain_chunk)
{
	const u32 original_hash_index = TG_TERRAIN_HASH_V3(p_terrain_chunk->p_transvoxel_chunk->coordinates);

	u32 hash_index = original_hash_index;
	tg_terrain_chunk* p_current_terrain_chunk = terrain_h->pp_chunks_hashmap[hash_index];

	while (p_current_terrain_chunk != p_terrain_chunk)
	{
		hash_index = tgm_u32_incmod(hash_index, TG_TERRAIN_HASHMAP_COUNT);
		p_current_terrain_chunk = terrain_h->pp_chunks_hashmap[hash_index];

		if (!p_current_terrain_chunk || hash_index == original_hash_index)
		{
			return TG_TERRAIN_INVALID_CHUNK_INDEX;
		}
	}

	return hash_index;
}

tg_terrain_chunk* tg_terrain_internal_get_chunk(tg_terrain_h terrain_h, i32 x, i32 y, i32 z)
{
	const u32 original_hash_index = TG_TERRAIN_HASH(x, y, z);

	u32 hash_index = original_hash_index;
	tg_terrain_chunk* p_current_terrain_chunk = terrain_h->pp_chunks_hashmap[hash_index];

	if (!p_current_terrain_chunk)
	{
		return TG_NULL;
	}

	while (p_current_terrain_chunk->p_transvoxel_chunk->coordinates.x != x || p_current_terrain_chunk->p_transvoxel_chunk->coordinates.y != y || p_current_terrain_chunk->p_transvoxel_chunk->coordinates.z != z)
	{
		hash_index = tgm_u32_incmod(hash_index, TG_TERRAIN_HASHMAP_COUNT);
		p_current_terrain_chunk = terrain_h->pp_chunks_hashmap[hash_index];

		if (!p_current_terrain_chunk || hash_index == original_hash_index)
		{
			return TG_NULL;
		}
	}

	return p_current_terrain_chunk;
}

void tg_terrain_internal_find_connecting_chunks(tg_terrain_h terrain_h, const tg_terrain_chunk* p_terrain_chunk, tg_transvoxel_connecting_chunks* p_transvoxel_connecting_chunks)
{
	const i32 x = p_terrain_chunk->p_transvoxel_chunk->coordinates.x;
	const i32 y = p_terrain_chunk->p_transvoxel_chunk->coordinates.y;
	const i32 z = p_terrain_chunk->p_transvoxel_chunk->coordinates.z;

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
					p_transvoxel_connecting_chunks->pp_chunks[i] = p_connecting_terrain_chunk->p_transvoxel_chunk;
				}
				else
				{
					p_transvoxel_connecting_chunks->pp_chunks[i] = TG_NULL;
				}
			}
		}
	}
}

b32 tg_terrain_internal_store_chunk(tg_terrain_h terrain_h, tg_terrain_chunk* p_chunk)
{
	b32 result = TG_FALSE;

	if (terrain_h->chunk_count + 1 < TG_TERRAIN_HASHMAP_COUNT)
	{
		u32 hash_index = TG_TERRAIN_HASH_V3(p_chunk->p_transvoxel_chunk->coordinates);
		while (terrain_h->pp_chunks_hashmap[hash_index])
		{
			hash_index = tgm_u32_incmod(hash_index, TG_TERRAIN_HASHMAP_COUNT);
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
				const f32 n_hills = n_hills0 + 0.005f * n_hills1;

				const f32 s_caves = 0.06f;
				const f32 n_caves = tgm_f32_clamp(tgm_noise(s_caves * base_x, s_caves * base_y, s_caves * base_z), -1.0f, 0.0f);

				const f32 n = n_hills;
				f32 noise = (n * 64.0f) - ((f32)cy + (f32)y * 16.0f);
				noise += 60.0f * n_caves;
				const f32 noise_clamped = tgm_f32_clamp(noise, -1.0f, 1.0f);
				const f32 f0 = (noise_clamped + 1.0f) * 0.5f;
				const f32 f1 = 254.0f * f0;
				i8 f2 = -(i8)(tgm_f32_round_to_i32(f1) - 127);
				TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, cx, cy, cz) = f2;
			}
		}
	}
}

b32 tg_terrain_internal_is_in_view_radius_xz(v3i focal_point_chunk_coordinates, i32 x, i32 z)
{
	const v3i v0 = { focal_point_chunk_coordinates.x, 0, focal_point_chunk_coordinates.z };
	const v3i v1 = { x, 0, z };
	const v3i difference = tgm_v3i_sub(v0, v1);
	const i32 distance_sqr = tgm_v3i_magsqr(difference);
	const i32 view_radius_sqr = TG_TERRAIN_VIEW_DISTANCE_CHUNKS * TG_TERRAIN_VIEW_DISTANCE_CHUNKS;
	const b32 result = distance_sqr <= view_radius_sqr;
	return result;
}

b32 tg_terrain_internal_remove_chunk(tg_terrain_h terrain_h, i32 x, i32 y, i32 z)
{
	const u32 original_hash_index = TG_TERRAIN_HASH(x, y, z);

	u32 hash_index = original_hash_index;
	tg_terrain_chunk* p_terrain_chunk = terrain_h->pp_chunks_hashmap[hash_index];

	if (!p_terrain_chunk)
	{
		return TG_FALSE;
	}

	while (p_terrain_chunk->p_transvoxel_chunk->coordinates.x != x || p_terrain_chunk->p_transvoxel_chunk->coordinates.y != y || p_terrain_chunk->p_transvoxel_chunk->coordinates.z != z)
	{
		hash_index = tgm_u32_incmod(hash_index, TG_TERRAIN_HASHMAP_COUNT);
		p_terrain_chunk = terrain_h->pp_chunks_hashmap[hash_index];

		if (!p_terrain_chunk || hash_index == original_hash_index)
		{
			return TG_FALSE;
		}
	}

	terrain_h->chunk_count--;
	const b32 is_last_chunk = terrain_h->pp_chunks_hashmap[hash_index] == &terrain_h->p_memory_blocks[terrain_h->chunk_count];
	terrain_h->pp_chunks_hashmap[hash_index] = TG_NULL;

	if (terrain_h->chunk_count != 0)
	{
		if (!is_last_chunk)
		{
			const u32 original_last_hash_index = TG_TERRAIN_HASH_V3(terrain_h->p_memory_blocks[terrain_h->chunk_count].p_transvoxel_chunk->coordinates);
			u32 i = original_last_hash_index;
			tg_terrain_chunk* p_it = terrain_h->pp_chunks_hashmap[i];

			while (p_it != &terrain_h->p_memory_blocks[terrain_h->chunk_count])
			{
				i = tgm_u32_incmod(i, TG_TERRAIN_HASHMAP_COUNT);
				p_it = terrain_h->pp_chunks_hashmap[i];
				TG_ASSERT(i != original_last_hash_index);
			}

			const tg_terrain_chunk temp_terrain_chunk = *p_terrain_chunk;
			*p_terrain_chunk = terrain_h->p_memory_blocks[terrain_h->chunk_count];
			terrain_h->p_memory_blocks[terrain_h->chunk_count] = temp_terrain_chunk;
			terrain_h->pp_chunks_hashmap[i] = p_terrain_chunk;
		}

		u32 neighbour_count = 0;
		u32 neighbour_hash_index = tgm_u32_incmod(hash_index, TG_TERRAIN_HASHMAP_COUNT);
		while (terrain_h->pp_chunks_hashmap[neighbour_hash_index])
		{
			neighbour_count++;
			neighbour_hash_index = tgm_u32_incmod(neighbour_hash_index, TG_TERRAIN_HASHMAP_COUNT);
		}

		if (neighbour_count != 0)
		{
			const u64 size = neighbour_count * sizeof(tg_terrain_chunk**);
			tg_terrain_chunk** pp_neighbour_chunks = TG_MEMORY_STACK_ALLOC(size);

			neighbour_hash_index = tgm_u32_incmod(hash_index, TG_TERRAIN_HASHMAP_COUNT);
			for (u32 i = 0; i < neighbour_count; i++)
			{
				pp_neighbour_chunks[i] = terrain_h->pp_chunks_hashmap[neighbour_hash_index];
				terrain_h->pp_chunks_hashmap[neighbour_hash_index] = TG_NULL;
				neighbour_hash_index = tgm_u32_incmod(neighbour_hash_index, TG_TERRAIN_HASHMAP_COUNT);
			}

			terrain_h->chunk_count -= neighbour_count;
			for (u32 i = 0; i < neighbour_count; i++)
			{
				tg_terrain_internal_store_chunk(terrain_h, pp_neighbour_chunks[i]);
			}

			TG_MEMORY_STACK_FREE(size);
		}
	}

	return TG_TRUE;
}

u8 tg_terrain_internal_select_lod(v3 focal_point, i32 x, i32 y, i32 z)
{
	const v3 chunk_center = TG_TERRAIN_CHUNK_CENTER(x, y, z);
	const v3 delta = tgm_v3_sub((v3){ focal_point.x, focal_point.y, focal_point.z }, chunk_center);
	const f32 distance = tgm_v3_mag(delta);
	const u8 lod = (u8)tgm_u64_min((u64)(distance / (f32)(TG_TERRAIN_CHUNKS_PER_LOD * TG_TRANSVOXEL_CELLS)), TG_TERRAIN_MAX_LOD);
	return lod;
}

void tg_terrain_internal_generate_chunk(tg_terrain_h terrain_h, i32 x, i32 y, i32 z)
{
	tg_terrain_chunk* p_terrain_chunk = &terrain_h->p_memory_blocks[terrain_h->chunk_count];
	// TODO: for now! this needs to be removed and entities potentially initialized, once
	// multiple focal points should be supported!
	TG_ASSERT(p_terrain_chunk->entity.flags & TG_ENTITY_FLAG_INITIALIZED);

	const v3 focal_point = tg_camera_get_position(terrain_h->focal_point);

	if (!p_terrain_chunk->p_transvoxel_chunk) // TODO: untested, see above
	{
		p_terrain_chunk->p_transvoxel_chunk = TG_MEMORY_ALLOC(sizeof(*p_terrain_chunk->p_transvoxel_chunk));
	}
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
	if (!p_terrain_chunk->p_transvoxel_chunk->p_triangles) // TODO: untested, see above
	{
		p_terrain_chunk->p_transvoxel_chunk->p_triangles = TG_MEMORY_ALLOC(TG_TRANSVOXEL_CHUNK_MAX_TRIANGLES * sizeof(tg_transvoxel_triangle));
	}
	tg_terrain_internal_store_chunk(terrain_h, p_terrain_chunk);

	tg_transvoxel_fill_chunk(p_terrain_chunk->p_transvoxel_chunk);
	if (p_terrain_chunk->p_transvoxel_chunk->transition_faces_bitmap)
	{
		tg_transvoxel_connecting_chunks transvoxel_connecting_chunks = { 0 };
		tg_terrain_internal_find_connecting_chunks(terrain_h, p_terrain_chunk, &transvoxel_connecting_chunks);
		tg_transvoxel_fill_transitions(&transvoxel_connecting_chunks);
	}

	if (!(p_terrain_chunk->entity.flags & TG_ENTITY_FLAG_INITIALIZED)) // TODO: untested, see above
	{
		p_terrain_chunk->entity = tg_entity_create(TG_NULL, terrain_h->material_h);
	}

	if (p_terrain_chunk->p_transvoxel_chunk->triangle_count)
	{
		const u32 vertex_count = 3 * p_terrain_chunk->p_transvoxel_chunk->triangle_count;
		const tg_vertex* p_vertices = (tg_vertex*)p_terrain_chunk->p_transvoxel_chunk->p_triangles;
		if (p_terrain_chunk->entity.graphics_data_ptr_h->p_lod_meshes_h[0])
		{
			tg_mesh_update2(p_terrain_chunk->entity.graphics_data_ptr_h->p_lod_meshes_h[0], vertex_count, p_vertices, 0, TG_NULL);
			tg_entity_graphics_data_ptr_set_mesh(p_terrain_chunk->entity.graphics_data_ptr_h, p_terrain_chunk->entity.graphics_data_ptr_h->p_lod_meshes_h[0], 0);
		}
		else
		{
			TG_ASSERT(TG_FALSE);
		}
	}
}



tg_terrain_h tg_terrain_create(tg_camera_h focal_point_camera_h)
{
	tg_terrain_h terrain_h = TG_MEMORY_ALLOC(sizeof(*terrain_h));

	terrain_h->focal_point = focal_point_camera_h;
	terrain_h->last_focal_point_position = tg_camera_get_position(focal_point_camera_h);
	terrain_h->vertex_shader_h = tg_vertex_shader_get("shaders/deferred.vert");
	terrain_h->fragment_shader_h = tg_fragment_shader_get("shaders/deferred_terrain.frag");
	terrain_h->material_h = tg_material_create_deferred(terrain_h->vertex_shader_h, terrain_h->fragment_shader_h, 0, TG_NULL);

	tg_memory_nullify(TG_TERRAIN_MAX_CHUNK_COUNT * sizeof(*terrain_h->pp_chunks_hashmap), terrain_h->pp_chunks_hashmap);

	const v3 focal_point = terrain_h->last_focal_point_position;
	const v3i focal_point_chunk = tg_terrain_internal_convert_to_chunk_coordinates(focal_point);

	for (i32 relz = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; relz < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; relz++)
	{
		for (i32 y = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y; y < TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y + 1; y++)
		{
			for (i32 relx = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; relx < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; relx++)
			{
				const i32 x = focal_point_chunk.x + relx;
				const i32 z = focal_point_chunk.z + relz;

				if (!tg_terrain_internal_is_in_view_radius_xz(focal_point_chunk, x, z))
				{
					continue;
				}

				tg_terrain_chunk* p_terrain_chunk = &terrain_h->p_memory_blocks[terrain_h->chunk_count];

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
				// TODO: to reduce memory consumption, create only one of those for (re)creation of
				// chunks and let the chunks have smaller buffers, that can grow if need be.
				p_terrain_chunk->p_transvoxel_chunk->p_triangles = TG_MEMORY_ALLOC(TG_TRANSVOXEL_CHUNK_MAX_TRIANGLES * sizeof(tg_transvoxel_triangle));

				TG_ASSERT(tgm_i32_abs(p_terrain_chunk->p_transvoxel_chunk->lod - tg_terrain_internal_select_lod(focal_point, x - 1, y, z)) <= 1);
				TG_ASSERT(tgm_i32_abs(p_terrain_chunk->p_transvoxel_chunk->lod - tg_terrain_internal_select_lod(focal_point, x + 1, y, z)) <= 1);
				TG_ASSERT(tgm_i32_abs(p_terrain_chunk->p_transvoxel_chunk->lod - tg_terrain_internal_select_lod(focal_point, x, y - 1, z)) <= 1);
				TG_ASSERT(tgm_i32_abs(p_terrain_chunk->p_transvoxel_chunk->lod - tg_terrain_internal_select_lod(focal_point, x, y + 1, z)) <= 1);
				TG_ASSERT(tgm_i32_abs(p_terrain_chunk->p_transvoxel_chunk->lod - tg_terrain_internal_select_lod(focal_point, x, y, z - 1)) <= 1);
				TG_ASSERT(tgm_i32_abs(p_terrain_chunk->p_transvoxel_chunk->lod - tg_terrain_internal_select_lod(focal_point, x, y, z + 1)) <= 1);

				tg_transvoxel_fill_chunk(p_terrain_chunk->p_transvoxel_chunk);

				p_terrain_chunk->entity = tg_entity_create(TG_NULL, terrain_h->material_h);
				tg_terrain_internal_store_chunk(terrain_h, p_terrain_chunk);
			}
		}
	}

	tg_transvoxel_connecting_chunks transvoxel_connecting_chunks = { 0 };
	for (i32 relz = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; relz < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; relz++)
	{
		for (i32 y = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y; y < TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y + 1; y++)
		{
			for (i32 relx = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; relx < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; relx++)
			{
				const i32 x = focal_point_chunk.x + relx;
				const i32 z = focal_point_chunk.z + relz;
				tg_terrain_chunk* p_terrain_chunk = tg_terrain_internal_get_chunk(terrain_h, x, y, z);

				if (!p_terrain_chunk)
				{
					continue;
				}

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
				tg_mesh_h mesh_h = tg_mesh_create_empty(p_terrain_chunk->p_transvoxel_chunk->triangle_count, 0);
				if (p_terrain_chunk->p_transvoxel_chunk->triangle_count)
				{
					tg_mesh_update2(mesh_h, 3 * p_terrain_chunk->p_transvoxel_chunk->triangle_count, (tg_vertex*)p_terrain_chunk->p_transvoxel_chunk->p_triangles, 0, TG_NULL);
				}
				tg_entity_graphics_data_ptr_set_mesh(p_terrain_chunk->entity.graphics_data_ptr_h, mesh_h, 0);
			}
		}
	}

	return terrain_h;
}

void tg_terrain_update(tg_terrain_h terrain_h)
{
	TG_ASSERT(terrain_h);

	const v3 last_focal_point = terrain_h->last_focal_point_position;
	const v3i last_focal_point_chunk = tg_terrain_internal_convert_to_chunk_coordinates(last_focal_point);
	terrain_h->last_focal_point_position = tg_camera_get_position(terrain_h->focal_point);
	const v3 focal_point = terrain_h->last_focal_point_position;
	const v3i focal_point_chunk = tg_terrain_internal_convert_to_chunk_coordinates(focal_point);

	if (tgm_v3_equal(last_focal_point, focal_point))
	{
		return;
	}

	// TODO: if this becomes too slow, cut 45 degree rotated quad out of center, that will not be updated
	if (last_focal_point_chunk.x != focal_point_chunk.x || last_focal_point_chunk.z != focal_point_chunk.z)
	{
		for (i32 relz = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; relz < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; relz++)
		{
			const i32 z = last_focal_point_chunk.z + relz;
			for (i32 relx = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; relx < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; relx++)
			{
				const i32 x = last_focal_point_chunk.x + relx;
				if (!tg_terrain_internal_is_in_view_radius_xz(focal_point_chunk, x, z))
				{
					for (i32 y = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y; y < TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y + 1; y++)
					{
						tg_terrain_chunk* p_terrain_chunk = tg_terrain_internal_get_chunk(terrain_h, x, y, z);
						if (p_terrain_chunk)
						{
							tg_terrain_internal_remove_chunk(terrain_h, x, y, z);
						}
					}
				}
			}
		}
	}

	for (i32 relz = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; relz < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; relz++)
	{
		for (i32 y = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y; y < TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y + 1; y++)
		{
			for (i32 relx = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; relx < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; relx++)
			{
				const i32 x = focal_point_chunk.x + relx;
				const i32 z = focal_point_chunk.z + relz;
				tg_terrain_chunk* p_terrain_chunk = tg_terrain_internal_get_chunk(terrain_h, x, y, z);
	
				if (p_terrain_chunk)
				{
					const u8 lod = tg_terrain_internal_select_lod(focal_point, x, y, z);

					const u8 transition_faces_bitmap = lod == 0 ? 0 : TG_TRANSVOXEL_TRANSITION_FACES(
						lod > tg_terrain_internal_select_lod(focal_point, x - 1, y, z),
						lod > tg_terrain_internal_select_lod(focal_point, x + 1, y, z),
						lod > tg_terrain_internal_select_lod(focal_point, x, y - 1, z),
						lod > tg_terrain_internal_select_lod(focal_point, x, y + 1, z),
						lod > tg_terrain_internal_select_lod(focal_point, x, y, z - 1),
						lod > tg_terrain_internal_select_lod(focal_point, x, y, z + 1)
					);

					if (lod != p_terrain_chunk->p_transvoxel_chunk->lod || transition_faces_bitmap != p_terrain_chunk->p_transvoxel_chunk->transition_faces_bitmap)
					{
						p_terrain_chunk->flags |= TG_TERRAIN_FLAG_REGENERATE;
						p_terrain_chunk->p_transvoxel_chunk->lod = lod;
						p_terrain_chunk->p_transvoxel_chunk->transition_faces_bitmap = transition_faces_bitmap;
					}
				}
				else
				{
					if (tg_terrain_internal_is_in_view_radius_xz(focal_point_chunk, x, z))
					{
						tg_terrain_internal_generate_chunk(terrain_h, x, y, z);
					}
				}
			}
		}
	}

	for (i32 relz = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; relz < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; relz++)
	{
		for (i32 y = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y; y < TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y + 1; y++)
		{
			for (i32 relx = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; relx < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; relx++)
			{
				const i32 x = focal_point_chunk.x + relx;
				const i32 z = focal_point_chunk.z + relz;
				tg_terrain_chunk* p_terrain_chunk = tg_terrain_internal_get_chunk(terrain_h, x, y, z);

				if (p_terrain_chunk && (p_terrain_chunk->flags & TG_TERRAIN_FLAG_REGENERATE))
				{
					tg_transvoxel_fill_chunk(p_terrain_chunk->p_transvoxel_chunk);
				}
			}
		}
	}

	tg_transvoxel_connecting_chunks transvoxel_connecting_chunks = { 0 };
	for (i32 relz = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; relz < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; relz++)
	{
		for (i32 y = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y; y < TG_TERRAIN_VIEW_DISTANCE_CHUNKS_Y + 1; y++)
		{
			for (i32 relx = -TG_TERRAIN_VIEW_DISTANCE_CHUNKS; relx < TG_TERRAIN_VIEW_DISTANCE_CHUNKS + 1; relx++)
			{
				const i32 x = focal_point_chunk.x + relx;
				const i32 z = focal_point_chunk.z + relz;
				tg_terrain_chunk* p_terrain_chunk = tg_terrain_internal_get_chunk(terrain_h, x, y, z);

				if (p_terrain_chunk && (p_terrain_chunk->flags & TG_TERRAIN_FLAG_REGENERATE))
				{
					p_terrain_chunk->flags &= ~TG_TERRAIN_FLAG_REGENERATE;

					if (p_terrain_chunk->p_transvoxel_chunk->transition_faces_bitmap)
					{
						tg_terrain_internal_find_connecting_chunks(terrain_h, p_terrain_chunk, &transvoxel_connecting_chunks);
						tg_transvoxel_fill_transitions(&transvoxel_connecting_chunks);
					}
					if (p_terrain_chunk->p_transvoxel_chunk->triangle_count)
					{
						if (p_terrain_chunk->entity.graphics_data_ptr_h->p_lod_meshes_h[0])
						{
							tg_mesh_update2(
								p_terrain_chunk->entity.graphics_data_ptr_h->p_lod_meshes_h[0],
								3 * p_terrain_chunk->p_transvoxel_chunk->triangle_count,
								(tg_vertex*)p_terrain_chunk->p_transvoxel_chunk->p_triangles,
								0, TG_NULL
							);
							tg_entity_graphics_data_ptr_set_mesh(p_terrain_chunk->entity.graphics_data_ptr_h, p_terrain_chunk->entity.graphics_data_ptr_h->p_lod_meshes_h[0], 0);
						}
						else
						{
							TG_ASSERT(TG_FALSE);
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

	tg_terrain_chunk* p_terrain_chunk = &terrain_h->p_memory_blocks[0];
	while (p_terrain_chunk->entity.flags & TG_ENTITY_FLAG_INITIALIZED)
	{
		if (p_terrain_chunk->entity.graphics_data_ptr_h->p_lod_meshes_h[0])
		{
			tg_mesh_destroy(p_terrain_chunk->entity.graphics_data_ptr_h->p_lod_meshes_h[0]);
		}
		tg_entity_destroy(&p_terrain_chunk->entity);
		TG_MEMORY_FREE(p_terrain_chunk->p_transvoxel_chunk->p_triangles);
		TG_MEMORY_FREE(p_terrain_chunk->p_transvoxel_chunk);

		p_terrain_chunk++;
	}

	tg_material_destroy(terrain_h->material_h);

	TG_MEMORY_FREE(terrain_h);
}

#endif
