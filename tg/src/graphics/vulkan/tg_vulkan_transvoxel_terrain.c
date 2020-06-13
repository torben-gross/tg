#include "graphics/vulkan/tg_vulkan_transvoxel_terrain.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_transvoxel_lookup_tables.h"



#define TGI_DIRECTION_TO_PRECEDING_CELL(direction)    ((v3i){ -((direction) & 1), -(((direction) >> 1) & 1), -(((direction) >> 2) & 1) })
#define TGI_FIXED_FACTOR                              (1.0f / 256.0f)
#define TGI_INV_LOD(lod)                              (TG_TRANSVOXEL_LOD_COUNT - 1 - (lod))
#define TGI_REUSE_CELL_AT(reuse_cells, position)      ((reuse_cells).pp_decks[(position).z & 1][(position).y * TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING + (position).x])
#define TGI_TRANSITION_CELL_SCALE                     0.5f



typedef struct tgi_reuse_cell
{
	u16    p_indices[4];
} tgi_reuse_cell;

typedef struct tgi_reuse_cells
{
	tgi_reuse_cell    pp_decks[2][TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING * TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING * TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING];
} tgi_reuse_cells;



void* tgi_create_compressed_voxel_map(const i8* p_voxel_map)
{
	void* p_compressed_voxel_map = TG_NULL;

	u32 size = 4;
	
	i8 current_value = p_voxel_map[0];
	for (u16 z = 0; z < TG_TRANSVOXEL_OCTREE_STRIDE_IN_VOXELS_WITH_PADDING; z++)
	{
		for (u16 y = 0; y < TG_TRANSVOXEL_OCTREE_STRIDE_IN_VOXELS_WITH_PADDING; y++)
		{
			for (u16 x = 0; x < TG_TRANSVOXEL_OCTREE_STRIDE_IN_VOXELS_WITH_PADDING; x++)
			{
				if (TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, x, y, z) != current_value)
				{
					current_value = TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, x, y, z);
					size += 5;
				}
			}
		}
	}
	
	p_compressed_voxel_map = TG_MEMORY_ALLOC(size);

	*((u32*)p_compressed_voxel_map) = size;

	u32 counter = 0;
	size = 4;
	
	current_value = p_voxel_map[0];
	for (u16 z = 0; z < TG_TRANSVOXEL_OCTREE_STRIDE_IN_VOXELS_WITH_PADDING; z++)
	{
		for (u16 y = 0; y < TG_TRANSVOXEL_OCTREE_STRIDE_IN_VOXELS_WITH_PADDING; y++)
		{
			for (u16 x = 0; x < TG_TRANSVOXEL_OCTREE_STRIDE_IN_VOXELS_WITH_PADDING; x++)
			{
				if (TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, x, y, z) != current_value)
				{
					*((u32*)&((u8*)p_compressed_voxel_map)[size]) = counter;
					((i8*)p_compressed_voxel_map)[size + 4] = current_value;
					size += 5;

					counter = 0;
					current_value = TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, x, y, z);
				}
				counter++;
			}
		}
	}

	return p_compressed_voxel_map;
}

void tgi_decompress_compressed_voxel_map(const void* p_compressed_voxel_map, i8* p_voxel_map)
{
	const u32 size = *(u32*)p_compressed_voxel_map;

	u32 processed_bytes = 4;
	u32 i = 0;
	do
	{
		const u32 counter = *((u32*)&((u8*)p_compressed_voxel_map)[processed_bytes]);
		const i8 current_value = ((i8*)p_compressed_voxel_map)[processed_bytes + 4];

		const u32 end = i + counter;
		for (; i < end; i++)
		{
			p_voxel_map[i] = current_value;
		}

		processed_bytes += 5;
	} while (processed_bytes < size);
}

void tgi_destroy_compressed_voxel_map(void* p_compressed_voxel_map)
{
	TG_MEMORY_FREE(p_compressed_voxel_map);
}

