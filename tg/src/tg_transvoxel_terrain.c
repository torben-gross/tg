#include "tg_transvoxel_terrain.h"

#include "graphics/tg_transvoxel_terrain_internal.h"
#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "tg_transvoxel_lookup_tables.h"
#include "util/tg_string.h"



typedef enum tg_full_resolution_face
{
	TG_FULL_RESOLUTION_FACE_NEGATIVE_X    = 0,
	TG_FULL_RESOLUTION_FACE_POSITIVE_X    = 1,
	TG_FULL_RESOLUTION_FACE_NEGATIVE_Y    = 2,
	TG_FULL_RESOLUTION_FACE_POSITIVE_Y    = 3,
	TG_FULL_RESOLUTION_FACE_NEGATIVE_Z    = 4,
	TG_FULL_RESOLUTION_FACE_POSITIVE_Z    = 5
} tg_full_resolution_face;

typedef struct tg_worker_thread_info
{
	tg_terrain* p_terrain;
	tg_terrain_octree* p_octree;
	tg_terrain_octree_node* p_node;
	i8* p_voxel_map;
	v3i                        node_coords_offset_rel_to_octree;
	u8                         lod;
	u16                        first_index_of_lod;
} tg_worker_thread_info;





static void tg__fill_voxel_map(v3i octree_min_coordinates, i8* p_voxel_map_buffer)
{
	for (i32 z = 0; z < TG_TERRAIN_VOXEL_MAP_STRIDE; z++)
	{
		for (i32 y = 0; y < TG_TERRAIN_VOXEL_MAP_STRIDE; y++)
		{
			for (i32 x = 0; x < TG_TERRAIN_VOXEL_MAP_STRIDE; x++)
			{
				const f32 cell_coordinate_x = (f32)(octree_min_coordinates.x + x);
				const f32 cell_coordinate_y = (f32)(octree_min_coordinates.y + y);
				const f32 cell_coordinate_z = (f32)(octree_min_coordinates.z + z);

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
				f32 noise = (n * 64.0f) - (f32)(y + octree_min_coordinates.y - 128.0f);
				noise += 10.0f * n_caves;
				const f32 noise_clamped = tgm_f32_clamp(noise, -1.0f, 1.0f);
				const f32 f0 = (noise_clamped + 1.0f) * 0.5f;
				const f32 f1 = 254.0f * f0;
				i8 f2 = -(i8)(tgm_f32_round_to_i32(f1) - 127);
				TG_TERRAIN_VOXEL_MAP_AT(p_voxel_map_buffer, x, y, z) = f2;
			}
		}
	}
}

static void tg__compress_voxel_map(const i8* p_voxel_map, u32* p_size, void* p_compressed_voxel_map_buffer)
{
	*p_size = 4;

	u32 counter = 0;
	i8 current_value = p_voxel_map[0];
	const u32 entry_size = sizeof(counter) + sizeof(current_value);

	const i8* p_itvm = p_voxel_map;
	const i8* p_itvm_end = &p_itvm[TG_TERRAIN_VOXEL_MAP_VOXELS];
	i8* p_itcvm = &((i8*)p_compressed_voxel_map_buffer)[*p_size];

	while (p_itvm < p_itvm_end)
	{
		if (*p_itvm != current_value)
		{
			*(u32*)p_itcvm = counter;
			p_itcvm[sizeof(counter)] = current_value;
			p_itcvm = &p_itcvm[entry_size];
			*p_size += entry_size;

			counter = 0;
			current_value = *p_itvm;
		}
		counter++;
		p_itvm++;
	}

	*(u32*)p_itcvm = counter;
	p_itcvm[sizeof(counter)] = current_value;
	p_itcvm = &p_itcvm[entry_size];
	*p_size += entry_size;

	*((u32*)p_compressed_voxel_map_buffer) = *p_size;
}

static void tg__decompress_voxel_map(const void* p_compressed_voxel_map, i8* p_voxel_map_buffer)
{
	const u32 size = *(u32*)p_compressed_voxel_map;
	const u32 entry_size = sizeof(u32) + sizeof(i8);

	const i8* p_itcvm = &((i8*)p_compressed_voxel_map)[sizeof(size)];
	const i8* p_itcvm_end = &((i8*)p_compressed_voxel_map)[size];
	i8* p_itvm = p_voxel_map_buffer;

	while (p_itcvm < p_itcvm_end)
	{
		const u32 counter = *(u32*)p_itcvm;
		const i8 current_value = p_itcvm[sizeof(counter)];
		for (u32 i = 0; i < counter; i++)
		{
			*p_itvm++ = current_value;
		}
		p_itcvm = &p_itcvm[entry_size];
	}
}





static b32 tg__should_split_node(v3 camera_position, v3i absolute_min_coordinates, u8 lod)
{
	b32 result = TG_FALSE;

	if (lod != 0)
	{
		const i32 half_stride = TG_TERRAIN_CELLS_PER_BLOCK_SIDE * (1 << (lod - 1)); // 2^lod / 2
		
		const f32 center_x = (f32)(absolute_min_coordinates.x + half_stride);
		const f32 center_y = (f32)(absolute_min_coordinates.y + half_stride);
		const f32 center_z = (f32)(absolute_min_coordinates.z + half_stride);

		const f32 direction_x = center_x - camera_position.x;
		const f32 direction_y = center_y - camera_position.y;
		const f32 direction_z = center_z - camera_position.z;

		const f32 distance_sqr = direction_x * direction_x + direction_y * direction_y + direction_z * direction_z;
		result = distance_sqr / (1 << (lod + lod)) < 4 * TG_TERRAIN_CELLS_PER_BLOCK_SIDE * TG_TERRAIN_CELLS_PER_BLOCK_SIDE;
	}

	return result;
}

static u8 tg__get_transition_mask(v3 camera_position, v3i octree_min_coordinates, v3i block_offset_in_octree, u8 lod)
{
	u8 transition_mask = 0;

	if (lod != 0)
	{
		const i32 stride = TG_TERRAIN_CELLS_PER_BLOCK_SIDE * (1 << lod);

		const i32 base_x = octree_min_coordinates.x + block_offset_in_octree.x;
		const i32 base_y = octree_min_coordinates.y + block_offset_in_octree.y;
		const i32 base_z = octree_min_coordinates.z + block_offset_in_octree.z;

		const v3i off0 = { base_x - stride, base_y, base_z };
		const v3i off1 = { base_x + stride, base_y, base_z };
		const v3i off2 = { base_x, base_y - stride, base_z };
		const v3i off3 = { base_x, base_y + stride, base_z };
		const v3i off4 = { base_x, base_y, base_z - stride };
		const v3i off5 = { base_x, base_y, base_z + stride };

		const b32 s0 = tg__should_split_node(camera_position, off0, lod);
		const b32 s1 = tg__should_split_node(camera_position, off1, lod);
		const b32 s2 = tg__should_split_node(camera_position, off2, lod);
		const b32 s3 = tg__should_split_node(camera_position, off3, lod);
		const b32 s4 = tg__should_split_node(camera_position, off4, lod);
		const b32 s5 = tg__should_split_node(camera_position, off5, lod);

		transition_mask |= s0;
		transition_mask |= s1 << 1;
		transition_mask |= s2 << 2;
		transition_mask |= s3 << 3;
		transition_mask |= s4 << 4;
		transition_mask |= s5 << 5;
	}

	return transition_mask;
}



