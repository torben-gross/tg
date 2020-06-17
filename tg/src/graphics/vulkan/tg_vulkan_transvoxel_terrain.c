#include "graphics/vulkan/tg_vulkan_transvoxel_terrain.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_transvoxel_lookup_tables.h"



#define TGI_DIRECTION_TO_PRECEDING_CELL(direction)         ((v3i){ -((direction) & 1), -(((direction) >> 1) & 1), -(((direction) >> 2) & 1) })
#define TGI_FIXED_FACTOR                                   (1.0f / 256.0f)
#define TGI_REUSE_CELL_AT(reuse_cells, position)           ((reuse_cells).pp_decks[(position).z & 1][(position).y * TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING + (position).x])
#define TGI_REUSE_TRANSITION_CELL_AT(reuse_cells, x, y)    ((reuse_cells).pp_decks[(y) & 1][x])
#define TGI_TRANSITION_CELL_SCALE                          0.5f



typedef enum tgi_cube_side
{
	TGI_CUBE_SIDE_NEGATIVE_X    = 0,
	TGI_CUBE_SIDE_POSITIVE_X    = 1,
	TGI_CUBE_SIDE_NEGATIVE_Y    = 2,
	TGI_CUBE_SIDE_POSITIVE_Y    = 3,
	TGI_CUBE_SIDE_NEGATIVE_Z    = 4,
	TGI_CUBE_SIDE_POSITIVE_Z    = 5
} tgi_cube_side;

typedef enum tgi_axis
{
	TGI_AXIS_X    = 0,
	TGI_AXIS_Y    = 1,
	TGI_AXIS_Z    = 2
} tgi_axis;



typedef struct tgi_reuse_cell
{
	u16    p_indices[4];
} tgi_reuse_cell;

typedef struct tgi_reuse_cells
{
	tgi_reuse_cell    pp_decks[2][TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING * TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING * TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING];
} tgi_reuse_cells;

typedef struct tgi_reuse_transition_cell
{
	u16    p_indices[12];
} tgi_reuse_transition_cell;

typedef struct tgi_reuse_transition_cells
{
	tgi_reuse_transition_cell    pp_decks[2][TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING];
} tgi_reuse_transition_cells;