void tgi_fill_voxel_map(v3i min_coordinates, i8* p_voxel_map)
{
	for (i32 z = 0; z < TG_TRANSVOXEL_OCTREE_STRIDE_IN_VOXELS_WITH_PADDING; z++)
	{
		for (i32 y = 0; y < TG_TRANSVOXEL_OCTREE_STRIDE_IN_VOXELS_WITH_PADDING; y++)
		{
			for (i32 x = 0; x < TG_TRANSVOXEL_OCTREE_STRIDE_IN_VOXELS_WITH_PADDING; x++)
			{
				const f32 cell_coordinate_x = (f32)(min_coordinates.x + (x - TG_TRANSVOXEL_PADDING_PER_SIDE));
				const f32 cell_coordinate_y = (f32)(min_coordinates.y + (y - TG_TRANSVOXEL_PADDING_PER_SIDE));
				const f32 cell_coordinate_z = (f32)(min_coordinates.z + (z - TG_TRANSVOXEL_PADDING_PER_SIDE));

				const f32 n_hills0 = tgm_noise(cell_coordinate_x * 0.008f, 0.0f, cell_coordinate_z * 0.008f);
				const f32 n_hills1 = tgm_noise(cell_coordinate_x * 0.2f, 0.0f, cell_coordinate_z * 0.2f);
				const f32 n_hills = n_hills0 + 0.005f * n_hills1;

				const f32 s_caves = 0.06f;
				const f32 c_caves_x = s_caves * cell_coordinate_x;
				const f32 c_caves_y = s_caves * cell_coordinate_y;
				const f32 c_caves_z = s_caves * cell_coordinate_z;
				const f32 unclamped_noise_caves = tgm_noise(c_caves_x, c_caves_y, c_caves_z);
				const f32 n_caves = tgm_f32_clamp(unclamped_noise_caves, -1.0f, 0.0f);

				const f32 n = n_hills;
				f32 noise = (n * 64.0f) - (f32)(y + min_coordinates.y * 16);
				noise += 60.0f * n_caves;
				const f32 noise_clamped = tgm_f32_clamp(noise, -1.0f, 1.0f);
				const f32 f0 = (noise_clamped + 1.0f) * 0.5f;
				const f32 f1 = 254.0f * f0;
				i8 f2 = -(i8)(tgm_f32_round_to_i32(f1) - 127);
				TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, x, y, z) = f2;
			}
		}
	}
}



u16 tgi_emit_vertex(v3 primary, v3 normal, u16 border_mask, v3 secondary, u16* p_vertex_count, tg_transvoxel_vertex* p_vertex_buffer, u16* p_index_count, u16* p_index_buffer)
{
	const u16 result = *p_vertex_count;

	p_vertex_buffer[*p_vertex_count].position = primary;
	p_vertex_buffer[*p_vertex_count].normal = normal;
	p_vertex_buffer[*p_vertex_count].extra = tgm_v3_to_v4(secondary, (f32)border_mask);
	(*p_vertex_count)++;

	return result;
}

u8 tgi_get_border_mask(v3i position, i32 block_size)
{
	u8 mask = 0;

	for (u8 i = 0; i < 3; i++) {
		if (position.p_data[i] == 0) {
			mask |= (1 << (i * 2));
		}
		if (position.p_data[i] == block_size) {
			mask |= (1 << (i * 2 + 1));
		}
	}

	return mask;
}

v3 tgi_get_border_offset(v3 position, u8 lod, i32 block_size)
{
	v3 delta = { 0 };

	const f32 p2k = (f32)(1 << lod);
	const f32 p2mk = 1.0f / p2k;

	const f32 wk = TGI_TRANSITION_CELL_SCALE * p2k;

	for (u8 i = 0; i < 3; ++i)
	{
		const f32 p = position.p_data[i];
		const f32 s = (f32)block_size;

		if (p < p2k)
		{
			delta.p_data[i] = (1.0f - p2mk * p) * wk;
		}
		else if (p > (p2k * (s - 1)))
		{
			delta.p_data[i] = ((p2k * s) - 1.0f - p) * wk;
		}
	}

	return delta;
}