static void tg__build_cells(v3i octree_min_coordinates, v3i block_offset_in_octree, u8 lod, const i8* p_voxel_map, u16* p_vertex_count, v3* p_position_buffer, v3* p_normal_buffer) // TODO: GPU!
{
	const i32 lod_scale = 1 << lod;

	v3i p_sample_positions[8] = { 0 };
	i8 p_cell_samples[8] = { 0 };
	v3i p_corner_positions[8] = { 0 };
	v3 p_triangle_positions[3] = { 0 };

	v3i position = { 0 };
	for (position.z = 0; position.z < TG_TERRAIN_CELLS_PER_BLOCK_SIDE; position.z++)
	{
		for (position.y = 0; position.y < TG_TERRAIN_CELLS_PER_BLOCK_SIDE; position.y++)
		{
			for (position.x = 0; position.x < TG_TERRAIN_CELLS_PER_BLOCK_SIDE; position.x++)
			{
				const i32 sample_position_pad_x = block_offset_in_octree.x + position.x * lod_scale;
				const i32 sample_position_pad_y = block_offset_in_octree.y + position.y * lod_scale;
				const i32 sample_position_pad_z = block_offset_in_octree.z + position.z * lod_scale;

				p_sample_positions[0].x = sample_position_pad_x;
				p_sample_positions[0].y = sample_position_pad_y;
				p_sample_positions[0].z = sample_position_pad_z;

				p_sample_positions[1].x = sample_position_pad_x + lod_scale;
				p_sample_positions[1].y = sample_position_pad_y;
				p_sample_positions[1].z = sample_position_pad_z;

				p_sample_positions[2].x = sample_position_pad_x;
				p_sample_positions[2].y = sample_position_pad_y + lod_scale;
				p_sample_positions[2].z = sample_position_pad_z;

				p_sample_positions[3].x = sample_position_pad_x + lod_scale;
				p_sample_positions[3].y = sample_position_pad_y + lod_scale;
				p_sample_positions[3].z = sample_position_pad_z;

				p_sample_positions[4].x = sample_position_pad_x;
				p_sample_positions[4].y = sample_position_pad_y;
				p_sample_positions[4].z = sample_position_pad_z + lod_scale;

				p_sample_positions[5].x = sample_position_pad_x + lod_scale;
				p_sample_positions[5].y = sample_position_pad_y;
				p_sample_positions[5].z = sample_position_pad_z + lod_scale;

				p_sample_positions[6].x = sample_position_pad_x;
				p_sample_positions[6].y = sample_position_pad_y + lod_scale;
				p_sample_positions[6].z = sample_position_pad_z + lod_scale;

				p_sample_positions[7].x = sample_position_pad_x + lod_scale;
				p_sample_positions[7].y = sample_position_pad_y + lod_scale;
				p_sample_positions[7].z = sample_position_pad_z + lod_scale;

				p_cell_samples[0] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0]);
				p_cell_samples[1] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[1]);
				p_cell_samples[2] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[2]);
				p_cell_samples[3] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[3]);
				p_cell_samples[4] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[4]);
				p_cell_samples[5] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[5]);
				p_cell_samples[6] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[6]);
				p_cell_samples[7] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[7]);

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

				if (case_code == 0 || case_code == 255)
				{
					continue;
				}
				
				const i32 cell_position_base_x = octree_min_coordinates.x + block_offset_in_octree.x + position.x * lod_scale;
				const i32 cell_position_base_y = octree_min_coordinates.y + block_offset_in_octree.y + position.y * lod_scale;
				const i32 cell_position_base_z = octree_min_coordinates.z + block_offset_in_octree.z + position.z * lod_scale;

				p_corner_positions[0].x = cell_position_base_x;
				p_corner_positions[0].y = cell_position_base_y;
				p_corner_positions[0].z = cell_position_base_z;

				p_corner_positions[1].x = cell_position_base_x + lod_scale;
				p_corner_positions[1].y = cell_position_base_y;
				p_corner_positions[1].z = cell_position_base_z;

				p_corner_positions[2].x = cell_position_base_x;
				p_corner_positions[2].y = cell_position_base_y + lod_scale;
				p_corner_positions[2].z = cell_position_base_z;

				p_corner_positions[3].x = cell_position_base_x + lod_scale;
				p_corner_positions[3].y = cell_position_base_y + lod_scale;
				p_corner_positions[3].z = cell_position_base_z;

				p_corner_positions[4].x = cell_position_base_x;
				p_corner_positions[4].y = cell_position_base_y;
				p_corner_positions[4].z = cell_position_base_z + lod_scale;

				p_corner_positions[5].x = cell_position_base_x + lod_scale;
				p_corner_positions[5].y = cell_position_base_y;
				p_corner_positions[5].z = cell_position_base_z + lod_scale;

				p_corner_positions[6].x = cell_position_base_x;
				p_corner_positions[6].y = cell_position_base_y + lod_scale;
				p_corner_positions[6].z = cell_position_base_z + lod_scale;

				p_corner_positions[7].x = cell_position_base_x + lod_scale;
				p_corner_positions[7].y = cell_position_base_y + lod_scale;
				p_corner_positions[7].z = cell_position_base_z + lod_scale;

				const u8 regular_cell_class_index = p_regular_cell_class[case_code];
				const tg_regular_cell_data* p_cell_data = &p_regular_cell_data[regular_cell_class_index];
				const u8 triangle_count = TG_TRANSVOXEL_CELL_GET_TRIANGLE_COUNT(*p_cell_data);

				for (u8 i = 0; i < triangle_count; i++)
				{
					for (u8 j = 0; j < 3; j++)
					{
						const u16 edge_code = p_regular_vertex_data[case_code][p_cell_data->p_vertex_indices[3 * i + j]];
						const u8 edge_code_low = edge_code & 0xff;

						const u8 v0 = (edge_code_low >> 4) & 0xf;
						const u8 v1 = edge_code_low & 0xf;
						TG_ASSERT(v0 < v1);

						v3i i0 = p_sample_positions[v0];
						v3i i1 = p_sample_positions[v1];

						i32 d0 = p_cell_samples[v0];
						i32 d1 = p_cell_samples[v1];

						v3i p0 = p_corner_positions[v0];
						v3i p1 = p_corner_positions[v1];

						for (u8 k = 0; k < lod; k++)
						{
							v3i midpoint = { 0 };
							midpoint.x = (i0.x + i1.x) / 2;
							midpoint.y = (i0.y + i1.y) / 2;
							midpoint.z = (i0.z + i1.z) / 2;
							const i8 imid = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, midpoint);

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

						if (t & 0xff)
						{
							p_triangle_positions[j].x = (f32)p0.x * t0 + (f32)p1.x * t1;
							p_triangle_positions[j].y = (f32)p0.y * t0 + (f32)p1.y * t1;
							p_triangle_positions[j].z = (f32)p0.z * t0 + (f32)p1.z * t1;
						}
						else
						{
							if (t == 0)
							{
								p_triangle_positions[j].x = (f32)p1.x;
								p_triangle_positions[j].y = (f32)p1.y;
								p_triangle_positions[j].z = (f32)p1.z;
							}
							else
							{
								p_triangle_positions[j].x = (f32)p0.x;
								p_triangle_positions[j].y = (f32)p0.y;
								p_triangle_positions[j].z = (f32)p0.z;
							}
						}
					}

					const f32 v01_x = p_triangle_positions[1].x - p_triangle_positions[0].x;
					const f32 v01_y = p_triangle_positions[1].y - p_triangle_positions[0].y;
					const f32 v01_z = p_triangle_positions[1].z - p_triangle_positions[0].z;

					const f32 v02_x = p_triangle_positions[2].x - p_triangle_positions[0].x;
					const f32 v02_y = p_triangle_positions[2].y - p_triangle_positions[0].y;
					const f32 v02_z = p_triangle_positions[2].z - p_triangle_positions[0].z;

					const f32 cross_x = v01_y * v02_z - v01_z * v02_y;
					const f32 cross_y = v01_z * v02_x - v01_x * v02_z;
					const f32 cross_z = v01_x * v02_y - v01_y * v02_x;

					const f32 magsqr = cross_x * cross_x + cross_y * cross_y + cross_z * cross_z;

					if (magsqr != 0.0f)
					{
						const f32 mag = tgm_f32_sqrt(magsqr);
						const v3 n = { cross_x / mag, cross_y / mag, cross_z / mag };

						p_position_buffer[0] = p_triangle_positions[0];
						p_position_buffer[1] = p_triangle_positions[1];
						p_position_buffer[2] = p_triangle_positions[2];
						p_position_buffer += 3;

						p_normal_buffer[0] = n;
						p_normal_buffer[1] = n;
						p_normal_buffer[2] = n;
						p_normal_buffer += 3;

						*p_vertex_count += 3;
					}
				}
			}
		}
	}
}

static void tg__build_transition_nx(v3i octree_min_coordinates, v3i block_offset_in_octree, u8 lod, const i8* p_voxel_map, u16* p_vertex_count, v3* p_position_buffer, v3* p_normal_buffer)
{
	TG_ASSERT(lod > 0);

	const i32 high_res_lod_scale = 1 << (lod - 1);

	v3i p_sample_positions[13] = { 0 };
	i8 p_cell_samples[13] = { 0 };
	v3i p_cell_positions[13] = { 0 };
	v3 p_triangle_positions[3] = { 0 };

	for (u8 y = 0; y < TG_TERRAIN_CELLS_PER_BLOCK_SIDE; y++)
	{
		const u8 yd = 2 * y;
		const i32 yd0 = yd * high_res_lod_scale;
		const i32 yd1 = yd0 + high_res_lod_scale;
		const i32 yd2 = yd1 + high_res_lod_scale;

		for (u8 x = 0; x < TG_TERRAIN_CELLS_PER_BLOCK_SIDE; x++)
		{
			const u8 xd = 2 * x;
			const i32 xd0 = xd * high_res_lod_scale;
			const i32 xd1 = xd0 + high_res_lod_scale;
			const i32 xd2 = xd1 + high_res_lod_scale;

			p_sample_positions[0x0] = (v3i){ block_offset_in_octree.x + 0, block_offset_in_octree.y + xd0, block_offset_in_octree.z + yd0 };
			p_sample_positions[0x1] = (v3i){ block_offset_in_octree.x + 0, block_offset_in_octree.y + xd1, block_offset_in_octree.z + yd0 };
			p_sample_positions[0x2] = (v3i){ block_offset_in_octree.x + 0, block_offset_in_octree.y + xd2, block_offset_in_octree.z + yd0 };
			p_sample_positions[0x3] = (v3i){ block_offset_in_octree.x + 0, block_offset_in_octree.y + xd0, block_offset_in_octree.z + yd1 };
			p_sample_positions[0x4] = (v3i){ block_offset_in_octree.x + 0, block_offset_in_octree.y + xd1, block_offset_in_octree.z + yd1 };
			p_sample_positions[0x5] = (v3i){ block_offset_in_octree.x + 0, block_offset_in_octree.y + xd2, block_offset_in_octree.z + yd1 };
			p_sample_positions[0x6] = (v3i){ block_offset_in_octree.x + 0, block_offset_in_octree.y + xd0, block_offset_in_octree.z + yd2 };
			p_sample_positions[0x7] = (v3i){ block_offset_in_octree.x + 0, block_offset_in_octree.y + xd1, block_offset_in_octree.z + yd2 };
			p_sample_positions[0x8] = (v3i){ block_offset_in_octree.x + 0, block_offset_in_octree.y + xd2, block_offset_in_octree.z + yd2 };
			p_sample_positions[0x9] = p_sample_positions[0x0];
			p_sample_positions[0xa] = p_sample_positions[0x2];
			p_sample_positions[0xb] = p_sample_positions[0x6];
			p_sample_positions[0xc] = p_sample_positions[0x8];

			p_cell_samples[0x0] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x0]);
			p_cell_samples[0x1] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x1]);
			p_cell_samples[0x2] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x2]);
			p_cell_samples[0x3] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x3]);
			p_cell_samples[0x4] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x4]);
			p_cell_samples[0x5] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x5]);
			p_cell_samples[0x6] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x6]);
			p_cell_samples[0x7] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x7]);
			p_cell_samples[0x8] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x8]);
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

			const v3i cell_position_pad = tgm_v3i_add(octree_min_coordinates, block_offset_in_octree);
			p_cell_positions[0x0] = (v3i){ cell_position_pad.x + 0, cell_position_pad.y + xd0, cell_position_pad.z + yd0 };
			p_cell_positions[0x1] = (v3i){ cell_position_pad.x + 0, cell_position_pad.y + xd1, cell_position_pad.z + yd0 };
			p_cell_positions[0x2] = (v3i){ cell_position_pad.x + 0, cell_position_pad.y + xd2, cell_position_pad.z + yd0 };
			p_cell_positions[0x3] = (v3i){ cell_position_pad.x + 0, cell_position_pad.y + xd0, cell_position_pad.z + yd1 };
			p_cell_positions[0x4] = (v3i){ cell_position_pad.x + 0, cell_position_pad.y + xd1, cell_position_pad.z + yd1 };
			p_cell_positions[0x5] = (v3i){ cell_position_pad.x + 0, cell_position_pad.y + xd2, cell_position_pad.z + yd1 };
			p_cell_positions[0x6] = (v3i){ cell_position_pad.x + 0, cell_position_pad.y + xd0, cell_position_pad.z + yd2 };
			p_cell_positions[0x7] = (v3i){ cell_position_pad.x + 0, cell_position_pad.y + xd1, cell_position_pad.z + yd2 };
			p_cell_positions[0x8] = (v3i){ cell_position_pad.x + 0, cell_position_pad.y + xd2, cell_position_pad.z + yd2 };
			p_cell_positions[0x9] = p_cell_positions[0x0];
			p_cell_positions[0xa] = p_cell_positions[0x2];
			p_cell_positions[0xb] = p_cell_positions[0x6];
			p_cell_positions[0xc] = p_cell_positions[0x8];

			const u8 cell_class = p_transition_cell_class[case_code];
			TG_ASSERT((cell_class & 0x7f) <= 55);

			const tg_transition_cell_data* p_cell_data = &p_transition_cell_data[cell_class & 0x7f];
			const b32 invert = (cell_class & 0x80) != 0;

			const u8 triangle_count = TG_TRANSVOXEL_CELL_GET_TRIANGLE_COUNT(*p_cell_data);

			for (u8 i = 0; i < triangle_count; i++)
			{
				for (u8 j = 0; j < 3; j++)
				{
					const u16 edge_code = p_transition_vertex_data[case_code][p_cell_data->p_vertex_indices[3 * i + j]];
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

					const b32 high_resolution_face = v0 < 0x9 || v1 < 0x9;
					const u8 interpolation_end = high_resolution_face ? lod - 1 : lod;
					for (u8 k = 0; k < interpolation_end; k++)
					{
						v3i midpoint = { 0 };
						midpoint.x = (i0.x + i1.x) / 2;
						midpoint.y = (i0.y + i1.y) / 2;
						midpoint.z = (i0.z + i1.z) / 2;
						const i8 d = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, midpoint);

						if (d < 0 && d0 < 0 || d >= 0 && d0 >= 0)
						{
							d0 = d;
							i0 = midpoint;
							p0.x = (p0.x + p1.x) / 2;
							p0.y = (p0.y + p1.y) / 2;
							p0.z = (p0.z + p1.z) / 2;
						}
						else if (d < 0 && d1 < 0 || d >= 0 && d1 >= 0)
						{
							d1 = d;
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

					if (t & 0xff)
					{
						p_triangle_positions[j].x = (f32)p0.x * t0 + (f32)p1.x * t1;
						p_triangle_positions[j].y = (f32)p0.y * t0 + (f32)p1.y * t1;
						p_triangle_positions[j].z = (f32)p0.z * t0 + (f32)p1.z * t1;
					}
					else
					{
						if (t == 0)
						{
							p_triangle_positions[j].x = (f32)p1.x;
							p_triangle_positions[j].y = (f32)p1.y;
							p_triangle_positions[j].z = (f32)p1.z;
						}
						else
						{
							p_triangle_positions[j].x = (f32)p0.x;
							p_triangle_positions[j].y = (f32)p0.y;
							p_triangle_positions[j].z = (f32)p0.z;
						}
					}
				}

				const f32 v01_x = p_triangle_positions[1].x - p_triangle_positions[0].x;
				const f32 v01_y = p_triangle_positions[1].y - p_triangle_positions[0].y;
				const f32 v01_z = p_triangle_positions[1].z - p_triangle_positions[0].z;

				const f32 v02_x = p_triangle_positions[2].x - p_triangle_positions[0].x;
				const f32 v02_y = p_triangle_positions[2].y - p_triangle_positions[0].y;
				const f32 v02_z = p_triangle_positions[2].z - p_triangle_positions[0].z;

				const f32 cross_x = v01_y * v02_z - v01_z * v02_y;
				const f32 cross_y = v01_z * v02_x - v01_x * v02_z;
				const f32 cross_z = v01_x * v02_y - v01_y * v02_x;

				const f32 magsqr = cross_x * cross_x + cross_y * cross_y + cross_z * cross_z;

				if (magsqr != 0.0f)
				{
					const f32 mag = tgm_f32_sqrt(magsqr);
					v3 n = { cross_x / mag, cross_y / mag, cross_z / mag };

					if (invert)
					{
						p_position_buffer[0] = p_triangle_positions[0];
						p_position_buffer[1] = p_triangle_positions[2];
						p_position_buffer[2] = p_triangle_positions[1];
						n.x = -n.x;
						n.y = -n.y;
						n.z = -n.z;
					}
					else
					{
						p_position_buffer[0] = p_triangle_positions[0];
						p_position_buffer[1] = p_triangle_positions[1];
						p_position_buffer[2] = p_triangle_positions[2];
					}
					p_position_buffer += 3;

					p_normal_buffer[0] = n;
					p_normal_buffer[1] = n;
					p_normal_buffer[2] = n;
					p_normal_buffer += 3;

					*p_vertex_count += 3;
				}
			}
		}
	}
}