void* tgi_create_compressed_voxel_map(const i8* p_voxel_map)
{
	void* p_compressed_voxel_map = TG_NULL;

	u32 size = 4;
	
	i8 current_value = p_voxel_map[0];
	for (u16 z = 0; z < TG_TRANSVOXEL_VOXEL_MAP_STRIDE_WITH_PADDING; z++)
	{
		for (u16 y = 0; y < TG_TRANSVOXEL_VOXEL_MAP_STRIDE_WITH_PADDING; y++)
		{
			for (u16 x = 0; x < TG_TRANSVOXEL_VOXEL_MAP_STRIDE_WITH_PADDING; x++)
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
	for (u16 z = 0; z < TG_TRANSVOXEL_VOXEL_MAP_STRIDE_WITH_PADDING; z++)
	{
		for (u16 y = 0; y < TG_TRANSVOXEL_VOXEL_MAP_STRIDE_WITH_PADDING; y++)
		{
			for (u16 x = 0; x < TG_TRANSVOXEL_VOXEL_MAP_STRIDE_WITH_PADDING; x++)
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

void tgi_fill_voxel_map(v3i octree_min_coordinates, i8* p_voxel_map)
{
	for (i32 z = 0; z < TG_TRANSVOXEL_VOXEL_MAP_STRIDE_WITH_PADDING; z++)
	{
		for (i32 y = 0; y < TG_TRANSVOXEL_VOXEL_MAP_STRIDE_WITH_PADDING; y++)
		{
			for (i32 x = 0; x < TG_TRANSVOXEL_VOXEL_MAP_STRIDE_WITH_PADDING; x++)
			{
				const f32 cell_coordinate_x = (f32)(octree_min_coordinates.x + (x - TG_TRANSVOXEL_VOXEL_MAP_PADDING));
				const f32 cell_coordinate_y = (f32)(octree_min_coordinates.y + (y - TG_TRANSVOXEL_VOXEL_MAP_PADDING));
				const f32 cell_coordinate_z = (f32)(octree_min_coordinates.z + (z - TG_TRANSVOXEL_VOXEL_MAP_PADDING));

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
				f32 noise = (n * 64.0f) - (f32)((y - TG_TRANSVOXEL_VOXEL_MAP_PADDING) + octree_min_coordinates.y);
				noise += 10.0f * n_caves;
				const f32 noise_clamped = tgm_f32_clamp(noise, -1.0f, 1.0f);
				const f32 f0 = (noise_clamped + 1.0f) * 0.5f;
				const f32 f1 = 254.0f * f0;
				i8 f2 = -(i8)(tgm_f32_round_to_i32(f1) - 127);
				TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, x, y, z) = f2;
			}
		}
	}
}





u8 tgi_determine_max_lod()
{
	u8 max_lod = 0;
	u8 cell_count = TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE;
	while (cell_count != 1)
	{
		max_lod++;
		cell_count >>= 1;
	}
	return max_lod;
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

b32 tgi_should_split_node(v3 camera_position, v3i octree_min_coordinates, v3i block_offset_in_octree, u8 lod)
{
	if (lod == 0)
	{
		return TG_FALSE;
	}
	const i32 half_stride = TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE * (1 << (lod - 1)); // (1 << (lod - 1) => 2^lod / 2
	const v3 center = tgm_v3i_to_v3(tgm_v3i_add(tgm_v3i_add(octree_min_coordinates, block_offset_in_octree), V3I(half_stride)));
	const v3 direction = tgm_v3_sub(center, camera_position);
	const f32 distance = tgm_v3_mag(direction);
	const f32 scaled_distance = distance / (1 << lod);
	const b32 result = scaled_distance < 2 * TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE;
	return result;
}

v3i tgi_transition_face_to_block(v3i pad, u8 low_resolution_lod, i32 x, i32 y, u8 direction)
{
	const u8 high_res_scale = 1 << (low_resolution_lod - 1);

	const i32 x0 = pad.x + high_res_scale * x;
	const i32 y0 = pad.y + high_res_scale * y;
	const i32 z0 = pad.z + 0;
	const i32 z1 = pad.z + high_res_scale * (TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE - 1);

	switch (direction)
	{
	case TGI_CUBE_SIDE_NEGATIVE_X: return (v3i){ z0, x0, y0 };
	case TGI_CUBE_SIDE_POSITIVE_X: return (v3i){ z1, y0, x0 };
	case TGI_CUBE_SIDE_NEGATIVE_Y: return (v3i){ y0, z0, x0 };
	case TGI_CUBE_SIDE_POSITIVE_Y: return (v3i){ x0, z1, y0 };
	case TGI_CUBE_SIDE_NEGATIVE_Z: return (v3i){ x0, y0, z0 };
	case TGI_CUBE_SIDE_POSITIVE_Z: return (v3i){ y0, x0, z1 };
	default: TG_INVALID_CODEPATH(); return (v3i){ 0 };
	}
}



void tgi_build_block(v3i octree_min_coordinates, v3i block_offset_in_octree, u8 lod, const i8* p_voxel_map, u16* p_vertex_count, tg_transvoxel_vertex* p_vertex_buffer, u16* p_index_count, u16* p_index_buffer)
{
	tgi_reuse_cells* p_reuse_cells = TG_MEMORY_STACK_ALLOC(sizeof(*p_reuse_cells));
	tg_memory_set_all_bits(sizeof(*p_reuse_cells), p_reuse_cells);

	const i32 scaled_block_stride_in_cells = TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE << lod;
	const i32 lod_scale = 1 << lod;

	v3i p_sample_positions[8] = { 0 };
	i8 p_cell_samples[8] = { 0 };
	v3 p_corner_gradients[8] = { 0 };
	v3i p_corner_positions[8] = { 0 };

	v3i position = { 0 };
	for (position.z = 0; position.z < TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE; position.z++)
	{
		for (position.y = 0; position.y < TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE; position.y++)
		{
			for (position.x = 0; position.x < TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE; position.x++)
			{
				const v3i position_with_padding = tgm_v3i_addi(position, TG_TRANSVOXEL_PADDING_PER_SIDE);

				const v3i sample_base_position = tgm_v3i_add(tgm_v3i_addi(block_offset_in_octree, TG_TRANSVOXEL_VOXEL_MAP_PADDING), tgm_v3i_muli(position, lod_scale));
				p_sample_positions[0] = tgm_v3i_add(sample_base_position, (v3i){         0,         0,         0 });
				p_sample_positions[1] = tgm_v3i_add(sample_base_position, (v3i){ lod_scale,         0,         0 });
				p_sample_positions[2] = tgm_v3i_add(sample_base_position, (v3i){         0, lod_scale,         0 });
				p_sample_positions[3] = tgm_v3i_add(sample_base_position, (v3i){ lod_scale, lod_scale,         0 });
				p_sample_positions[4] = tgm_v3i_add(sample_base_position, (v3i){         0,         0, lod_scale });
				p_sample_positions[5] = tgm_v3i_add(sample_base_position, (v3i){ lod_scale,         0, lod_scale });
				p_sample_positions[6] = tgm_v3i_add(sample_base_position, (v3i){         0, lod_scale, lod_scale });
				p_sample_positions[7] = tgm_v3i_add(sample_base_position, (v3i){ lod_scale, lod_scale, lod_scale });

				p_cell_samples[0] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0]);
				p_cell_samples[1] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[1]);
				p_cell_samples[2] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[2]);
				p_cell_samples[3] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[3]);
				p_cell_samples[4] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[4]);
				p_cell_samples[5] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[5]);
				p_cell_samples[6] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[6]);
				p_cell_samples[7] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[7]);

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

				tgi_reuse_cell* p_reuse_cell = &TGI_REUSE_CELL_AT(*p_reuse_cells, position_with_padding);
				p_reuse_cell->p_indices[0] = TG_U16_MAX;

				if (case_code == 0 || case_code == 255)
				{
					continue;
				}

				for (u32 i = 0; i < 8; i++)
				{
					const v3i p = p_sample_positions[i];

					const f32 nx = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, p.x - lod_scale, p.y            , p.z            ));
					const f32 ny = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, p.x            , p.y - lod_scale, p.z            ));
					const f32 nz = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, p.x            , p.y            , p.z - lod_scale));
					const f32 px = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, p.x + lod_scale, p.y            , p.z            ));
					const f32 py = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, p.x            , p.y + lod_scale, p.z            ));
					const f32 pz = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, p.x            , p.y            , p.z + lod_scale));

					p_corner_gradients[i].x = nx - px;
					p_corner_gradients[i].y = ny - py;
					p_corner_gradients[i].z = nz - pz;
				}

				// TODO: too many unused adds (block_min_coordinates)
				const v3i block_min_coordinates = tgm_v3i_add(octree_min_coordinates, block_offset_in_octree);
				p_corner_positions[0] = tgm_v3i_add(block_min_coordinates, tgm_v3i_muli(tgm_v3i_add(position, (v3i) { 0, 0, 0 }), lod_scale));
				p_corner_positions[1] = tgm_v3i_add(block_min_coordinates, tgm_v3i_muli(tgm_v3i_add(position, (v3i) { 1, 0, 0 }), lod_scale));
				p_corner_positions[2] = tgm_v3i_add(block_min_coordinates, tgm_v3i_muli(tgm_v3i_add(position, (v3i) { 0, 1, 0 }), lod_scale));
				p_corner_positions[3] = tgm_v3i_add(block_min_coordinates, tgm_v3i_muli(tgm_v3i_add(position, (v3i) { 1, 1, 0 }), lod_scale));
				p_corner_positions[4] = tgm_v3i_add(block_min_coordinates, tgm_v3i_muli(tgm_v3i_add(position, (v3i) { 0, 0, 1 }), lod_scale));
				p_corner_positions[5] = tgm_v3i_add(block_min_coordinates, tgm_v3i_muli(tgm_v3i_add(position, (v3i) { 1, 0, 1 }), lod_scale));
				p_corner_positions[6] = tgm_v3i_add(block_min_coordinates, tgm_v3i_muli(tgm_v3i_add(position, (v3i) { 0, 1, 1 }), lod_scale));
				p_corner_positions[7] = tgm_v3i_add(block_min_coordinates, tgm_v3i_muli(tgm_v3i_add(position, (v3i) { 1, 1, 1 }), lod_scale));

				const u8 direction_validity_mask =
					((position_with_padding.x > TG_TRANSVOXEL_PADDING_PER_SIDE ? 1 : 0) << 0) |
					((position_with_padding.y > TG_TRANSVOXEL_PADDING_PER_SIDE ? 1 : 0) << 1) |
					((position_with_padding.z > TG_TRANSVOXEL_PADDING_PER_SIDE ? 1 : 0) << 2);

				const u8 regular_cell_class_index = p_regular_cell_class[case_code];
				const tg_regular_cell_data* p_cell_data = &p_regular_cell_data[regular_cell_class_index];
				const u8 triangle_count = TG_TRANSVOXEL_CELL_GET_TRIANGLE_COUNT(*p_cell_data);
				const u8 vertex_count = TG_TRANSVOXEL_CELL_GET_VERTEX_COUNT(*p_cell_data);

				u16 p_cell_vertex_indices[12] = { TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX };

				const u8 cell_border_mask = tgi_get_border_mask(position, TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE);

				for (u8 i = 0; i < vertex_count; i++)
				{
					const u16 regular_vertex_data = p_regular_vertex_data[case_code][i];
					const u8 edge_code_low = regular_vertex_data & 0xff;
					const u8 edge_code_high = (regular_vertex_data >> 8) & 0xff;

					const u8 v0 = (edge_code_low >> 4) & 0xf;
					const u8 v1 = edge_code_low & 0xf;
					TG_ASSERT(v0 < v1);

					v3i i0 = p_sample_positions[v0];
					v3i i1 = p_sample_positions[v1];

					i32 d0 = p_cell_samples[v0];
					i32 d1 = p_cell_samples[v1];

					v3i p0 = p_corner_positions[v0];
					v3i p1 = p_corner_positions[v1];

					for (u8 i = 0; i < lod; i++)
					{
						v3i midpoint = { 0 };
						midpoint.x = (i0.x + i1.x) / 2;
						midpoint.y = (i0.y + i1.y) / 2;
						midpoint.z = (i0.z + i1.z) / 2;
						const i8 imid = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, midpoint);

						if (imid < 0 && d0 < 0 || imid >= 0 && d0 >= 0)
						{
							d0 = imid;
							i0 = midpoint;
							p0.x = (p0.x + p1.x) / 2;
							p0.y = (p0.y + p1.y) / 2;
							p0.z = (p0.z + p1.z) / 2;
						}
						else if (imid < 0 && d1 < 0 || imid >= 0 && d1 >= 0)
						{
							d1 = imid;
							i1 = midpoint;
							p1.x = (p0.x + p1.x) / 2;
							p1.y = (p0.y + p1.y) / 2;
							p1.z = (p0.z + p1.z) / 2;
						}
					}

					TG_ASSERT(d0 != d1);

					const i32 t = (d1 << 8) / (d1 - d0);

					const f32 t0 = (f32)t / 256.0f;
					const f32 t1 = (f32)(0x100 - t) / 256.0f;
					const i32 ti0 = t;
					const i32 ti1 = 0x100 - t;

					if (t & 0xff)
					{
						const u8 reuse_direction = (edge_code_high >> 4) & 0xf;
						const u8 reuse_vertex_index = edge_code_high & 0xf;

						const b32 present = (reuse_direction & direction_validity_mask) == reuse_direction;

						if (present)
						{
							const v3i cache_position = tgm_v3i_add(position_with_padding, TGI_DIRECTION_TO_PRECEDING_CELL(reuse_direction));
							const tgi_reuse_cell* p_preceding_cell = &TGI_REUSE_CELL_AT(*p_reuse_cells, cache_position);
							p_cell_vertex_indices[i] = p_preceding_cell->p_indices[reuse_vertex_index];
						}

						if (!present || p_cell_vertex_indices[i] == TG_U16_MAX)
						{
							const v3i primary = tgm_v3i_add(tgm_v3i_muli(p0, ti0), tgm_v3i_muli(p1, ti1));
							const v3 primaryf = tgm_v3_mulf(tgm_v3i_to_v3(primary), TGI_FIXED_FACTOR);
							const v3 normal = tgi_normalized_not_null(tgm_v3_add(tgm_v3_mulf(p_corner_gradients[v0], t0), tgm_v3_mulf(p_corner_gradients[v1], t1)));

							v3 secondary = { 0 };
							u16 border_mask = cell_border_mask;

							if (cell_border_mask > 0)
							{
								secondary = tgi_get_secondary_position(primaryf, normal, 0, scaled_block_stride_in_cells);
								border_mask |= (tgi_get_border_mask(p0, scaled_block_stride_in_cells) & tgi_get_border_mask(p1, scaled_block_stride_in_cells)) << 6;
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
							secondary = tgi_get_secondary_position(primaryf, normal, 0, scaled_block_stride_in_cells);
							border_mask |= tgi_get_border_mask(p1, scaled_block_stride_in_cells) << 6;
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
							const v3i cache_position = tgm_v3i_add(position_with_padding, TGI_DIRECTION_TO_PRECEDING_CELL(reuse_direction));
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
								secondary = tgi_get_secondary_position(primaryf, normal, 0, scaled_block_stride_in_cells);
								border_mask |= tgi_get_border_mask(primary, scaled_block_stride_in_cells) >> 6;
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

	TG_MEMORY_STACK_FREE(sizeof(*p_reuse_cells));
}

void tgi_build_transition(v3i octree_min_coordinates, v3i block_offset_in_octree, u8 lod, const i8* p_voxel_map, u8 direction, u16* p_vertex_count, tg_transvoxel_vertex* p_vertex_buffer, u16* p_index_count, u16* p_index_buffer)
{
	TG_ASSERT(lod > 0);

	tgi_reuse_transition_cells* p_reuse_cells = TG_MEMORY_STACK_ALLOC(sizeof(*p_reuse_cells));
	tg_memory_set_all_bits(sizeof(*p_reuse_cells), p_reuse_cells);

	const i32 scaled_block_stride_in_cells = TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE << lod;
	const i32 low_res_lod_scale = 1 << lod;
	const i32 high_res_lod_scale = 1 << (lod - 1);

	v3i p_sample_positions[13] = { 0 };
	i8 p_cell_samples[13] = { 0 };
	v3i p_cell_positions[13] = { 0 };
	v3 p_cell_gradients[13] = { 0 };

	for (u8 y = 0; y < TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE; y++)
	{
		const u8 yd = 2 * y;
		const u8 yp = y + TG_TRANSVOXEL_PADDING_PER_SIDE;

		for (u8 x = 0; x < TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE; x++)
		{
			const u8 xd = 2 * x;
			const u8 xp = x + TG_TRANSVOXEL_PADDING_PER_SIDE;

			const v3i sample_positions_pad = tgm_v3i_addi(block_offset_in_octree, TG_TRANSVOXEL_VOXEL_MAP_PADDING);
			p_sample_positions[0x0] = tgi_transition_face_to_block(sample_positions_pad, lod, xd    , yd    , direction);
			p_sample_positions[0x1] = tgi_transition_face_to_block(sample_positions_pad, lod, xd + 1, yd    , direction);
			p_sample_positions[0x2] = tgi_transition_face_to_block(sample_positions_pad, lod, xd + 2, yd    , direction);
			p_sample_positions[0x3] = tgi_transition_face_to_block(sample_positions_pad, lod, xd    , yd + 1, direction);
			p_sample_positions[0x4] = tgi_transition_face_to_block(sample_positions_pad, lod, xd + 1, yd + 1, direction);
			p_sample_positions[0x5] = tgi_transition_face_to_block(sample_positions_pad, lod, xd + 2, yd + 1, direction);
			p_sample_positions[0x6] = tgi_transition_face_to_block(sample_positions_pad, lod, xd    , yd + 2, direction);
			p_sample_positions[0x7] = tgi_transition_face_to_block(sample_positions_pad, lod, xd + 1, yd + 2, direction);
			p_sample_positions[0x8] = tgi_transition_face_to_block(sample_positions_pad, lod, xd + 2, yd + 2, direction);
			p_sample_positions[0x9] = p_sample_positions[0x0];
			p_sample_positions[0xa] = p_sample_positions[0x2];
			p_sample_positions[0xb] = p_sample_positions[0x6];
			p_sample_positions[0xc] = p_sample_positions[0x8];

			p_cell_samples[0x0] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x0]);
			p_cell_samples[0x1] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x1]);
			p_cell_samples[0x2] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x2]);
			p_cell_samples[0x3] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x3]);
			p_cell_samples[0x4] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x4]);
			p_cell_samples[0x5] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x5]);
			p_cell_samples[0x6] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x6]);
			p_cell_samples[0x7] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x7]);
			p_cell_samples[0x8] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x8]);
			p_cell_samples[0x9] = p_cell_samples[0x0];
			p_cell_samples[0xa] = p_cell_samples[0x2];
			p_cell_samples[0xb] = p_cell_samples[0x6];
			p_cell_samples[0xc] = p_cell_samples[0x8];

			const u16 case_code =
				((p_cell_samples[0] >> 7) & 0x001) |
				((p_cell_samples[1] >> 6) & 0x002) |
				((p_cell_samples[2] >> 5) & 0x004) |
				((p_cell_samples[5] >> 4) & 0x008) |
				((p_cell_samples[8] >> 3) & 0x010) |
				((p_cell_samples[7] >> 2) & 0x020) |
				((p_cell_samples[6] >> 1) & 0x040) |
				((p_cell_samples[3] >> 0) & 0x080) |
				((p_cell_samples[4] >> 0) & 0x100);

			if (case_code == 0 || case_code == 511)
			{
				continue;
			}

			TG_ASSERT(case_code != 512);

			for (u8 i = 0; i < 9; i++)
			{
				const v3i p = p_sample_positions[i];

				const f32 nx = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, p.x - high_res_lod_scale, p.y                     , p.z                     ));
				const f32 ny = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, p.x                     , p.y - high_res_lod_scale, p.z                     ));
				const f32 nz = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, p.x                     , p.y                     , p.z - high_res_lod_scale));
				const f32 px = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, p.x + high_res_lod_scale, p.y                     , p.z                     ));
				const f32 py = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, p.x                     , p.y + high_res_lod_scale, p.z                     ));
				const f32 pz = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_voxel_map, p.x                     , p.y                     , p.z + high_res_lod_scale));

				p_cell_gradients[i].x = nx - px;
				p_cell_gradients[i].y = ny - py;
				p_cell_gradients[i].z = nz - pz;
			}
			p_cell_gradients[0x9] = p_cell_gradients[0x0];
			p_cell_gradients[0xa] = p_cell_gradients[0x2];
			p_cell_gradients[0xb] = p_cell_gradients[0x6];
			p_cell_gradients[0xc] = p_cell_gradients[0x8];

			const v3i cell_position_pad = tgm_v3i_add(octree_min_coordinates, block_offset_in_octree);
			p_cell_positions[0x0] = tgi_transition_face_to_block(cell_position_pad, lod, xd    , yd    , direction);
			p_cell_positions[0x1] = tgi_transition_face_to_block(cell_position_pad, lod, xd + 1, yd    , direction);
			p_cell_positions[0x2] = tgi_transition_face_to_block(cell_position_pad, lod, xd + 2, yd    , direction);
			p_cell_positions[0x3] = tgi_transition_face_to_block(cell_position_pad, lod, xd    , yd + 1, direction);
			p_cell_positions[0x4] = tgi_transition_face_to_block(cell_position_pad, lod, xd + 1, yd + 1, direction);
			p_cell_positions[0x5] = tgi_transition_face_to_block(cell_position_pad, lod, xd + 2, yd + 1, direction);
			p_cell_positions[0x6] = tgi_transition_face_to_block(cell_position_pad, lod, xd    , yd + 2, direction);
			p_cell_positions[0x7] = tgi_transition_face_to_block(cell_position_pad, lod, xd + 1, yd + 2, direction);
			p_cell_positions[0x8] = tgi_transition_face_to_block(cell_position_pad, lod, xd + 2, yd + 2, direction);
			p_cell_positions[0x9] = p_cell_positions[0x0];
			p_cell_positions[0xa] = p_cell_positions[0x2];
			p_cell_positions[0xb] = p_cell_positions[0x6];
			p_cell_positions[0xc] = p_cell_positions[0x8];

			TGI_REUSE_TRANSITION_CELL_AT(*p_reuse_cells, xp, yp).p_indices[0] = TG_U16_MAX;

			const u8 cell_class = p_transition_cell_class[case_code];
			TG_ASSERT((cell_class & 0x7f) <= 55);

			const tg_transition_cell_data* p_cell_data = &p_transition_cell_data[cell_class & 0x7f];
			const b32 invert = (cell_class & 0x80) != 0;

			const u8 vertex_count = TG_TRANSVOXEL_CELL_GET_VERTEX_COUNT(*p_cell_data);

			u16 p_cell_vertex_indices[12] = { TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX };
			TG_ASSERT(vertex_count <= 12);

			const u8 direction_validity_mask = (x > 0 ? 1 : 0) | ((y > 0 ? 1 : 0) << 1);
			const u8 cell_border_mask = tgi_get_border_mask(tgm_v3i_sub(p_cell_positions[0], cell_position_pad), scaled_block_stride_in_cells);

			for (u8 i = 0; i < vertex_count; i++)
			{
				const u16 edge_code = p_transition_vertex_data[case_code][i];
				const u8 v0 = (edge_code >> 4) & 0xf;
				const u8 v1 = edge_code & 0xf;
				TG_ASSERT(v0 < 13 && v1 < 13);

				v3i i0 = p_sample_positions[v0];
				v3i i1 = p_sample_positions[v1];

				i32 d0 = p_cell_samples[v0];
				i32 d1 = p_cell_samples[v1];
				TG_ASSERT(d0 != d1);

				v3i p0 = p_cell_positions[v0];
				v3i p1 = p_cell_positions[v1];

				const b32 high_resolution_side = v0 < 0x9 || v1 < 0x9;
				const u8 interpolation_end = high_resolution_side ? lod - 1 : lod;
				for (u8 i = 0; i < interpolation_end; i++)
				{
					v3i midpoint = { 0 };
					midpoint.x = (i0.x + i1.x) / 2;
					midpoint.y = (i0.y + i1.y) / 2;
					midpoint.z = (i0.z + i1.z) / 2;
					const i8 imid = TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(p_voxel_map, midpoint);

					if (imid < 0 && d0 < 0 || imid >= 0 && d0 >= 0)
					{
						d0 = imid;
						i0 = midpoint;
						p0.x = (p0.x + p1.x) / 2;
						p0.y = (p0.y + p1.y) / 2;
						p0.z = (p0.z + p1.z) / 2;
					}
					else if (imid < 0 && d1 < 0 || imid >= 0 && d1 >= 0)
					{
						d1 = imid;
						i1 = midpoint;
						p1.x = (p0.x + p1.x) / 2;
						p1.y = (p0.y + p1.y) / 2;
						p1.z = (p0.z + p1.z) / 2;
					}
				}
				TG_ASSERT(d0 != d1);

				const i32 t = (d1 << 8) / (d1 - d0);

				const f32 t0 = (f32)t / 256.0f;
				const f32 t1 = (f32)(0x100 - t) / 256.0f;
				const i32 ti0 = t;
				const i32 ti1 = 0x100 - t;

				if (t & 0xff)
				{
					const u8 vertex_index_to_reuse_or_create = (edge_code >> 8) & 0xf;
					const u8 reuse_direction = edge_code >> 12;
					const b32 present = (reuse_direction & direction_validity_mask) == reuse_direction;

					if (present)
					{
						const tgi_reuse_transition_cell* p_preceding_cell = &TGI_REUSE_TRANSITION_CELL_AT(*p_reuse_cells, xp - (reuse_direction & 1), yp - ((reuse_direction >> 1) & 1));
						p_cell_vertex_indices[i] = p_preceding_cell->p_indices[vertex_index_to_reuse_or_create];
					}

					if (!present || p_cell_vertex_indices[i] == TG_U16_MAX)
					{
						const v3 n0 = p_cell_gradients[v0];
						const v3 n1 = p_cell_gradients[v1];

						const v3i primary = tgm_v3i_add(tgm_v3i_muli(p0, ti0), tgm_v3i_muli(p1, ti1));
						const v3 primaryf = tgm_v3_mulf(tgm_v3i_to_v3(primary), TGI_FIXED_FACTOR);
						const v3 normal = tgi_normalized_not_null(tgm_v3_add(tgm_v3_mulf(n0, t0), tgm_v3_mulf(n1, t1)));

						u16 border_mask = 0;

						v3 secondary = { 0 };
						if (!high_resolution_side)
						{
							secondary = tgi_get_secondary_position(primaryf, normal, 0, scaled_block_stride_in_cells);
							border_mask = cell_border_mask | ((tgi_get_border_mask(p0, scaled_block_stride_in_cells) & tgi_get_border_mask(p1, scaled_block_stride_in_cells)) << 6);
						}

						p_cell_vertex_indices[i] = tgi_emit_vertex(primaryf, normal, border_mask, secondary, p_vertex_count, p_vertex_buffer, p_index_count, p_index_buffer);

						if (reuse_direction & 0x8)
						{
							tgi_reuse_transition_cell* p_reuse_cell = &TGI_REUSE_TRANSITION_CELL_AT(*p_reuse_cells, xp, yp);
							p_reuse_cell->p_indices[vertex_index_to_reuse_or_create] = p_cell_vertex_indices[i];
						}
					}
				}
				else
				{
					const u8 v = t ? v0 : v1;
					const u8 corner_data = p_transition_corner_data[v];
					const u8 vertex_index_to_reuse_or_create = corner_data & 0xf;
					const u8 reuse_direction = (corner_data >> 4) & 0xf;
					const b32 present = (reuse_direction & direction_validity_mask) == reuse_direction;

					if (present)
					{
						const tgi_reuse_transition_cell* p_preceding_cell = &TGI_REUSE_TRANSITION_CELL_AT(*p_reuse_cells, xp - (reuse_direction & 1), yp - ((reuse_direction >> 1) & 1));
						p_cell_vertex_indices[i] = p_preceding_cell->p_indices[vertex_index_to_reuse_or_create];
					}

					if (!present || p_cell_vertex_indices[i] == TG_U16_MAX)
					{
						const v3i primary = p_cell_positions[v];
						const v3 primaryf = tgm_v3i_to_v3(primary);
						const v3 normal = tgi_normalized_not_null(p_cell_gradients[v]);

						const b32 high_resolution_side = v < 0x9;
						u16 border_mask = 0;

						v3 secondary = { 0 };
						if (!high_resolution_side)
						{
							secondary = tgi_get_secondary_position(primaryf, normal, 0, scaled_block_stride_in_cells);
							border_mask = cell_border_mask | (tgi_get_border_mask(primary, scaled_block_stride_in_cells) << 6);
						}

						p_cell_vertex_indices[i] = tgi_emit_vertex(primaryf, normal, border_mask, secondary, p_vertex_count, p_vertex_buffer, p_index_count, p_index_buffer);

						tgi_reuse_transition_cell* p_reuse_cell = &TGI_REUSE_TRANSITION_CELL_AT(*p_reuse_cells, xp, yp);
						p_reuse_cell->p_indices[vertex_index_to_reuse_or_create] = p_cell_vertex_indices[i];
					}
				}
			}

			const u8 triangle_count = TG_TRANSVOXEL_CELL_GET_TRIANGLE_COUNT(*p_cell_data);
			for (u8 t = 0; t < triangle_count; t++)
			{
				if (invert)
				{
					p_index_buffer[(*p_index_count)++] = p_cell_vertex_indices[p_cell_data->p_vertex_indices[3 * t + 2]];
					p_index_buffer[(*p_index_count)++] = p_cell_vertex_indices[p_cell_data->p_vertex_indices[3 * t + 1]];
					p_index_buffer[(*p_index_count)++] = p_cell_vertex_indices[p_cell_data->p_vertex_indices[3 * t    ]];
				}
				else
				{
					p_index_buffer[(*p_index_count)++] = p_cell_vertex_indices[p_cell_data->p_vertex_indices[3 * t    ]];
					p_index_buffer[(*p_index_count)++] = p_cell_vertex_indices[p_cell_data->p_vertex_indices[3 * t + 1]];
					p_index_buffer[(*p_index_count)++] = p_cell_vertex_indices[p_cell_data->p_vertex_indices[3 * t + 2]];
				}
			}
		}
	}

	TG_MEMORY_STACK_FREE(sizeof(*p_reuse_cells));
}