v3 tgi_project_border_offset(v3 delta, v3 normal)
{
	const v3 result = {
		(1.0f - normal.x * normal.x) * delta.x         - normal.y * normal.x  * delta.y         - normal.z * normal.x  * delta.z,
		      - normal.x * normal.y  * delta.x + (1.0f - normal.y * normal.y) * delta.y         - normal.z * normal.y  * delta.z,
		      - normal.x * normal.z  * delta.x         - normal.y * normal.z  * delta.y + (1.0f - normal.z * normal.z) * delta.z
	};
	return result;
}

v3 tgi_get_secondary_position(v3 primary, v3 normal, u8 lod, i32 block_size)
{
	v3 delta = tgi_get_border_offset(primary, lod, block_size);
	delta = tgi_project_border_offset(delta, normal);
	const v3 result = tgm_v3_add(primary, delta);
	return result;
}

v3 tgi_normalized_not_null(v3 v)
{
	const f32 magsqr = tgm_v3_magsqr(v);
	v3 result = { 0.0f, 1.0f, 0.0f };
	if (magsqr != 0.0f)
	{
		const f32 mag = tgm_f32_sqrt(magsqr);
		result = (v3){ v.x / mag, v.y / mag, v.z / mag };
	}
	return result;
}

void tgi_build_block(v3i min_coordinates, const i8* p_voxel_map, u8 lod, u16* p_vertex_count, tg_transvoxel_vertex* p_vertex_buffer, u16* p_index_count, u16* p_index_buffer)
{
	*p_vertex_count = 0;
	*p_index_count = 0;

	tgi_reuse_cells* p_reuse_cells = TG_MEMORY_STACK_ALLOC(sizeof(*p_reuse_cells));
	tg_memory_set_all_bits(sizeof(*p_reuse_cells), p_reuse_cells);

	const i32 cells_per_block_side_scaled = TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE << lod;
	const i32 max_position = TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING - TG_TRANSVOXEL_PADDING_PER_SIDE;
	const i32 lod_scale = 1 << lod;

	v3i p_corner_positions[8] = { 0 };
	i8 p_cell_samples[8] = { 0 };
	v3 p_corner_gradients[8] = { 0 };

	v3i position = { 0 };
	for (position.z = TG_TRANSVOXEL_PADDING_PER_SIDE; position.z < max_position; position.z++)
	{
		for (position.y = TG_TRANSVOXEL_PADDING_PER_SIDE; position.y < max_position; position.y++)
		{
			for (position.x = TG_TRANSVOXEL_PADDING_PER_SIDE; position.x < max_position; position.x++)
			{
				p_corner_positions[0] = (v3i){ position.x    , position.y    , position.z     };
				p_corner_positions[1] = (v3i){ position.x + 1, position.y    , position.z     };
				p_corner_positions[2] = (v3i){ position.x    , position.y + 1, position.z     };
				p_corner_positions[3] = (v3i){ position.x + 1, position.y + 1, position.z     };
				p_corner_positions[4] = (v3i){ position.x    , position.y    , position.z + 1 };
				p_corner_positions[5] = (v3i){ position.x + 1, position.y    , position.z + 1 };
				p_corner_positions[6] = (v3i){ position.x    , position.y + 1, position.z + 1 };
				p_corner_positions[7] = (v3i){ position.x + 1, position.y + 1, position.z + 1 };

				p_cell_samples[0] = TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, min_coordinates.x + lod_scale * position.x            , min_coordinates.y + lod_scale * position.y            , min_coordinates.z + lod_scale * position.z            );
				p_cell_samples[1] = TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, min_coordinates.x + lod_scale * position.x + lod_scale, min_coordinates.y + lod_scale * position.y            , min_coordinates.z + lod_scale * position.z            );
				p_cell_samples[2] = TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, min_coordinates.x + lod_scale * position.x            , min_coordinates.y + lod_scale * position.y + lod_scale, min_coordinates.z + lod_scale * position.z            );
				p_cell_samples[3] = TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, min_coordinates.x + lod_scale * position.x + lod_scale, min_coordinates.y + lod_scale * position.y + lod_scale, min_coordinates.z + lod_scale * position.z            );
				p_cell_samples[4] = TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, min_coordinates.x + lod_scale * position.x            , min_coordinates.y + lod_scale * position.y            , min_coordinates.z + lod_scale * position.z + lod_scale);
				p_cell_samples[5] = TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, min_coordinates.x + lod_scale * position.x + lod_scale, min_coordinates.y + lod_scale * position.y            , min_coordinates.z + lod_scale * position.z + lod_scale);
				p_cell_samples[6] = TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, min_coordinates.x + lod_scale * position.x            , min_coordinates.y + lod_scale * position.y + lod_scale, min_coordinates.z + lod_scale * position.z + lod_scale);
				p_cell_samples[7] = TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, min_coordinates.x + lod_scale * position.x + lod_scale, min_coordinates.y + lod_scale * position.y + lod_scale, min_coordinates.z + lod_scale * position.z + lod_scale);

				const u8 case_code =
					((p_cell_samples[0] >> 7) & 0x01) |
					((p_cell_samples[1] >> 6) & 0x02) |
					((p_cell_samples[2] >> 5) & 0x04) |
					((p_cell_samples[3] >> 4) & 0x08) |
					((p_cell_samples[4] >> 3) & 0x10) |
					((p_cell_samples[5] >> 2) & 0x20) |
					((p_cell_samples[6] >> 1) & 0x40) |
					((p_cell_samples[7] >> 0) & 0x80);

				TG_ASSERT(case_code != 256);

				tgi_reuse_cell* p_reuse_cell = &TGI_REUSE_CELL_AT(*p_reuse_cells, position);
				p_reuse_cell->p_indices[0] = TG_U16_MAX;

				if (case_code == 0 || case_code == 255)
				{
					continue;
				}

				for (u32 i = 0; i < 8; i++) {

					const v3i p = p_corner_positions[i];

					const f32 nx = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, min_coordinates.x + lod_scale * p.x - lod_scale, min_coordinates.y + lod_scale * p.y            , min_coordinates.z + lod_scale * p.z            ));
					const f32 ny = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, min_coordinates.x + lod_scale * p.x            , min_coordinates.y + lod_scale * p.y - lod_scale, min_coordinates.z + lod_scale * p.z            ));
					const f32 nz = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, min_coordinates.x + lod_scale * p.x            , min_coordinates.y + lod_scale * p.y            , min_coordinates.z + lod_scale * p.z - lod_scale));
					const f32 px = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, min_coordinates.x + lod_scale * p.x + lod_scale, min_coordinates.y + lod_scale * p.y            , min_coordinates.z + lod_scale * p.z            ));
					const f32 py = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, min_coordinates.x + lod_scale * p.x            , min_coordinates.y + lod_scale * p.y + lod_scale, min_coordinates.z + lod_scale * p.z            ));
					const f32 pz = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, min_coordinates.x + lod_scale * p.x            , min_coordinates.y + lod_scale * p.y            , min_coordinates.z + lod_scale * p.z + lod_scale));

					p_corner_gradients[i].x = nx - px;
					p_corner_gradients[i].y = ny - py;
					p_corner_gradients[i].z = nz - pz;

					p_corner_positions[i].x = (p_corner_positions[i].x - TG_TRANSVOXEL_PADDING_PER_SIDE) << lod;
					p_corner_positions[i].y = (p_corner_positions[i].y - TG_TRANSVOXEL_PADDING_PER_SIDE) << lod;
					p_corner_positions[i].z = (p_corner_positions[i].z - TG_TRANSVOXEL_PADDING_PER_SIDE) << lod;
				}

				const u8 direction_validity_mask =
					((position.x > TG_TRANSVOXEL_PADDING_PER_SIDE ? 1 : 0) << 0) |
					((position.y > TG_TRANSVOXEL_PADDING_PER_SIDE ? 1 : 0) << 1) |
					((position.z > TG_TRANSVOXEL_PADDING_PER_SIDE ? 1 : 0) << 2);

				const u8 regular_cell_class_index = p_regular_cell_class[case_code];
				const tg_regular_cell_data* p_cell_data = &p_regular_cell_data[regular_cell_class_index];
				const u8 triangle_count = TG_TRANSVOXEL_CELL_GET_TRIANGLE_COUNT(*p_cell_data);
				const u8 vertex_count = TG_TRANSVOXEL_CELL_GET_VERTEX_COUNT(*p_cell_data);

				u16 p_cell_vertex_indices[12] = { TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX };

				const u8 cell_border_mask = tgi_get_border_mask(tgm_v3i_subi(position, TG_TRANSVOXEL_PADDING_PER_SIDE), TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE);

				for (u8 i = 0; i < vertex_count; i++)
				{
					const u16 regular_vertex_data = p_regular_vertex_data[case_code][i];
					const u8 edge_code_low = regular_vertex_data & 0xff;
					const u8 edge_code_high = (regular_vertex_data >> 8) & 0xff;

					const u8 v0 = (edge_code_low >> 4) & 0xf;
					const u8 v1 = (edge_code_low >> 0) & 0xf;
					TG_ASSERT(v0 < v1);

					const i32 sample0 = p_cell_samples[v0];
					const i32 sample1 = p_cell_samples[v1];
					TG_ASSERT(sample0 != sample1);

					const i32 t = (sample1 << 8) / (sample1 - sample0);

					const f32 t0 = (f32)t / 256.0f;
					const f32 t1 = (f32)(0x100 - t) / 256.0f;
					const i32 ti0 = t;
					const i32 ti1 = 0x100 - t;

					const v3i p0 = tgm_v3i_add(p_corner_positions[v0], min_coordinates);
					const v3i p1 = tgm_v3i_add(p_corner_positions[v1], min_coordinates);
					if (t & 0xff)
					{
						const u8 reuse_direction = (edge_code_high >> 4) & 0xf;
						const u8 reuse_vertex_index = edge_code_high & 0xf;

						const b32 present = (reuse_direction & direction_validity_mask) == reuse_direction;

						if (present)
						{
							const v3i cache_position = tgm_v3i_add(position, TGI_DIRECTION_TO_PRECEDING_CELL(reuse_direction));
							const tgi_reuse_cell* p_preceding_cell = &TGI_REUSE_CELL_AT(*p_reuse_cells, cache_position);
							p_cell_vertex_indices[i] = p_preceding_cell->p_indices[reuse_vertex_index];
						}

						if (!present || p_cell_vertex_indices[i] == TG_U16_MAX)
						{
							// TODO: surface shifting interpolation

							const v3i primary = tgm_v3i_add(tgm_v3i_muli(p0, ti0), tgm_v3i_muli(p1, ti1));
							const v3 primaryf = tgm_v3_mulf(tgm_v3i_to_v3(primary), TGI_FIXED_FACTOR);
							const v3 normal = tgi_normalized_not_null(tgm_v3_add(tgm_v3_mulf(p_corner_gradients[v0], t0), tgm_v3_mulf(p_corner_gradients[v1], t1)));

							v3 secondary = { 0 };
							u16 border_mask = cell_border_mask;

							if (cell_border_mask > 0)
							{
								secondary = tgi_get_secondary_position(primaryf, normal, 0, cells_per_block_side_scaled);
								border_mask |= (tgi_get_border_mask(p0, cells_per_block_side_scaled) &
									tgi_get_border_mask(p1, cells_per_block_side_scaled)) << 6;
							}

							p_cell_vertex_indices[i] = tgi_emit_vertex(primaryf, normal, border_mask, secondary, p_vertex_count, p_vertex_buffer, p_index_count, p_index_buffer);

							if (reuse_direction & 8)
							{
								p_reuse_cell->p_indices[reuse_vertex_index] = p_cell_vertex_indices[i];
							}
						}
					}
					else if (t == 0 && v1 == 7)
					{
						const v3i primary = p1;
						const v3 primaryf = tgm_v3i_to_v3(primary);
						const v3 normal = tgi_normalized_not_null(p_corner_gradients[v1]);

						v3 secondary = { 0 };
						u16 border_mask = cell_border_mask;

						if (cell_border_mask > 0)
						{
							secondary = tgi_get_secondary_position(primaryf, normal, 0, cells_per_block_side_scaled);
							border_mask |= tgi_get_border_mask(p1, cells_per_block_side_scaled) << 6;
						}

						p_cell_vertex_indices[i] = tgi_emit_vertex(primaryf, normal, border_mask, secondary, p_vertex_count, p_vertex_buffer, p_index_count, p_index_buffer);

						p_reuse_cell->p_indices[0] = p_cell_vertex_indices[i];
					}
					else
					{
						const u8 reuse_direction = (t == 0 ? v1 ^ 7 : v0 ^ 7);
						const b32 present = (reuse_direction & direction_validity_mask) == reuse_direction;

						if (present)
						{
							const v3i cache_position = tgm_v3i_add(position, TGI_DIRECTION_TO_PRECEDING_CELL(reuse_direction));
							const tgi_reuse_cell* p_preceding_cell = &TGI_REUSE_CELL_AT(*p_reuse_cells, cache_position);
							p_cell_vertex_indices[i] = p_preceding_cell->p_indices[0];
						}

						if (!present || p_cell_vertex_indices[i] == TG_U16_MAX)
						{
							const v3i primary = t == 0 ? p1 : p0;
							const v3 primaryf = tgm_v3i_to_v3(primary);
							const v3 normal = tgi_normalized_not_null(p_corner_gradients[t == 0 ? v1 : v0]);

							v3 secondary = { 0 };
							u16 border_mask = cell_border_mask;

							if (cell_border_mask > 0)
							{
								secondary = tgi_get_secondary_position(primaryf, normal, 0, cells_per_block_side_scaled);
								border_mask |= tgi_get_border_mask(primary, cells_per_block_side_scaled) >> 6;
							}

							p_cell_vertex_indices[i] = tgi_emit_vertex(primaryf, normal, border_mask, secondary, p_vertex_count, p_vertex_buffer, p_index_count, p_index_buffer);
						}
					}
				}

				for (u8 t = 0; t < triangle_count; t++)
				{
					for (u8 i = 0; i < 3; i++)
					{
						const u16 index = p_cell_vertex_indices[p_cell_data->p_vertex_indices[3 * t + i]];
						p_index_buffer[(*p_index_count)++] = index;
					}
				}
			}
		}
	}
}