static void tg__build_transition_px(v3i octree_min_coordinates, v3i block_offset_in_octree, u8 lod, const i8* p_voxel_map, u16* p_vertex_count, v3* p_position_buffer, v3* p_normal_buffer)
{
	TG_ASSERT(lod > 0);

	const i32 low_res_lod_scale = 1 << lod;
	const i32 high_res_lod_scale = 1 << (lod - 1);
	const i32 z1 = low_res_lod_scale * (TG_TERRAIN_VOXELS_PER_BLOCK_SIDE - 1);

	v3i p_sample_positions[13] = { 0 };
	i8 p_cell_samples[13] = { 0 };
	v3i p_cell_positions[13] = { 0 };
	v3 p_triangle_positions[3] = { 0 };

	for (u8 y = 0; y < TG_TERRAIN_CELLS_PER_BLOCK_SIDE; y++)
	{
		const u8 yd = 2 * y;
		const i32 yd0 = yd * high_res_lod_scale;
		const i32 yd1 = yd0 + high_res_lod_scale;
		const i32 yd2 = yd1 + high_res_lod_scale;

		for (u8 x = 0; x < TG_TERRAIN_CELLS_PER_BLOCK_SIDE; x++)
		{
			const u8 xd = 2 * x;
			const i32 xd0 = xd * high_res_lod_scale;
			const i32 xd1 = xd0 + high_res_lod_scale;
			const i32 xd2 = xd1 + high_res_lod_scale;

			p_sample_positions[0x0] = (v3i){ block_offset_in_octree.x + z1, block_offset_in_octree.y + yd0, block_offset_in_octree.z + xd0 };
			p_sample_positions[0x1] = (v3i){ block_offset_in_octree.x + z1, block_offset_in_octree.y + yd0, block_offset_in_octree.z + xd1 };
			p_sample_positions[0x2] = (v3i){ block_offset_in_octree.x + z1, block_offset_in_octree.y + yd0, block_offset_in_octree.z + xd2 };
			p_sample_positions[0x3] = (v3i){ block_offset_in_octree.x + z1, block_offset_in_octree.y + yd1, block_offset_in_octree.z + xd0 };
			p_sample_positions[0x4] = (v3i){ block_offset_in_octree.x + z1, block_offset_in_octree.y + yd1, block_offset_in_octree.z + xd1 };
			p_sample_positions[0x5] = (v3i){ block_offset_in_octree.x + z1, block_offset_in_octree.y + yd1, block_offset_in_octree.z + xd2 };
			p_sample_positions[0x6] = (v3i){ block_offset_in_octree.x + z1, block_offset_in_octree.y + yd2, block_offset_in_octree.z + xd0 };
			p_sample_positions[0x7] = (v3i){ block_offset_in_octree.x + z1, block_offset_in_octree.y + yd2, block_offset_in_octree.z + xd1 };
			p_sample_positions[0x8] = (v3i){ block_offset_in_octree.x + z1, block_offset_in_octree.y + yd2, block_offset_in_octree.z + xd2 };
			p_sample_positions[0x9] = p_sample_positions[0x0];
			p_sample_positions[0xa] = p_sample_positions[0x2];
			p_sample_positions[0xb] = p_sample_positions[0x6];
			p_sample_positions[0xc] = p_sample_positions[0x8];

			p_cell_samples[0x0] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x0]);
			p_cell_samples[0x1] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x1]);
			p_cell_samples[0x2] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x2]);
			p_cell_samples[0x3] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x3]);
			p_cell_samples[0x4] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x4]);
			p_cell_samples[0x5] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x5]);
			p_cell_samples[0x6] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x6]);
			p_cell_samples[0x7] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x7]);
			p_cell_samples[0x8] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x8]);
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

			const v3i cell_position_pad = tgm_v3i_add(octree_min_coordinates, block_offset_in_octree);
			p_cell_positions[0x0] = (v3i){ cell_position_pad.x + z1, cell_position_pad.y + yd0, cell_position_pad.z + xd0 };
			p_cell_positions[0x1] = (v3i){ cell_position_pad.x + z1, cell_position_pad.y + yd0, cell_position_pad.z + xd1 };
			p_cell_positions[0x2] = (v3i){ cell_position_pad.x + z1, cell_position_pad.y + yd0, cell_position_pad.z + xd2 };
			p_cell_positions[0x3] = (v3i){ cell_position_pad.x + z1, cell_position_pad.y + yd1, cell_position_pad.z + xd0 };
			p_cell_positions[0x4] = (v3i){ cell_position_pad.x + z1, cell_position_pad.y + yd1, cell_position_pad.z + xd1 };
			p_cell_positions[0x5] = (v3i){ cell_position_pad.x + z1, cell_position_pad.y + yd1, cell_position_pad.z + xd2 };
			p_cell_positions[0x6] = (v3i){ cell_position_pad.x + z1, cell_position_pad.y + yd2, cell_position_pad.z + xd0 };
			p_cell_positions[0x7] = (v3i){ cell_position_pad.x + z1, cell_position_pad.y + yd2, cell_position_pad.z + xd1 };
			p_cell_positions[0x8] = (v3i){ cell_position_pad.x + z1, cell_position_pad.y + yd2, cell_position_pad.z + xd2 };
			p_cell_positions[0x9] = p_cell_positions[0x0];
			p_cell_positions[0xa] = p_cell_positions[0x2];
			p_cell_positions[0xb] = p_cell_positions[0x6];
			p_cell_positions[0xc] = p_cell_positions[0x8];

			const u8 cell_class = p_transition_cell_class[case_code];
			TG_ASSERT((cell_class & 0x7f) <= 55);

			const tg_transition_cell_data* p_cell_data = &p_transition_cell_data[cell_class & 0x7f];
			const b32 invert = (cell_class & 0x80) != 0;

			const u8 triangle_count = TG_TRANSVOXEL_CELL_GET_TRIANGLE_COUNT(*p_cell_data);

			for (u8 i = 0; i < triangle_count; i++)
			{
				for (u8 j = 0; j < 3; j++)
				{
					const u16 edge_code = p_transition_vertex_data[case_code][p_cell_data->p_vertex_indices[3 * i + j]];
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

					const b32 high_resolution_face = v0 < 0x9 || v1 < 0x9;
					const u8 interpolation_end = high_resolution_face ? lod - 1 : lod;
					for (u8 k = 0; k < interpolation_end; k++)
					{
						v3i midpoint = { 0 };
						midpoint.x = (i0.x + i1.x) / 2;
						midpoint.y = (i0.y + i1.y) / 2;
						midpoint.z = (i0.z + i1.z) / 2;
						const i8 d = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, midpoint);

						if (d < 0 && d0 < 0 || d >= 0 && d0 >= 0)
						{
							d0 = d;
							i0 = midpoint;
							p0.x = (p0.x + p1.x) / 2;
							p0.y = (p0.y + p1.y) / 2;
							p0.z = (p0.z + p1.z) / 2;
						}
						else if (d < 0 && d1 < 0 || d >= 0 && d1 >= 0)
						{
							d1 = d;
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

					if (t & 0xff)
					{
						p_triangle_positions[j].x = (f32)p0.x * t0 + (f32)p1.x * t1;
						p_triangle_positions[j].y = (f32)p0.y * t0 + (f32)p1.y * t1;
						p_triangle_positions[j].z = (f32)p0.z * t0 + (f32)p1.z * t1;
					}
					else
					{
						if (t == 0)
						{
							p_triangle_positions[j].x = (f32)p1.x;
							p_triangle_positions[j].y = (f32)p1.y;
							p_triangle_positions[j].z = (f32)p1.z;
						}
						else
						{
							p_triangle_positions[j].x = (f32)p0.x;
							p_triangle_positions[j].y = (f32)p0.y;
							p_triangle_positions[j].z = (f32)p0.z;
						}
					}
				}

				const f32 v01_x = p_triangle_positions[1].x - p_triangle_positions[0].x;
				const f32 v01_y = p_triangle_positions[1].y - p_triangle_positions[0].y;
				const f32 v01_z = p_triangle_positions[1].z - p_triangle_positions[0].z;

				const f32 v02_x = p_triangle_positions[2].x - p_triangle_positions[0].x;
				const f32 v02_y = p_triangle_positions[2].y - p_triangle_positions[0].y;
				const f32 v02_z = p_triangle_positions[2].z - p_triangle_positions[0].z;

				const f32 cross_x = v01_y * v02_z - v01_z * v02_y;
				const f32 cross_y = v01_z * v02_x - v01_x * v02_z;
				const f32 cross_z = v01_x * v02_y - v01_y * v02_x;

				const f32 magsqr = cross_x * cross_x + cross_y * cross_y + cross_z * cross_z;

				if (magsqr != 0.0f)
				{
					const f32 mag = tgm_f32_sqrt(magsqr);
					v3 n = { cross_x / mag, cross_y / mag, cross_z / mag };

					if (invert)
					{
						p_position_buffer[0] = p_triangle_positions[0];
						p_position_buffer[1] = p_triangle_positions[2];
						p_position_buffer[2] = p_triangle_positions[1];
						n.x = -n.x;
						n.y = -n.y;
						n.z = -n.z;
					}
					else
					{
						p_position_buffer[0] = p_triangle_positions[0];
						p_position_buffer[1] = p_triangle_positions[1];
						p_position_buffer[2] = p_triangle_positions[2];
					}
					p_position_buffer += 3;

					p_normal_buffer[0] = n;
					p_normal_buffer[1] = n;
					p_normal_buffer[2] = n;
					p_normal_buffer += 3;

					*p_vertex_count += 3;
				}
			}
		}
	}
}