void tgi_build_node(tg_transvoxel_node* p_node, v3 camera_position, v3i octree_min_coordinates, v3i block_offset_in_octree, u8 lod, const i8* p_voxel_map)
{
	tg_memory_nullify(sizeof(*p_node), p_node);

	const u64 vertex_buffer_size = 15 * TG_TRANSVOXEL_CELLS_PER_BLOCK * sizeof(tg_transvoxel_vertex);
	const u64 index_buffer_size = 15 * TG_TRANSVOXEL_CELLS_PER_BLOCK * sizeof(u16);

	tg_transvoxel_vertex* p_vertex_buffer = TG_MEMORY_STACK_ALLOC(vertex_buffer_size);
	u16* p_index_buffer = TG_MEMORY_STACK_ALLOC(index_buffer_size);

	const b32 split = tgi_should_split_node(camera_position, octree_min_coordinates, block_offset_in_octree, lod);
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
					const v3i child_block_offset_in_octree = tgm_v3i_add(block_offset_in_octree, tgm_v3i_mul((v3i) { x, y, z }, half_stride_vec));
					tgi_build_node(p_node->pp_children[i], camera_position, octree_min_coordinates, child_block_offset_in_octree, lod - 1, p_voxel_map);
				}
			}
		}
	}
	else
	{
		tgi_build_block(octree_min_coordinates, block_offset_in_octree, lod, p_voxel_map, &p_node->node.vertex_count, p_vertex_buffer, &p_node->node.index_count, p_index_buffer);

		if (p_node->node.index_count)
		{
			const u64 vertices_size = p_node->node.vertex_count * sizeof(*p_node->node.p_vertices);
			const u64 indices_size = p_node->node.index_count * sizeof(*p_node->node.p_indices);
			p_node->node.p_vertices = TG_MEMORY_ALLOC(vertices_size);
			p_node->node.p_indices = TG_MEMORY_ALLOC(indices_size);
			tg_memory_copy(vertices_size, p_vertex_buffer, p_node->node.p_vertices);
			tg_memory_copy(indices_size, p_index_buffer, p_node->node.p_indices);

			// TODO: save these somehow for destruction
			tg_mesh_h h_mesh = tg_mesh_create2(
				p_node->node.vertex_count, sizeof(tg_transvoxel_vertex), p_node->node.p_vertices,
				p_node->node.index_count, p_node->node.p_indices
			);
			tg_material_h h_material = tg_material_create_deferred(
				tg_vertex_shader_get("shaders/deferred_flat_terrain.vert"),
				tg_fragment_shader_get("shaders/deferred_flat_terrain.frag"),
				0, TG_NULL
			);
			p_node->node.entity = tg_entity_create(h_mesh, h_material);
		}

		if (lod > 0)
		{
			for (u8 i = 0; i < 6; i++)
			{
				tgi_build_transition(octree_min_coordinates, block_offset_in_octree, lod, p_voxel_map, i, &p_node->node.p_transition_vertex_counts[i], p_vertex_buffer, &p_node->node.p_transition_index_counts[i], p_index_buffer);

				if (p_node->node.p_transition_index_counts[i])
				{
					const u64 vertices_size = p_node->node.p_transition_vertex_counts[i] * sizeof(tg_transvoxel_vertex);
					const u64 indices_size = p_node->node.p_transition_index_counts[i] * sizeof(u16);
					p_node->node.pp_transition_vertices[i] = TG_MEMORY_ALLOC(vertices_size);
					p_node->node.pp_transition_indices[i] = TG_MEMORY_ALLOC(indices_size);
					tg_memory_copy(vertices_size, p_vertex_buffer, p_node->node.pp_transition_vertices[i]);
					tg_memory_copy(indices_size, p_index_buffer, p_node->node.pp_transition_indices[i]);

					// TODO: save these somehow for destruction
					tg_mesh_h h_mesh = tg_mesh_create2(
						p_node->node.p_transition_vertex_counts[i], sizeof(tg_transvoxel_vertex), p_node->node.pp_transition_vertices[i],
						p_node->node.p_transition_index_counts[i], p_node->node.pp_transition_indices[i]
					);
					tg_material_h h_material = tg_material_create_deferred(
						tg_vertex_shader_get("shaders/deferred_flat_terrain.vert"),
						tg_fragment_shader_get("shaders/deferred_flat_terrain.frag"),
						0, TG_NULL
					);
					p_node->node.p_transition_entities[i] = tg_entity_create(h_mesh, h_material);
				}
			}
		}
	}

	TG_MEMORY_STACK_FREE(index_buffer_size);
	TG_MEMORY_STACK_FREE(vertex_buffer_size);
}