b32 tgi_should_split(v3 camera_position, v3i min_coordinates, u8 lod)
{
	if (lod == 0)
	{
		return TG_FALSE;
	}
	const i32 half_stride = TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE * (1 << (lod - 1)); // (1 << (lod - 1) => 2^lod / 2
	const v3 center = tgm_v3i_to_v3(tgm_v3i_add(min_coordinates, V3I(half_stride)));
	const v3 direction = tgm_v3_sub(center, camera_position);
	const f32 distance = tgm_v3_mag(direction);
	const f32 scaled_distance = distance / (1 << lod);
	const b32 result = scaled_distance < 2 * TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE;
	return result;
}

void tgi_build_node(tg_transvoxel_node* p_node, v3i min_coordinates, u8 lod, v3 camera_position, const i8* p_voxel_map)
{
	const u64 vertex_buffer_size = 15 * TG_TRANSVOXEL_CELLS_PER_BLOCK * sizeof(tg_transvoxel_vertex);
	const u64 index_buffer_size = 15 * TG_TRANSVOXEL_CELLS_PER_BLOCK * sizeof(u16);

	tg_transvoxel_vertex* p_vertex_buffer = TG_MEMORY_STACK_ALLOC(vertex_buffer_size);
	u16* p_index_buffer = TG_MEMORY_STACK_ALLOC(index_buffer_size);

	const b32 split = tgi_should_split(camera_position, min_coordinates, lod);
	if (split)
	{
		const i32 half_stride = TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE * (1 << (lod - 1)); // (1 << (lod - 1) => 2^lod / 2
		const v3i half_stride_vec = V3I(half_stride);
		for (u8 z = 0; z < 2; z++)
		{
			for (u8 y = 0; y < 2; y++)
			{
				for (u8 x = 0; x < 2; x++)
				{
					const u8 i = 4 * z + 2 * y + x;
					p_node->pp_children[i] = TG_MEMORY_ALLOC(sizeof(tg_transvoxel_node));
					tg_memory_nullify(sizeof(*p_node->pp_children[i]), p_node->pp_children[i]);
					const v3i offset = tgm_v3i_mul((v3i){ x, y, z }, half_stride_vec);
					const v3i child_min_coordinates = tgm_v3i_add(min_coordinates, offset);
					tgi_build_node(p_node->pp_children[i], child_min_coordinates, lod - 1, camera_position, p_voxel_map);
				}
			}
		}
	}
	else
	{
		tgi_build_block(
			min_coordinates,
			p_voxel_map, lod,
			&p_node->node.vertex_count, p_vertex_buffer,
			&p_node->node.index_count, p_index_buffer
		);

		if (p_node->node.index_count)
		{
			const u64 vertices_size = p_node->node.vertex_count * sizeof(*p_node->node.p_vertices);
			const u64 indices_size = p_node->node.index_count * sizeof(*p_node->node.p_indices);
			p_node->node.p_vertices = TG_MEMORY_ALLOC(vertices_size);
			p_node->node.p_indices = TG_MEMORY_ALLOC(indices_size);
			tg_memory_copy(vertices_size, p_vertex_buffer, p_node->node.p_vertices);
			tg_memory_copy(indices_size, p_index_buffer, p_node->node.p_indices);

			// TODO: save these somehow for destruction
			tg_mesh_h mesh_h = tg_mesh_create2(
				p_node->node.vertex_count, sizeof(tg_transvoxel_vertex), p_node->node.p_vertices,
				p_node->node.index_count, p_node->node.p_indices
			);
			tg_material_h material_h = tg_material_create_deferred(
				tg_vertex_shader_get("shaders/deferred_flat_terrain.vert"),
				tg_fragment_shader_get("shaders/deferred_flat_terrain.frag"),
				0, TG_NULL
			);
			p_node->node.entity = tg_entity_create(mesh_h, material_h);
		}
	}

	TG_MEMORY_STACK_FREE(index_buffer_size);
	TG_MEMORY_STACK_FREE(vertex_buffer_size);
}