static void tg__build_transition_ny(v3i octree_min_coordinates, v3i block_offset_in_octree, u8 lod, const i8* p_voxel_map, u16* p_vertex_count, v3* p_position_buffer, v3* p_normal_buffer)
{
	TG_ASSERT(lod > 0);

	const i32 high_res_lod_scale = 1 << (lod - 1);

	v3i p_sample_positions[13] = { 0 };
	i8 p_cell_samples[13] = { 0 };
	v3i p_cell_positions[13] = { 0 };
	v3 p_triangle_positions[3] = { 0 };

	for (u8 y = 0; y < TG_TERRAIN_CELLS_PER_BLOCK_SIDE; y++)
	{
		const u8 yd = 2 * y;
		const i32 yd0 = yd * high_res_lod_scale;
		const i32 yd1 = yd0 + high_res_lod_scale;
		const i32 yd2 = yd1 + high_res_lod_scale;

		for (u8 x = 0; x < TG_TERRAIN_CELLS_PER_BLOCK_SIDE; x++)
		{
			const u8 xd = 2 * x;
			const i32 xd0 = xd * high_res_lod_scale;
			const i32 xd1 = xd0 + high_res_lod_scale;
			const i32 xd2 = xd1 + high_res_lod_scale;

			p_sample_positions[0x0] = (v3i){ block_offset_in_octree.x + yd0, block_offset_in_octree.y + 0, block_offset_in_octree.z + xd0 };
			p_sample_positions[0x1] = (v3i){ block_offset_in_octree.x + yd0, block_offset_in_octree.y + 0, block_offset_in_octree.z + xd1 };
			p_sample_positions[0x2] = (v3i){ block_offset_in_octree.x + yd0, block_offset_in_octree.y + 0, block_offset_in_octree.z + xd2 };
			p_sample_positions[0x3] = (v3i){ block_offset_in_octree.x + yd1, block_offset_in_octree.y + 0, block_offset_in_octree.z + xd0 };
			p_sample_positions[0x4] = (v3i){ block_offset_in_octree.x + yd1, block_offset_in_octree.y + 0, block_offset_in_octree.z + xd1 };
			p_sample_positions[0x5] = (v3i){ block_offset_in_octree.x + yd1, block_offset_in_octree.y + 0, block_offset_in_octree.z + xd2 };
			p_sample_positions[0x6] = (v3i){ block_offset_in_octree.x + yd2, block_offset_in_octree.y + 0, block_offset_in_octree.z + xd0 };
			p_sample_positions[0x7] = (v3i){ block_offset_in_octree.x + yd2, block_offset_in_octree.y + 0, block_offset_in_octree.z + xd1 };
			p_sample_positions[0x8] = (v3i){ block_offset_in_octree.x + yd2, block_offset_in_octree.y + 0, block_offset_in_octree.z + xd2 };
			p_sample_positions[0x9] = p_sample_positions[0x0];
			p_sample_positions[0xa] = p_sample_positions[0x2];
			p_sample_positions[0xb] = p_sample_positions[0x6];
			p_sample_positions[0xc] = p_sample_positions[0x8];

			p_cell_samples[0x0] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x0]);
			p_cell_samples[0x1] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x1]);
			p_cell_samples[0x2] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x2]);
			p_cell_samples[0x3] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x3]);
			p_cell_samples[0x4] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x4]);
			p_cell_samples[0x5] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x5]);
			p_cell_samples[0x6] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x6]);
			p_cell_samples[0x7] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x7]);
			p_cell_samples[0x8] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x8]);
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

			const v3i cell_position_pad = tgm_v3i_add(octree_min_coordinates, block_offset_in_octree);
			p_cell_positions[0x0] = (v3i){ cell_position_pad.x + yd0, cell_position_pad.y + 0, cell_position_pad.z + xd0 };
			p_cell_positions[0x1] = (v3i){ cell_position_pad.x + yd0, cell_position_pad.y + 0, cell_position_pad.z + xd1 };
			p_cell_positions[0x2] = (v3i){ cell_position_pad.x + yd0, cell_position_pad.y + 0, cell_position_pad.z + xd2 };
			p_cell_positions[0x3] = (v3i){ cell_position_pad.x + yd1, cell_position_pad.y + 0, cell_position_pad.z + xd0 };
			p_cell_positions[0x4] = (v3i){ cell_position_pad.x + yd1, cell_position_pad.y + 0, cell_position_pad.z + xd1 };
			p_cell_positions[0x5] = (v3i){ cell_position_pad.x + yd1, cell_position_pad.y + 0, cell_position_pad.z + xd2 };
			p_cell_positions[0x6] = (v3i){ cell_position_pad.x + yd2, cell_position_pad.y + 0, cell_position_pad.z + xd0 };
			p_cell_positions[0x7] = (v3i){ cell_position_pad.x + yd2, cell_position_pad.y + 0, cell_position_pad.z + xd1 };
			p_cell_positions[0x8] = (v3i){ cell_position_pad.x + yd2, cell_position_pad.y + 0, cell_position_pad.z + xd2 };
			p_cell_positions[0x9] = p_cell_positions[0x0];
			p_cell_positions[0xa] = p_cell_positions[0x2];
			p_cell_positions[0xb] = p_cell_positions[0x6];
			p_cell_positions[0xc] = p_cell_positions[0x8];

			const u8 cell_class = p_transition_cell_class[case_code];
			TG_ASSERT((cell_class & 0x7f) <= 55);

			const tg_transition_cell_data* p_cell_data = &p_transition_cell_data[cell_class & 0x7f];
			const b32 invert = (cell_class & 0x80) != 0;

			const u8 triangle_count = TG_TRANSVOXEL_CELL_GET_TRIANGLE_COUNT(*p_cell_data);

			for (u8 i = 0; i < triangle_count; i++)
			{
				for (u8 j = 0; j < 3; j++)
				{
					const u16 edge_code = p_transition_vertex_data[case_code][p_cell_data->p_vertex_indices[3 * i + j]];
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

					const b32 high_resolution_face = v0 < 0x9 || v1 < 0x9;
					const u8 interpolation_end = high_resolution_face ? lod - 1 : lod;
					for (u8 k = 0; k < interpolation_end; k++)
					{
						v3i midpoint = { 0 };
						midpoint.x = (i0.x + i1.x) / 2;
						midpoint.y = (i0.y + i1.y) / 2;
						midpoint.z = (i0.z + i1.z) / 2;
						const i8 d = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, midpoint);

						if (d < 0 && d0 < 0 || d >= 0 && d0 >= 0)
						{
							d0 = d;
							i0 = midpoint;
							p0.x = (p0.x + p1.x) / 2;
							p0.y = (p0.y + p1.y) / 2;
							p0.z = (p0.z + p1.z) / 2;
						}
						else if (d < 0 && d1 < 0 || d >= 0 && d1 >= 0)
						{
							d1 = d;
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

					if (t & 0xff)
					{
						p_triangle_positions[j].x = (f32)p0.x * t0 + (f32)p1.x * t1;
						p_triangle_positions[j].y = (f32)p0.y * t0 + (f32)p1.y * t1;
						p_triangle_positions[j].z = (f32)p0.z * t0 + (f32)p1.z * t1;
					}
					else
					{
						if (t == 0)
						{
							p_triangle_positions[j].x = (f32)p1.x;
							p_triangle_positions[j].y = (f32)p1.y;
							p_triangle_positions[j].z = (f32)p1.z;
						}
						else
						{
							p_triangle_positions[j].x = (f32)p0.x;
							p_triangle_positions[j].y = (f32)p0.y;
							p_triangle_positions[j].z = (f32)p0.z;
						}
					}
				}

				const f32 v01_x = p_triangle_positions[1].x - p_triangle_positions[0].x;
				const f32 v01_y = p_triangle_positions[1].y - p_triangle_positions[0].y;
				const f32 v01_z = p_triangle_positions[1].z - p_triangle_positions[0].z;

				const f32 v02_x = p_triangle_positions[2].x - p_triangle_positions[0].x;
				const f32 v02_y = p_triangle_positions[2].y - p_triangle_positions[0].y;
				const f32 v02_z = p_triangle_positions[2].z - p_triangle_positions[0].z;

				const f32 cross_x = v01_y * v02_z - v01_z * v02_y;
				const f32 cross_y = v01_z * v02_x - v01_x * v02_z;
				const f32 cross_z = v01_x * v02_y - v01_y * v02_x;

				const f32 magsqr = cross_x * cross_x + cross_y * cross_y + cross_z * cross_z;

				if (magsqr != 0.0f)
				{
					const f32 mag = tgm_f32_sqrt(magsqr);
					v3 n = { cross_x / mag, cross_y / mag, cross_z / mag };

					if (invert)
					{
						p_position_buffer[0] = p_triangle_positions[0];
						p_position_buffer[1] = p_triangle_positions[2];
						p_position_buffer[2] = p_triangle_positions[1];
						n.x = -n.x;
						n.y = -n.y;
						n.z = -n.z;
					}
					else
					{
						p_position_buffer[0] = p_triangle_positions[0];
						p_position_buffer[1] = p_triangle_positions[1];
						p_position_buffer[2] = p_triangle_positions[2];
					}
					p_position_buffer += 3;

					p_normal_buffer[0] = n;
					p_normal_buffer[1] = n;
					p_normal_buffer[2] = n;
					p_normal_buffer += 3;

					*p_vertex_count += 3;
				}
			}
		}
	}
}