tg_transvoxel_terrain_h tg_transvoxel_terrain_create(tg_camera_h h_camera)
{
	tg_transvoxel_terrain_h h_terrain = TG_MEMORY_ALLOC(sizeof(*h_terrain));
	h_terrain->h_camera = h_camera;
	h_terrain->octree.min_coordinates = (v3i){ -128, -128, -128 };

	const u64 voxel_map_size = TG_TRANSVOXEL_VOXEL_MAP_VOXELS * sizeof(i8);
	i8* p_voxel_map = TG_MEMORY_STACK_ALLOC(voxel_map_size);

	tgi_fill_voxel_map(h_terrain->octree.min_coordinates, p_voxel_map);
	h_terrain->octree.p_compressed_voxel_map = tgi_create_compressed_voxel_map(p_voxel_map);
	tgi_build_node(
		&h_terrain->octree.root,
		tg_camera_get_position(h_camera),
		h_terrain->octree.min_coordinates,
		V3I(0),
		tgi_determine_max_lod(),
		p_voxel_map
	);

	TG_MEMORY_STACK_FREE(voxel_map_size);

	return h_terrain;
}

void tg_transvoxel_terrain_destroy(tg_transvoxel_terrain_h h_terrain)
{
	TG_ASSERT(h_terrain);
	TG_ASSERT(TG_FALSE);
}

void tg_transvoxel_terrain_update(tg_transvoxel_terrain_h h_terrain)
{

}



#undef TGI_TRANSITION_CELL_SCALE
#undef TGI_REUSE_TRANSITION_CELL_AT
#undef TGI_REUSE_CELL_AT
#undef TGI_FIXED_FACTOR
#undef TGI_DIRECTION_TO_PRECEDING_CELL

#endif