tg_transvoxel_terrain_h tg_transvoxel_terrain_create(tg_camera_h camera_h)
{
	tg_transvoxel_terrain_h terrain_h = TG_MEMORY_ALLOC(sizeof(*terrain_h));
	terrain_h->camera_h = camera_h;
	terrain_h->octree.min_coordinates = (v3i){ 0, 0, 0 };

	tgi_should_split(tg_camera_get_position(camera_h), terrain_h->octree.min_coordinates, 0);

	const u64 voxel_map_size = TG_TRANSVOXEL_VOXEL_MAP_VOXELS * sizeof(i8);

	i8* p_voxel_map = TG_MEMORY_STACK_ALLOC(voxel_map_size);

	tgi_fill_voxel_map(terrain_h->octree.min_coordinates, p_voxel_map);
	terrain_h->octree.p_compressed_voxel_map = tgi_create_compressed_voxel_map(p_voxel_map);
	tgi_build_node(
		&terrain_h->octree.root,
		terrain_h->octree.min_coordinates,
		TG_TRANSVOXEL_LOD_COUNT - 1,
		tg_camera_get_position(camera_h),
		p_voxel_map
	);

	TG_MEMORY_STACK_FREE(voxel_map_size);

	return terrain_h;
}

void tg_transvoxel_terrain_destroy(tg_transvoxel_terrain_h terrain_h)
{
	TG_ASSERT(terrain_h);
	TG_ASSERT(TG_FALSE);
}

void tg_transvoxel_terrain_update(tg_transvoxel_terrain_h terrain_h)
{

}



#undef TGI_TRANSITION_CELL_SCALE
#undef TGI_REUSE_CELL_AT
#undef TGI_FIXED_FACTOR
#undef TGI_DIRECTION_TO_PRECEDING_CELL

#endif