static void tg__build_transition_py(v3i octree_min_coordinates, v3i block_offset_in_octree, u8 lod, const i8* p_voxel_map, u16* p_vertex_count, v3* p_position_buffer, v3* p_normal_buffer)
{
	TG_ASSERT(lod > 0);

	const i32 low_res_lod_scale = 1 << lod;
	const i32 high_res_lod_scale = 1 << (lod - 1);
	const i32 z1 = low_res_lod_scale * (TG_TERRAIN_VOXELS_PER_BLOCK_SIDE - 1);

	v3i p_sample_positions[13] = { 0 };
	i8 p_cell_samples[13] = { 0 };
	v3i p_cell_positions[13] = { 0 };
	v3 p_triangle_positions[3] = { 0 };

	for (u8 y = 0; y < TG_TERRAIN_CELLS_PER_BLOCK_SIDE; y++)
	{
		const u8 yd = 2 * y;
		const i32 yd0 = yd * high_res_lod_scale;
		const i32 yd1 = yd0 + high_res_lod_scale;
		const i32 yd2 = yd1 + high_res_lod_scale;

		for (u8 x = 0; x < TG_TERRAIN_CELLS_PER_BLOCK_SIDE; x++)
		{
			const u8 xd = 2 * x;
			const i32 xd0 = xd * high_res_lod_scale;
			const i32 xd1 = xd0 + high_res_lod_scale;
			const i32 xd2 = xd1 + high_res_lod_scale;

			p_sample_positions[0x0] = (v3i){ block_offset_in_octree.x + xd0, block_offset_in_octree.y + z1, block_offset_in_octree.z + yd0 };
			p_sample_positions[0x1] = (v3i){ block_offset_in_octree.x + xd1, block_offset_in_octree.y + z1, block_offset_in_octree.z + yd0 };
			p_sample_positions[0x2] = (v3i){ block_offset_in_octree.x + xd2, block_offset_in_octree.y + z1, block_offset_in_octree.z + yd0 };
			p_sample_positions[0x3] = (v3i){ block_offset_in_octree.x + xd0, block_offset_in_octree.y + z1, block_offset_in_octree.z + yd1 };
			p_sample_positions[0x4] = (v3i){ block_offset_in_octree.x + xd1, block_offset_in_octree.y + z1, block_offset_in_octree.z + yd1 };
			p_sample_positions[0x5] = (v3i){ block_offset_in_octree.x + xd2, block_offset_in_octree.y + z1, block_offset_in_octree.z + yd1 };
			p_sample_positions[0x6] = (v3i){ block_offset_in_octree.x + xd0, block_offset_in_octree.y + z1, block_offset_in_octree.z + yd2 };
			p_sample_positions[0x7] = (v3i){ block_offset_in_octree.x + xd1, block_offset_in_octree.y + z1, block_offset_in_octree.z + yd2 };
			p_sample_positions[0x8] = (v3i){ block_offset_in_octree.x + xd2, block_offset_in_octree.y + z1, block_offset_in_octree.z + yd2 };
			p_sample_positions[0x9] = p_sample_positions[0x0];
			p_sample_positions[0xa] = p_sample_positions[0x2];
			p_sample_positions[0xb] = p_sample_positions[0x6];
			p_sample_positions[0xc] = p_sample_positions[0x8];

			p_cell_samples[0x0] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x0]);
			p_cell_samples[0x1] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x1]);
			p_cell_samples[0x2] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x2]);
			p_cell_samples[0x3] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x3]);
			p_cell_samples[0x4] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x4]);
			p_cell_samples[0x5] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x5]);
			p_cell_samples[0x6] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x6]);
			p_cell_samples[0x7] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x7]);
			p_cell_samples[0x8] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x8]);
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

			const v3i cell_position_pad = tgm_v3i_add(octree_min_coordinates, block_offset_in_octree);
			p_cell_positions[0x0] = (v3i){ cell_position_pad.x + xd0, cell_position_pad.y + z1, cell_position_pad.z + yd0 };
			p_cell_positions[0x1] = (v3i){ cell_position_pad.x + xd1, cell_position_pad.y + z1, cell_position_pad.z + yd0 };
			p_cell_positions[0x2] = (v3i){ cell_position_pad.x + xd2, cell_position_pad.y + z1, cell_position_pad.z + yd0 };
			p_cell_positions[0x3] = (v3i){ cell_position_pad.x + xd0, cell_position_pad.y + z1, cell_position_pad.z + yd1 };
			p_cell_positions[0x4] = (v3i){ cell_position_pad.x + xd1, cell_position_pad.y + z1, cell_position_pad.z + yd1 };
			p_cell_positions[0x5] = (v3i){ cell_position_pad.x + xd2, cell_position_pad.y + z1, cell_position_pad.z + yd1 };
			p_cell_positions[0x6] = (v3i){ cell_position_pad.x + xd0, cell_position_pad.y + z1, cell_position_pad.z + yd2 };
			p_cell_positions[0x7] = (v3i){ cell_position_pad.x + xd1, cell_position_pad.y + z1, cell_position_pad.z + yd2 };
			p_cell_positions[0x8] = (v3i){ cell_position_pad.x + xd2, cell_position_pad.y + z1, cell_position_pad.z + yd2 };
			p_cell_positions[0x9] = p_cell_positions[0x0];
			p_cell_positions[0xa] = p_cell_positions[0x2];
			p_cell_positions[0xb] = p_cell_positions[0x6];
			p_cell_positions[0xc] = p_cell_positions[0x8];

			const u8 cell_class = p_transition_cell_class[case_code];
			TG_ASSERT((cell_class & 0x7f) <= 55);

			const tg_transition_cell_data* p_cell_data = &p_transition_cell_data[cell_class & 0x7f];
			const b32 invert = (cell_class & 0x80) != 0;

			const u8 triangle_count = TG_TRANSVOXEL_CELL_GET_TRIANGLE_COUNT(*p_cell_data);

			for (u8 i = 0; i < triangle_count; i++)
			{
				for (u8 j = 0; j < 3; j++)
				{
					const u16 edge_code = p_transition_vertex_data[case_code][p_cell_data->p_vertex_indices[3 * i + j]];
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

					const b32 high_resolution_face = v0 < 0x9 || v1 < 0x9;
					const u8 interpolation_end = high_resolution_face ? lod - 1 : lod;
					for (u8 k = 0; k < interpolation_end; k++)
					{
						v3i midpoint = { 0 };
						midpoint.x = (i0.x + i1.x) / 2;
						midpoint.y = (i0.y + i1.y) / 2;
						midpoint.z = (i0.z + i1.z) / 2;
						const i8 d = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, midpoint);

						if (d < 0 && d0 < 0 || d >= 0 && d0 >= 0)
						{
							d0 = d;
							i0 = midpoint;
							p0.x = (p0.x + p1.x) / 2;
							p0.y = (p0.y + p1.y) / 2;
							p0.z = (p0.z + p1.z) / 2;
						}
						else if (d < 0 && d1 < 0 || d >= 0 && d1 >= 0)
						{
							d1 = d;
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

					if (t & 0xff)
					{
						p_triangle_positions[j].x = (f32)p0.x * t0 + (f32)p1.x * t1;
						p_triangle_positions[j].y = (f32)p0.y * t0 + (f32)p1.y * t1;
						p_triangle_positions[j].z = (f32)p0.z * t0 + (f32)p1.z * t1;
					}
					else
					{
						if (t == 0)
						{
							p_triangle_positions[j].x = (f32)p1.x;
							p_triangle_positions[j].y = (f32)p1.y;
							p_triangle_positions[j].z = (f32)p1.z;
						}
						else
						{
							p_triangle_positions[j].x = (f32)p0.x;
							p_triangle_positions[j].y = (f32)p0.y;
							p_triangle_positions[j].z = (f32)p0.z;
						}
					}
				}

				const f32 v01_x = p_triangle_positions[1].x - p_triangle_positions[0].x;
				const f32 v01_y = p_triangle_positions[1].y - p_triangle_positions[0].y;
				const f32 v01_z = p_triangle_positions[1].z - p_triangle_positions[0].z;

				const f32 v02_x = p_triangle_positions[2].x - p_triangle_positions[0].x;
				const f32 v02_y = p_triangle_positions[2].y - p_triangle_positions[0].y;
				const f32 v02_z = p_triangle_positions[2].z - p_triangle_positions[0].z;

				const f32 cross_x = v01_y * v02_z - v01_z * v02_y;
				const f32 cross_y = v01_z * v02_x - v01_x * v02_z;
				const f32 cross_z = v01_x * v02_y - v01_y * v02_x;

				const f32 magsqr = cross_x * cross_x + cross_y * cross_y + cross_z * cross_z;

				if (magsqr != 0.0f)
				{
					const f32 mag = tgm_f32_sqrt(magsqr);
					v3 n = { cross_x / mag, cross_y / mag, cross_z / mag };

					if (invert)
					{
						p_position_buffer[0] = p_triangle_positions[0];
						p_position_buffer[1] = p_triangle_positions[2];
						p_position_buffer[2] = p_triangle_positions[1];
						n.x = -n.x;
						n.y = -n.y;
						n.z = -n.z;
					}
					else
					{
						p_position_buffer[0] = p_triangle_positions[0];
						p_position_buffer[1] = p_triangle_positions[1];
						p_position_buffer[2] = p_triangle_positions[2];
					}
					p_position_buffer += 3;

					p_normal_buffer[0] = n;
					p_normal_buffer[1] = n;
					p_normal_buffer[2] = n;
					p_normal_buffer += 3;

					*p_vertex_count += 3;
				}
			}
		}
	}
}

static void tg__build_transition_nz(v3i octree_min_coordinates, v3i block_offset_in_octree, u8 lod, const i8* p_voxel_map, u16* p_vertex_count, v3* p_position_buffer, v3* p_normal_buffer)
{
	TG_ASSERT(lod > 0);

	const i32 high_res_lod_scale = 1 << (lod - 1);

	v3i p_sample_positions[13] = { 0 };
	i8 p_cell_samples[13] = { 0 };
	v3i p_cell_positions[13] = { 0 };
	v3 p_triangle_positions[3] = { 0 };

	for (u8 y = 0; y < TG_TERRAIN_CELLS_PER_BLOCK_SIDE; y++)
	{
		const u8 yd = 2 * y;
		const i32 yd0 = yd * high_res_lod_scale;
		const i32 yd1 = yd0 + high_res_lod_scale;
		const i32 yd2 = yd1 + high_res_lod_scale;

		for (u8 x = 0; x < TG_TERRAIN_CELLS_PER_BLOCK_SIDE; x++)
		{
			const u8 xd = 2 * x;
			const i32 xd0 = xd * high_res_lod_scale;
			const i32 xd1 = xd0 + high_res_lod_scale;
			const i32 xd2 = xd1 + high_res_lod_scale;

			p_sample_positions[0x0] = (v3i){ block_offset_in_octree.x + xd0, block_offset_in_octree.y + yd0, block_offset_in_octree.z + 0 };
			p_sample_positions[0x1] = (v3i){ block_offset_in_octree.x + xd1, block_offset_in_octree.y + yd0, block_offset_in_octree.z + 0 };
			p_sample_positions[0x2] = (v3i){ block_offset_in_octree.x + xd2, block_offset_in_octree.y + yd0, block_offset_in_octree.z + 0 };
			p_sample_positions[0x3] = (v3i){ block_offset_in_octree.x + xd0, block_offset_in_octree.y + yd1, block_offset_in_octree.z + 0 };
			p_sample_positions[0x4] = (v3i){ block_offset_in_octree.x + xd1, block_offset_in_octree.y + yd1, block_offset_in_octree.z + 0 };
			p_sample_positions[0x5] = (v3i){ block_offset_in_octree.x + xd2, block_offset_in_octree.y + yd1, block_offset_in_octree.z + 0 };
			p_sample_positions[0x6] = (v3i){ block_offset_in_octree.x + xd0, block_offset_in_octree.y + yd2, block_offset_in_octree.z + 0 };
			p_sample_positions[0x7] = (v3i){ block_offset_in_octree.x + xd1, block_offset_in_octree.y + yd2, block_offset_in_octree.z + 0 };
			p_sample_positions[0x8] = (v3i){ block_offset_in_octree.x + xd2, block_offset_in_octree.y + yd2, block_offset_in_octree.z + 0 };
			p_sample_positions[0x9] = p_sample_positions[0x0];
			p_sample_positions[0xa] = p_sample_positions[0x2];
			p_sample_positions[0xb] = p_sample_positions[0x6];
			p_sample_positions[0xc] = p_sample_positions[0x8];

			p_cell_samples[0x0] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x0]);
			p_cell_samples[0x1] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x1]);
			p_cell_samples[0x2] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x2]);
			p_cell_samples[0x3] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x3]);
			p_cell_samples[0x4] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x4]);
			p_cell_samples[0x5] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x5]);
			p_cell_samples[0x6] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x6]);
			p_cell_samples[0x7] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x7]);
			p_cell_samples[0x8] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x8]);
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

			const v3i cell_position_pad = tgm_v3i_add(octree_min_coordinates, block_offset_in_octree);
			p_cell_positions[0x0] = (v3i){ cell_position_pad.x + xd0, cell_position_pad.y + yd0, cell_position_pad.z + 0 };
			p_cell_positions[0x1] = (v3i){ cell_position_pad.x + xd1, cell_position_pad.y + yd0, cell_position_pad.z + 0 };
			p_cell_positions[0x2] = (v3i){ cell_position_pad.x + xd2, cell_position_pad.y + yd0, cell_position_pad.z + 0 };
			p_cell_positions[0x3] = (v3i){ cell_position_pad.x + xd0, cell_position_pad.y + yd1, cell_position_pad.z + 0 };
			p_cell_positions[0x4] = (v3i){ cell_position_pad.x + xd1, cell_position_pad.y + yd1, cell_position_pad.z + 0 };
			p_cell_positions[0x5] = (v3i){ cell_position_pad.x + xd2, cell_position_pad.y + yd1, cell_position_pad.z + 0 };
			p_cell_positions[0x6] = (v3i){ cell_position_pad.x + xd0, cell_position_pad.y + yd2, cell_position_pad.z + 0 };
			p_cell_positions[0x7] = (v3i){ cell_position_pad.x + xd1, cell_position_pad.y + yd2, cell_position_pad.z + 0 };
			p_cell_positions[0x8] = (v3i){ cell_position_pad.x + xd2, cell_position_pad.y + yd2, cell_position_pad.z + 0 };
			p_cell_positions[0x9] = p_cell_positions[0x0];
			p_cell_positions[0xa] = p_cell_positions[0x2];
			p_cell_positions[0xb] = p_cell_positions[0x6];
			p_cell_positions[0xc] = p_cell_positions[0x8];

			const u8 cell_class = p_transition_cell_class[case_code];
			TG_ASSERT((cell_class & 0x7f) <= 55);

			const tg_transition_cell_data* p_cell_data = &p_transition_cell_data[cell_class & 0x7f];
			const b32 invert = (cell_class & 0x80) != 0;

			const u8 triangle_count = TG_TRANSVOXEL_CELL_GET_TRIANGLE_COUNT(*p_cell_data);

			for (u8 i = 0; i < triangle_count; i++)
			{
				for (u8 j = 0; j < 3; j++)
				{
					const u16 edge_code = p_transition_vertex_data[case_code][p_cell_data->p_vertex_indices[3 * i + j]];
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

					const b32 high_resolution_face = v0 < 0x9 || v1 < 0x9;
					const u8 interpolation_end = high_resolution_face ? lod - 1 : lod;
					for (u8 k = 0; k < interpolation_end; k++)
					{
						v3i midpoint = { 0 };
						midpoint.x = (i0.x + i1.x) / 2;
						midpoint.y = (i0.y + i1.y) / 2;
						midpoint.z = (i0.z + i1.z) / 2;
						const i8 d = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, midpoint);

						if (d < 0 && d0 < 0 || d >= 0 && d0 >= 0)
						{
							d0 = d;
							i0 = midpoint;
							p0.x = (p0.x + p1.x) / 2;
							p0.y = (p0.y + p1.y) / 2;
							p0.z = (p0.z + p1.z) / 2;
						}
						else if (d < 0 && d1 < 0 || d >= 0 && d1 >= 0)
						{
							d1 = d;
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

					if (t & 0xff)
					{
						p_triangle_positions[j].x = (f32)p0.x * t0 + (f32)p1.x * t1;
						p_triangle_positions[j].y = (f32)p0.y * t0 + (f32)p1.y * t1;
						p_triangle_positions[j].z = (f32)p0.z * t0 + (f32)p1.z * t1;
					}
					else
					{
						if (t == 0)
						{
							p_triangle_positions[j].x = (f32)p1.x;
							p_triangle_positions[j].y = (f32)p1.y;
							p_triangle_positions[j].z = (f32)p1.z;
						}
						else
						{
							p_triangle_positions[j].x = (f32)p0.x;
							p_triangle_positions[j].y = (f32)p0.y;
							p_triangle_positions[j].z = (f32)p0.z;
						}
					}
				}

				const f32 v01_x = p_triangle_positions[1].x - p_triangle_positions[0].x;
				const f32 v01_y = p_triangle_positions[1].y - p_triangle_positions[0].y;
				const f32 v01_z = p_triangle_positions[1].z - p_triangle_positions[0].z;

				const f32 v02_x = p_triangle_positions[2].x - p_triangle_positions[0].x;
				const f32 v02_y = p_triangle_positions[2].y - p_triangle_positions[0].y;
				const f32 v02_z = p_triangle_positions[2].z - p_triangle_positions[0].z;

				const f32 cross_x = v01_y * v02_z - v01_z * v02_y;
				const f32 cross_y = v01_z * v02_x - v01_x * v02_z;
				const f32 cross_z = v01_x * v02_y - v01_y * v02_x;

				const f32 magsqr = cross_x * cross_x + cross_y * cross_y + cross_z * cross_z;

				if (magsqr != 0.0f)
				{
					const f32 mag = tgm_f32_sqrt(magsqr);
					v3 n = { cross_x / mag, cross_y / mag, cross_z / mag };

					if (invert)
					{
						p_position_buffer[0] = p_triangle_positions[0];
						p_position_buffer[1] = p_triangle_positions[2];
						p_position_buffer[2] = p_triangle_positions[1];
						n.x = -n.x;
						n.y = -n.y;
						n.z = -n.z;
					}
					else
					{
						p_position_buffer[0] = p_triangle_positions[0];
						p_position_buffer[1] = p_triangle_positions[1];
						p_position_buffer[2] = p_triangle_positions[2];
					}
					p_position_buffer += 3;

					p_normal_buffer[0] = n;
					p_normal_buffer[1] = n;
					p_normal_buffer[2] = n;
					p_normal_buffer += 3;

					*p_vertex_count += 3;
				}
			}
		}
	}
}

static void tg__build_transition_pz(v3i octree_min_coordinates, v3i block_offset_in_octree, u8 lod, const i8* p_voxel_map, u16* p_vertex_count, v3* p_position_buffer, v3* p_normal_buffer)
{
	TG_ASSERT(lod > 0);

	const i32 low_res_lod_scale = 1 << lod;
	const i32 high_res_lod_scale = 1 << (lod - 1);
	const i32 z1 = low_res_lod_scale * (TG_TERRAIN_VOXELS_PER_BLOCK_SIDE - 1);

	v3i p_sample_positions[13] = { 0 };
	i8 p_cell_samples[13] = { 0 };
	v3i p_cell_positions[13] = { 0 };
	v3 p_triangle_positions[3] = { 0 };

	for (u8 y = 0; y < TG_TERRAIN_CELLS_PER_BLOCK_SIDE; y++)
	{
		const u8 yd = 2 * y;
		const i32 yd0 = yd * high_res_lod_scale;
		const i32 yd1 = yd0 + high_res_lod_scale;
		const i32 yd2 = yd1 + high_res_lod_scale;

		for (u8 x = 0; x < TG_TERRAIN_CELLS_PER_BLOCK_SIDE; x++)
		{
			const u8 xd = 2 * x;
			const i32 xd0 = xd * high_res_lod_scale;
			const i32 xd1 = xd0 + high_res_lod_scale;
			const i32 xd2 = xd1 + high_res_lod_scale;

			p_sample_positions[0x0] = (v3i){ block_offset_in_octree.x + yd0, block_offset_in_octree.y + xd0, block_offset_in_octree.z + z1 };
			p_sample_positions[0x1] = (v3i){ block_offset_in_octree.x + yd0, block_offset_in_octree.y + xd1, block_offset_in_octree.z + z1 };
			p_sample_positions[0x2] = (v3i){ block_offset_in_octree.x + yd0, block_offset_in_octree.y + xd2, block_offset_in_octree.z + z1 };
			p_sample_positions[0x3] = (v3i){ block_offset_in_octree.x + yd1, block_offset_in_octree.y + xd0, block_offset_in_octree.z + z1 };
			p_sample_positions[0x4] = (v3i){ block_offset_in_octree.x + yd1, block_offset_in_octree.y + xd1, block_offset_in_octree.z + z1 };
			p_sample_positions[0x5] = (v3i){ block_offset_in_octree.x + yd1, block_offset_in_octree.y + xd2, block_offset_in_octree.z + z1 };
			p_sample_positions[0x6] = (v3i){ block_offset_in_octree.x + yd2, block_offset_in_octree.y + xd0, block_offset_in_octree.z + z1 };
			p_sample_positions[0x7] = (v3i){ block_offset_in_octree.x + yd2, block_offset_in_octree.y + xd1, block_offset_in_octree.z + z1 };
			p_sample_positions[0x8] = (v3i){ block_offset_in_octree.x + yd2, block_offset_in_octree.y + xd2, block_offset_in_octree.z + z1 };
			p_sample_positions[0x9] = p_sample_positions[0x0];
			p_sample_positions[0xa] = p_sample_positions[0x2];
			p_sample_positions[0xb] = p_sample_positions[0x6];
			p_sample_positions[0xc] = p_sample_positions[0x8];

			p_cell_samples[0x0] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x0]);
			p_cell_samples[0x1] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x1]);
			p_cell_samples[0x2] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x2]);
			p_cell_samples[0x3] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x3]);
			p_cell_samples[0x4] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x4]);
			p_cell_samples[0x5] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x5]);
			p_cell_samples[0x6] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x6]);
			p_cell_samples[0x7] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x7]);
			p_cell_samples[0x8] = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x8]);
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

			const v3i cell_position_pad = tgm_v3i_add(octree_min_coordinates, block_offset_in_octree);
			p_cell_positions[0x0] = (v3i){ cell_position_pad.x + yd0, cell_position_pad.y + xd0, cell_position_pad.z + z1 };
			p_cell_positions[0x1] = (v3i){ cell_position_pad.x + yd0, cell_position_pad.y + xd1, cell_position_pad.z + z1 };
			p_cell_positions[0x2] = (v3i){ cell_position_pad.x + yd0, cell_position_pad.y + xd2, cell_position_pad.z + z1 };
			p_cell_positions[0x3] = (v3i){ cell_position_pad.x + yd1, cell_position_pad.y + xd0, cell_position_pad.z + z1 };
			p_cell_positions[0x4] = (v3i){ cell_position_pad.x + yd1, cell_position_pad.y + xd1, cell_position_pad.z + z1 };
			p_cell_positions[0x5] = (v3i){ cell_position_pad.x + yd1, cell_position_pad.y + xd2, cell_position_pad.z + z1 };
			p_cell_positions[0x6] = (v3i){ cell_position_pad.x + yd2, cell_position_pad.y + xd0, cell_position_pad.z + z1 };
			p_cell_positions[0x7] = (v3i){ cell_position_pad.x + yd2, cell_position_pad.y + xd1, cell_position_pad.z + z1 };
			p_cell_positions[0x8] = (v3i){ cell_position_pad.x + yd2, cell_position_pad.y + xd2, cell_position_pad.z + z1 };
			p_cell_positions[0x9] = p_cell_positions[0x0];
			p_cell_positions[0xa] = p_cell_positions[0x2];
			p_cell_positions[0xb] = p_cell_positions[0x6];
			p_cell_positions[0xc] = p_cell_positions[0x8];

			const u8 cell_class = p_transition_cell_class[case_code];
			TG_ASSERT((cell_class & 0x7f) <= 55);

			const tg_transition_cell_data* p_cell_data = &p_transition_cell_data[cell_class & 0x7f];
			const b32 invert = (cell_class & 0x80) != 0;

			const u8 triangle_count = TG_TRANSVOXEL_CELL_GET_TRIANGLE_COUNT(*p_cell_data);

			for (u8 i = 0; i < triangle_count; i++)
			{
				for (u8 j = 0; j < 3; j++)
				{
					const u16 edge_code = p_transition_vertex_data[case_code][p_cell_data->p_vertex_indices[3 * i + j]];
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

					const b32 high_resolution_face = v0 < 0x9 || v1 < 0x9;
					const u8 interpolation_end = high_resolution_face ? lod - 1 : lod;
					for (u8 k = 0; k < interpolation_end; k++)
					{
						v3i midpoint = { 0 };
						midpoint.x = (i0.x + i1.x) / 2;
						midpoint.y = (i0.y + i1.y) / 2;
						midpoint.z = (i0.z + i1.z) / 2;
						const i8 d = TG_TERRAIN_VOXEL_MAP_AT_V3I(p_voxel_map, midpoint);

						if (d < 0 && d0 < 0 || d >= 0 && d0 >= 0)
						{
							d0 = d;
							i0 = midpoint;
							p0.x = (p0.x + p1.x) / 2;
							p0.y = (p0.y + p1.y) / 2;
							p0.z = (p0.z + p1.z) / 2;
						}
						else if (d < 0 && d1 < 0 || d >= 0 && d1 >= 0)
						{
							d1 = d;
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

					if (t & 0xff)
					{
						p_triangle_positions[j].x = (f32)p0.x * t0 + (f32)p1.x * t1;
						p_triangle_positions[j].y = (f32)p0.y * t0 + (f32)p1.y * t1;
						p_triangle_positions[j].z = (f32)p0.z * t0 + (f32)p1.z * t1;
					}
					else
					{
						if (t == 0)
						{
							p_triangle_positions[j].x = (f32)p1.x;
							p_triangle_positions[j].y = (f32)p1.y;
							p_triangle_positions[j].z = (f32)p1.z;
						}
						else
						{
							p_triangle_positions[j].x = (f32)p0.x;
							p_triangle_positions[j].y = (f32)p0.y;
							p_triangle_positions[j].z = (f32)p0.z;
						}
					}
				}

				const f32 v01_x = p_triangle_positions[1].x - p_triangle_positions[0].x;
				const f32 v01_y = p_triangle_positions[1].y - p_triangle_positions[0].y;
				const f32 v01_z = p_triangle_positions[1].z - p_triangle_positions[0].z;

				const f32 v02_x = p_triangle_positions[2].x - p_triangle_positions[0].x;
				const f32 v02_y = p_triangle_positions[2].y - p_triangle_positions[0].y;
				const f32 v02_z = p_triangle_positions[2].z - p_triangle_positions[0].z;

				const f32 cross_x = v01_y * v02_z - v01_z * v02_y;
				const f32 cross_y = v01_z * v02_x - v01_x * v02_z;
				const f32 cross_z = v01_x * v02_y - v01_y * v02_x;

				const f32 magsqr = cross_x * cross_x + cross_y * cross_y + cross_z * cross_z;

				if (magsqr != 0.0f)
				{
					const f32 mag = tgm_f32_sqrt(magsqr);
					v3 n = { cross_x / mag, cross_y / mag, cross_z / mag };

					if (invert)
					{
						p_position_buffer[0] = p_triangle_positions[0];
						p_position_buffer[1] = p_triangle_positions[2];
						p_position_buffer[2] = p_triangle_positions[1];
						n.x = -n.x;
						n.y = -n.y;
						n.z = -n.z;
					}
					else
					{
						p_position_buffer[0] = p_triangle_positions[0];
						p_position_buffer[1] = p_triangle_positions[1];
						p_position_buffer[2] = p_triangle_positions[2];
					}
					p_position_buffer += 3;

					p_normal_buffer[0] = n;
					p_normal_buffer[1] = n;
					p_normal_buffer[2] = n;
					p_normal_buffer += 3;

					*p_vertex_count += 3;
				}
			}
		}
	}
}

static void tg__build_node(volatile tg_terrain* p_terrain, i8* p_voxel_map, u8 octree_index, u8 x, u8 y, u8 z, u8 lod, u16 first_index_of_lod)
{
	const u8 inv_lod = TG_TERRAIN_MAX_LOD - lod;
	const u8 sqrt_nodes_per_lod = 1 << inv_lod;
	const u16 i = first_index_of_lod + sqrt_nodes_per_lod * sqrt_nodes_per_lod * z + sqrt_nodes_per_lod * y + x;
	const i32 coord_scale = TG_TERRAIN_OCTREE_STRIDE_IN_CELLS / sqrt_nodes_per_lod;
	const v3i node_coords_offset_rel_to_octree = { x * coord_scale, y * coord_scale, z * coord_scale };

	volatile tg_terrain_octree* p_octree = &p_terrain->p_octrees[octree_index];
	volatile tg_terrain_octree_node* p_node = &p_octree->p_nodes[i];

	const u64 vertices_size = 15 * TG_TERRAIN_CELLS_PER_BLOCK * sizeof(v3);
	v3* p_position_buffer = TG_MEMORY_STACK_ALLOC_ASYNC(vertices_size);
	v3* p_normal_buffer = TG_MEMORY_STACK_ALLOC_ASYNC(vertices_size);

	u16 vertex_count = 0;

#if 1
	tg__build_cells(p_octree->min_coords, node_coords_offset_rel_to_octree, lod, p_voxel_map, &vertex_count, p_position_buffer, p_normal_buffer);

	if (vertex_count)
	{
		tg_mesh_h h_mesh = tg_mesh_create();
		tg_mesh_set_positions(h_mesh, vertex_count, p_position_buffer);
		tg_mesh_set_normals(h_mesh, vertex_count, p_normal_buffer);
		p_node->h_block_render_command = tg_render_command_create(h_mesh, p_terrain->h_material, V3(0), 0, TG_NULL);
	}
#else
	// TODO: GPU, create low priority queues for streaming assets and use that for
	// terrain generation. this will only be useful if this whole thing runs on another
	// thread, so it actually does not stall everything else
	{
		tg_mesh_h h_mesh = tg_transvoxel_create_regular_mesh(p_octree->min_coords, node_coords_offset_rel_to_octree, lod, p_voxel_map);
		p_node->h_block_render_command = tg_render_command_create(h_mesh, p_terrain->h_material, V3(0), 0, TG_NULL);
	}
#endif

	if (lod > 0)
	{
		u32 transition_index = 0;
	
		vertex_count = 0;
		tg__build_transition_nx(p_octree->min_coords, node_coords_offset_rel_to_octree, lod, p_voxel_map, &vertex_count, p_position_buffer, p_normal_buffer);
	
		tg_mesh_h h_mesh = TG_NULL;
		if (vertex_count)
		{
			h_mesh = tg_mesh_create();
			tg_mesh_set_positions(h_mesh, vertex_count, p_position_buffer);
			tg_mesh_set_normals(h_mesh, vertex_count, p_normal_buffer);
			p_node->ph_transition_render_commands[transition_index] = tg_render_command_create(h_mesh, p_terrain->h_material, V3(0), 0, TG_NULL);
		}
		transition_index++;
	
		vertex_count = 0;
		tg__build_transition_px(p_octree->min_coords, node_coords_offset_rel_to_octree, lod, p_voxel_map, &vertex_count, p_position_buffer, p_normal_buffer);
	
		if (vertex_count)
		{
			h_mesh = tg_mesh_create();
			tg_mesh_set_positions(h_mesh, vertex_count, p_position_buffer);
			tg_mesh_set_normals(h_mesh, vertex_count, p_normal_buffer);
			p_node->ph_transition_render_commands[transition_index] = tg_render_command_create(h_mesh, p_terrain->h_material, V3(0), 0, TG_NULL);
		}
		transition_index++;
	
		vertex_count = 0;
		tg__build_transition_ny(p_octree->min_coords, node_coords_offset_rel_to_octree, lod, p_voxel_map, &vertex_count, p_position_buffer, p_normal_buffer);
	
		if (vertex_count)
		{
			h_mesh = tg_mesh_create();
			tg_mesh_set_positions(h_mesh, vertex_count, p_position_buffer);
			tg_mesh_set_normals(h_mesh, vertex_count, p_normal_buffer);
			p_node->ph_transition_render_commands[transition_index] = tg_render_command_create(h_mesh, p_terrain->h_material, V3(0), 0, TG_NULL);
		}
		transition_index++;
	
		vertex_count = 0;
		tg__build_transition_py(p_octree->min_coords, node_coords_offset_rel_to_octree, lod, p_voxel_map, &vertex_count, p_position_buffer, p_normal_buffer);
	
		if (vertex_count)
		{
			h_mesh = tg_mesh_create();
			tg_mesh_set_positions(h_mesh, vertex_count, p_position_buffer);
			tg_mesh_set_normals(h_mesh, vertex_count, p_normal_buffer);
			p_node->ph_transition_render_commands[transition_index] = tg_render_command_create(h_mesh, p_terrain->h_material, V3(0), 0, TG_NULL);
		}
		transition_index++;
	
		vertex_count = 0;
		tg__build_transition_nz(p_octree->min_coords, node_coords_offset_rel_to_octree, lod, p_voxel_map, &vertex_count, p_position_buffer, p_normal_buffer);
	
		if (vertex_count)
		{
			h_mesh = tg_mesh_create();
			tg_mesh_set_positions(h_mesh, vertex_count, p_position_buffer);
			tg_mesh_set_normals(h_mesh, vertex_count, p_normal_buffer);
			p_node->ph_transition_render_commands[transition_index] = tg_render_command_create(h_mesh, p_terrain->h_material, V3(0), 0, TG_NULL);
		}
		transition_index++;
	
		vertex_count = 0;
		tg__build_transition_pz(p_octree->min_coords, node_coords_offset_rel_to_octree, lod, p_voxel_map, &vertex_count, p_position_buffer, p_normal_buffer);
	
		if (vertex_count)
		{
			h_mesh = tg_mesh_create();
			tg_mesh_set_positions(h_mesh, vertex_count, p_position_buffer);
			tg_mesh_set_normals(h_mesh, vertex_count, p_normal_buffer);
			p_node->ph_transition_render_commands[transition_index] = tg_render_command_create(h_mesh, p_terrain->h_material, V3(0), 0, TG_NULL);
		}
		transition_index++;
	}

	TG_MEMORY_STACK_FREE_ASYNC(vertices_size);
	TG_MEMORY_STACK_FREE_ASYNC(vertices_size);

	if (lod > 0)
	{
		const u16 child_first_index_of_lod = first_index_of_lod + (1 << (3 * inv_lod));

		const u8 x0 = 2 * x;
		const u8 y0 = 2 * y;
		const u8 z0 = 2 * z;

		const u8 x1 = x0 + 2;
		const u8 y1 = y0 + 2;
		const u8 z1 = z0 + 2;

		for (u8 iz = z0; iz < z1; iz++)
		{
			for (u8 iy = y0; iy < y1; iy++)
			{
				for (u8 ix = x0; ix < x1; ix++)
				{
					tg__build_node(p_terrain, p_voxel_map, octree_index, ix, iy, iz, lod - 1, child_first_index_of_lod);
				}
			}
		}
	}
}

static void tg__build_octree(tg_terrain* p_terrain, u8 octree_index, i32 x, i32 y, i32 z)
{
	volatile tg_terrain_octree* p_octree = &p_terrain->p_octrees[octree_index];
	p_octree->min_coords = (v3i) {
		x * TG_TERRAIN_OCTREE_STRIDE_IN_CELLS,
		y * TG_TERRAIN_OCTREE_STRIDE_IN_CELLS,
		z * TG_TERRAIN_OCTREE_STRIDE_IN_CELLS
	};

	const u64 voxel_map_size = TG_TERRAIN_VOXEL_MAP_VOXELS * sizeof(i8);
	i8* p_voxel_map = TG_MEMORY_STACK_ALLOC_ASYNC(voxel_map_size);

	char p_filename_buffer[TG_MAX_PATH] = { 0 };
	tg_string_format(sizeof(p_filename_buffer), p_filename_buffer, "terrain/octree_%i_%i_%i.txt", x, y, z);
	const b32 exists = tg_platform_file_exists(p_filename_buffer);
	if (!exists)
	{
		tg__fill_voxel_map(p_octree->min_coords, p_voxel_map);

		u32 compressed_voxel_map_size;
		void* p_compressed_voxel_map_buffer = TG_MEMORY_STACK_ALLOC_ASYNC(voxel_map_size);
		tg__compress_voxel_map(p_voxel_map, &compressed_voxel_map_size, p_compressed_voxel_map_buffer);
		tg_platform_file_create(p_filename_buffer, compressed_voxel_map_size, p_compressed_voxel_map_buffer, TG_FALSE);
		TG_MEMORY_STACK_FREE_ASYNC(voxel_map_size);
	}
	else
	{
		tg_file_properties file_properties = { 0 };
		tg_platform_file_get_properties(p_filename_buffer, &file_properties);
		i8* p_file_data = TG_MEMORY_STACK_ALLOC_ASYNC(file_properties.size);
		tg_platform_file_read(p_filename_buffer, file_properties.size, p_file_data);
		tg__decompress_voxel_map(p_file_data, p_voxel_map);
		TG_MEMORY_STACK_FREE_ASYNC(file_properties.size);
	}

	tg__build_node(p_terrain, p_voxel_map, octree_index, 0, 0, 0, TG_TERRAIN_MAX_LOD, 0);
	TG_MEMORY_STACK_FREE_ASYNC(voxel_map_size);
}



static void tg__render(tg_terrain* p_terrain, u8 octree_index, u8 x, u8 y, u8 z, u8 lod, u16 first_index_of_lod, tg_renderer_h h_renderer)
{
	const u8 inv_lod = TG_TERRAIN_MAX_LOD - lod;
	const u8 sqrt_nodes_per_lod = 1 << inv_lod;
	const u16 i = first_index_of_lod + sqrt_nodes_per_lod * sqrt_nodes_per_lod * z + sqrt_nodes_per_lod * y + x;
	const u16 should_split_bitmap_offset = i / 8;
	const u8 should_split_bitmap_mask = 1 << (i % 8);

	if (p_terrain->p_octrees[octree_index].p_should_split_bitmap[should_split_bitmap_offset] & should_split_bitmap_mask)
	{
		const u16 child_first_index_of_lod = first_index_of_lod + (1 << (3 * inv_lod));

		const u8 x0 = 2 * x;
		const u8 y0 = 2 * y;
		const u8 z0 = 2 * z;

		const u8 x1 = x0 + 2;
		const u8 y1 = y0 + 2;
		const u8 z1 = z0 + 2;

		for (u8 iz = z0; iz < z1; iz++)
		{
			for (u8 iy = y0; iy < y1; iy++)
			{
				for (u8 ix = x0; ix < x1; ix++)
				{
					tg__render(p_terrain, octree_index, ix, iy, iz, lod - 1, child_first_index_of_lod, h_renderer);
				}
			}
		}
	}
	else
	{
		volatile tg_terrain_octree_node* p_node = &p_terrain->p_octrees[octree_index].p_nodes[i];
		
		if (p_node->h_block_render_command)
		{
			tg_renderer_exec(h_renderer, p_node->h_block_render_command);
		}

		const i32 scale = TG_TERRAIN_OCTREE_STRIDE_IN_CELLS / sqrt_nodes_per_lod;
		const v3i node_coords_offset_rel_to_octree = { x * scale, y * scale, z * scale };
		const u8 transition_mask = tg__get_transition_mask(p_terrain->p_camera->position, p_terrain->p_octrees[octree_index].min_coords, node_coords_offset_rel_to_octree, lod);;

		for (u8 transition_index = 0; transition_index < 6; transition_index++)
		{
			if ((transition_mask & (1 << transition_index)) && p_node->ph_transition_render_commands[transition_index])
			{
				tg_renderer_exec(h_renderer, p_node->ph_transition_render_commands[transition_index]);
			}
		}
	}
}

void tg__update(tg_terrain* p_terrain, u8 octree_index, u8 x, u8 y, u8 z, u8 lod, u16 first_index_of_lod)
{
	const u8 inv_lod = TG_TERRAIN_MAX_LOD - lod;
	const u8 sqrt_nodes_per_lod = 1 << inv_lod;
	const u16 i = first_index_of_lod + sqrt_nodes_per_lod * sqrt_nodes_per_lod * z + sqrt_nodes_per_lod * y + x;
	const u16 should_split_bitmap_offset = i / 8;
	const u8 should_split_bitmap_bit = i % 8;
	const u8 should_split_bitmap_mask = 1 << should_split_bitmap_bit;

	const i32 scale = TG_TERRAIN_OCTREE_STRIDE_IN_CELLS / sqrt_nodes_per_lod;
	const v3i node_coords_offset_rel_to_octree = { x * scale, y * scale, z * scale };

	const b32 should_split = tg__should_split_node(p_terrain->p_camera->position, tgm_v3i_add(p_terrain->p_octrees[octree_index].min_coords, node_coords_offset_rel_to_octree), lod);
	volatile u8* p_bitmask = &p_terrain->p_octrees[octree_index].p_should_split_bitmap[should_split_bitmap_offset];
	*p_bitmask &= ~should_split_bitmap_mask;
	*p_bitmask |= (u8)should_split << should_split_bitmap_bit;

	if (should_split && lod > 0)
	{
		const u16 child_first_index_of_lod = first_index_of_lod + (1 << (3 * inv_lod));

		const u8 x0 = 2 * x;
		const u8 y0 = 2 * y;
		const u8 z0 = 2 * z;

		const u8 x1 = x0 + 2;
		const u8 y1 = y0 + 2;
		const u8 z1 = z0 + 2;

		for (u8 iz = z0; iz < z1; iz++)
		{
			for (u8 iy = y0; iy < y1; iy++)
			{
				for (u8 ix = x0; ix < x1; ix++)
				{
					tg__update(p_terrain, octree_index, ix, iy, iz, lod - 1, child_first_index_of_lod);
				}
			}
		}
	}
}



static void tg__thread_fn(volatile tg_terrain* p_terrain)
{
	for (i8 z = -TG_TERRAIN_VIEW_DISTANCE_IN_OCTREES; z < TG_TERRAIN_VIEW_DISTANCE_IN_OCTREES + 1; z++)
	{
		for (i8 x = -TG_TERRAIN_VIEW_DISTANCE_IN_OCTREES; x < TG_TERRAIN_VIEW_DISTANCE_IN_OCTREES + 1; x++)
		{
			const u8 y = 0;
			const u8 octree_index = ((2 * TG_TERRAIN_VIEW_DISTANCE_IN_OCTREES) + 1) * (u8)(z + TG_TERRAIN_VIEW_DISTANCE_IN_OCTREES) + (u8)(x + TG_TERRAIN_VIEW_DISTANCE_IN_OCTREES);
			tg__build_octree((tg_terrain*)p_terrain, octree_index, x, y, z);
		}
	}

	TG_SEMAPHORE_WAIT(p_terrain->h_semaphore);

	for (u8 i = 0; i < TG_TERRAIN_OCTREES; i++)
	{
		volatile tg_terrain_octree* p_octree = &p_terrain->p_octrees[i];
		for (u32 j = 0; j < TG_TERRAIN_NODES_PER_OCTREE; j++)
		{
			volatile tg_terrain_octree_node* p_node = &p_octree->p_nodes[j];
			if (p_node->h_block_render_command)
			{
				tg_mesh_destroy(tg_render_command_get_mesh(p_node->h_block_render_command));
				tg_render_command_destroy(p_node->h_block_render_command);
			}

			for (u8 k = 0; k < 6; k++)
			{
				if (p_node->ph_transition_render_commands[k])
				{
					tg_mesh_destroy(tg_render_command_get_mesh(p_node->ph_transition_render_commands[k]));
					tg_render_command_destroy(p_node->ph_transition_render_commands[k]);
				}
			}
		}
	}
}

tg_terrain* tg_terrain_create(tg_camera* p_camera)
{
	tg_terrain* p_terrain = TG_MEMORY_ALLOC_NULLIFY(sizeof(*p_terrain));
	p_terrain->p_camera = p_camera;
	p_terrain->h_material = tg_material_create_deferred(tg_vertex_shader_get("shaders/deferred/terrain.vert"), tg_fragment_shader_get("shaders/deferred/terrain.frag"));
	p_terrain->h_semaphore = TG_SEMAPHORE_CREATE(0, 1);
	p_terrain->h_thread = tg_thread_create(tg__thread_fn, p_terrain);

	return p_terrain;
}

void tg_terrain_destroy(tg_terrain* p_terrain)
{
	TG_ASSERT(p_terrain);

	TG_SEMAPHORE_RELEASE(p_terrain->h_semaphore);
	tg_thread_destroy(p_terrain->h_thread);
	TG_SEMAPHORE_DESTROY(p_terrain->h_semaphore);
	tg_material_destroy(p_terrain->h_material);

	TG_MEMORY_FREE(p_terrain);
}

void tg_terrain_update(tg_terrain* p_terrain)
{
	TG_ASSERT(p_terrain);

	for (u8 i = 0; i < TG_TERRAIN_OCTREES; i++)
	{
		tg__update(p_terrain, i, 0, 0, 0, TG_TERRAIN_MAX_LOD, 0);
	}
}

void tg_terrain_render(tg_terrain* p_terrain, tg_renderer_h h_renderer)
{
	for (u8 i = 0; i < TG_TERRAIN_OCTREES; i++)
	{
		tg__render(p_terrain, i, 0, 0, 0, TG_TERRAIN_MAX_LOD, 0, h_renderer);
	}
}
