#include "tg_transvoxel_terrain.h"

#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "tg_transvoxel_lookup_tables.h"
#include "util/tg_string.h"



#define TG_CELLS_PER_BLOCK_SIDE                  16
#define TG_CELLS_PER_BLOCK                       4096 // 16^3
#define TG_VOXELS_PER_BLOCK_SIDE                 17
#define TG_OCTREE_STRIDE_IN_CELLS                256 // 16 * 16
#define TG_VOXEL_MAP_STRIDE                      257
#define TG_VOXEL_MAP_VOXELS                      16974593 // 257^3
#define TG_VOXEL_MAP_AT(voxel_map, x, y, z)      ((voxel_map)[66049 * (z) + 257 * (y) + (x)]) // 257 * 257 * z + 257 * y + x
#define TG_VOXEL_MAP_AT_V3I(voxel_map, v)        TG_VOXEL_MAP_AT(voxel_map, (v).x, (v).y, (v).z)
#define TG_VIEW_DISTANCE_IN_OCTREES              1



typedef enum tg_flag
{
	TG_TRANSVOXEL_FLAG_LEAF    = 0x1
} tg_flag;

typedef enum tg_full_resolution_face
{
	TG_FULL_RESOLUTION_FACE_NEGATIVE_X    = 0,
	TG_FULL_RESOLUTION_FACE_POSITIVE_X    = 1,
	TG_FULL_RESOLUTION_FACE_NEGATIVE_Y    = 2,
	TG_FULL_RESOLUTION_FACE_POSITIVE_Y    = 3,
	TG_FULL_RESOLUTION_FACE_NEGATIVE_Z    = 4,
	TG_FULL_RESOLUTION_FACE_POSITIVE_Z    = 5
} tg_full_resolution_face;



typedef struct tg_transvoxel_vertex
{
	v3    position;
	v3    normal;
} tg_transvoxel_vertex;





static void tg__fill_voxel_map(v3i octree_min_coordinates, i8* p_voxel_map_buffer)
{
	for (i32 z = 0; z < TG_VOXEL_MAP_STRIDE; z++)
	{
		for (i32 y = 0; y < TG_VOXEL_MAP_STRIDE; y++)
		{
			for (i32 x = 0; x < TG_VOXEL_MAP_STRIDE; x++)
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
				TG_VOXEL_MAP_AT(p_voxel_map_buffer, x, y, z) = f2;
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
	const i8* p_itvm_end = &p_itvm[TG_VOXEL_MAP_VOXELS];
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





static u8 tg__determine_max_lod()
{
	u8 max_lod = 0;
	u8 cell_count = TG_CELLS_PER_BLOCK_SIDE;
	while (cell_count != 1)
	{
		max_lod++;
		cell_count >>= 1;
	}
	return max_lod;
}

static v3 tg__block_center(v3i absolute_min_coordinates, u8 lod)
{
	const i32 half_stride = TG_CELLS_PER_BLOCK_SIDE * (1 << (lod - 1)); // 2^lod / 2
	const v3 center = tgm_v3i_to_v3(tgm_v3i_addi(absolute_min_coordinates, half_stride));
	return center;
}

static b32 tg__should_split_node(v3 camera_position, v3i absolute_min_coordinates, u8 lod)
{
	if (lod == 0)
	{
		return TG_FALSE;
	}
	const v3 center = tg__block_center(absolute_min_coordinates, lod);
	const v3 direction = tgm_v3_sub(center, camera_position);
	const f32 distance = tgm_v3_mag(direction);
	const f32 scaled_distance = distance / (1 << lod);
	const b32 result = scaled_distance < 2 * TG_CELLS_PER_BLOCK_SIDE;
	return result;
}

static u8 tg__get_transition_mask(v3 camera_position, v3i octree_min_coordinates, v3i block_offset_in_octree, u8 lod)
{
	u8 transition_mask = 0;

	if (lod != 0)
	{
		const i32 stride = TG_CELLS_PER_BLOCK_SIDE * (1 << lod);

		const v3i off0 = tgm_v3i_sub(block_offset_in_octree, (v3i) { stride, 0, 0 });
		const v3i off1 = tgm_v3i_add(block_offset_in_octree, (v3i) { stride, 0, 0 });
		const v3i off2 = tgm_v3i_sub(block_offset_in_octree, (v3i) { 0, stride, 0 });
		const v3i off3 = tgm_v3i_add(block_offset_in_octree, (v3i) { 0, stride, 0 });
		const v3i off4 = tgm_v3i_sub(block_offset_in_octree, (v3i) { 0, 0, stride });
		const v3i off5 = tgm_v3i_add(block_offset_in_octree, (v3i) { 0, 0, stride });

		const b32 s0 = tg__should_split_node(camera_position, tgm_v3i_add(octree_min_coordinates, off0), lod);
		const b32 s1 = tg__should_split_node(camera_position, tgm_v3i_add(octree_min_coordinates, off1), lod);
		const b32 s2 = tg__should_split_node(camera_position, tgm_v3i_add(octree_min_coordinates, off2), lod);
		const b32 s3 = tg__should_split_node(camera_position, tgm_v3i_add(octree_min_coordinates, off3), lod);
		const b32 s4 = tg__should_split_node(camera_position, tgm_v3i_add(octree_min_coordinates, off4), lod);
		const b32 s5 = tg__should_split_node(camera_position, tgm_v3i_add(octree_min_coordinates, off5), lod);

		transition_mask |= s0;
		transition_mask |= s1 << 1;
		transition_mask |= s2 << 2;
		transition_mask |= s3 << 3;
		transition_mask |= s4 << 4;
		transition_mask |= s5 << 5;
	}

	return transition_mask;
}

static v3 tg__normalized_not_null(v3 v)
{
	const f32 magsqr = tgm_v3_magsqr(v);
	v3 result = { 0.0f, 1.0f, 0.0f };
	if (magsqr != 0.0f)
	{
		const f32 mag = tgm_f32_sqrt(magsqr);
		result = (v3) { v.x / mag, v.y / mag, v.z / mag };
	}
	return result;
}

static v3i tg__transition_face_to_block(v3i pad, u8 half_resolution_lod, i32 x, i32 y, u8 direction)
{
	TG_ASSERT(half_resolution_lod > 0);

	const u8 full_res_scale = 1 << (half_resolution_lod - 1);

	const i32 x0 = full_res_scale * x;
	const i32 y0 = full_res_scale * y;
	const i32 z0 = 0;
	const i32 z1 = (1 << half_resolution_lod) * (TG_VOXELS_PER_BLOCK_SIDE - 1);

	switch (direction)
	{
	case TG_FULL_RESOLUTION_FACE_NEGATIVE_X: return tgm_v3i_add(pad, (v3i) { z0, x0, y0 });
	case TG_FULL_RESOLUTION_FACE_POSITIVE_X: return tgm_v3i_add(pad, (v3i) { z1, y0, x0 });
	case TG_FULL_RESOLUTION_FACE_NEGATIVE_Y: return tgm_v3i_add(pad, (v3i) { y0, z0, x0 });
	case TG_FULL_RESOLUTION_FACE_POSITIVE_Y: return tgm_v3i_add(pad, (v3i) { x0, z1, y0 });
	case TG_FULL_RESOLUTION_FACE_NEGATIVE_Z: return tgm_v3i_add(pad, (v3i) { x0, y0, z0 });
	case TG_FULL_RESOLUTION_FACE_POSITIVE_Z: return tgm_v3i_add(pad, (v3i) { y0, x0, z1 });

	default: TG_INVALID_CODEPATH(); return (v3i ){ 0 };
	}
}



static void tg__build_block(v3i octree_min_coordinates, v3i block_offset_in_octree, u8 lod, const i8* p_voxel_map, u16* p_vertex_count, tg_transvoxel_vertex* p_vertex_buffer)
{
	const i32 scaled_block_stride_in_cells = TG_CELLS_PER_BLOCK_SIDE << lod;
	const i32 lod_scale = 1 << lod;

	v3i p_sample_positions[8] = { 0 };
	i8 p_cell_samples[8] = { 0 };
	v3i p_corner_positions[8] = { 0 };

	v3i position = { 0 };
	for (position.z = 0; position.z < TG_CELLS_PER_BLOCK_SIDE; position.z++)
	{
		for (position.y = 0; position.y < TG_CELLS_PER_BLOCK_SIDE; position.y++)
		{
			for (position.x = 0; position.x < TG_CELLS_PER_BLOCK_SIDE; position.x++)
			{
				const v3i sample_position_pad = tgm_v3i_add(block_offset_in_octree, tgm_v3i_muli(position, lod_scale));
				p_sample_positions[0] = tgm_v3i_add(sample_position_pad, (v3i) {         0,         0,         0 });
				p_sample_positions[1] = tgm_v3i_add(sample_position_pad, (v3i) { lod_scale,         0,         0 });
				p_sample_positions[2] = tgm_v3i_add(sample_position_pad, (v3i) {         0, lod_scale,         0 });
				p_sample_positions[3] = tgm_v3i_add(sample_position_pad, (v3i) { lod_scale, lod_scale,         0 });
				p_sample_positions[4] = tgm_v3i_add(sample_position_pad, (v3i) {         0,         0, lod_scale });
				p_sample_positions[5] = tgm_v3i_add(sample_position_pad, (v3i) { lod_scale,         0, lod_scale });
				p_sample_positions[6] = tgm_v3i_add(sample_position_pad, (v3i) {         0, lod_scale, lod_scale });
				p_sample_positions[7] = tgm_v3i_add(sample_position_pad, (v3i) { lod_scale, lod_scale, lod_scale });

				p_cell_samples[0] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0]);
				p_cell_samples[1] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[1]);
				p_cell_samples[2] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[2]);
				p_cell_samples[3] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[3]);
				p_cell_samples[4] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[4]);
				p_cell_samples[5] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[5]);
				p_cell_samples[6] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[6]);
				p_cell_samples[7] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[7]);

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
				
				const v3i cell_position_pad = tgm_v3i_add(octree_min_coordinates, block_offset_in_octree);
				const v3i cell_position_base = tgm_v3i_add(cell_position_pad, tgm_v3i_muli(position, lod_scale));
				p_corner_positions[0] = tgm_v3i_add(cell_position_base, (v3i) {         0,         0,         0 });
				p_corner_positions[1] = tgm_v3i_add(cell_position_base, (v3i) { lod_scale,         0,         0 });
				p_corner_positions[2] = tgm_v3i_add(cell_position_base, (v3i) {         0, lod_scale,         0 });
				p_corner_positions[3] = tgm_v3i_add(cell_position_base, (v3i) { lod_scale, lod_scale,         0 });
				p_corner_positions[4] = tgm_v3i_add(cell_position_base, (v3i) {         0,         0, lod_scale });
				p_corner_positions[5] = tgm_v3i_add(cell_position_base, (v3i) { lod_scale,         0, lod_scale });
				p_corner_positions[6] = tgm_v3i_add(cell_position_base, (v3i) {         0, lod_scale, lod_scale });
				p_corner_positions[7] = tgm_v3i_add(cell_position_base, (v3i) { lod_scale, lod_scale, lod_scale });

				const u8 regular_cell_class_index = p_regular_cell_class[case_code];
				const tg_regular_cell_data* p_cell_data = &p_regular_cell_data[regular_cell_class_index];
				const u8 triangle_count = TG_TRANSVOXEL_CELL_GET_TRIANGLE_COUNT(*p_cell_data);

				for (u8 i = 0; i < triangle_count; i++)
				{
					for (u8 j = 0; j < 3; j++)
					{
						const u16 edge_code = p_regular_vertex_data[case_code][p_cell_data->p_vertex_indices[3 * i + j]];
						const u8 edge_code_low = edge_code & 0xff;
						const u8 edge_code_high = (edge_code >> 8) & 0xff;

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
							const i8 imid = TG_VOXEL_MAP_AT_V3I(p_voxel_map, midpoint);

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

						v3 q = { 0 };
						if (t & 0xff)
						{
							q = tgm_v3_divf(tgm_v3i_to_v3(tgm_v3i_add(tgm_v3i_muli(p0, ti0), tgm_v3i_muli(p1, ti1))), 256.0f);
						}
						else
						{
							q = tgm_v3i_to_v3(t == 0 ? p1 : p0);
						}
						p_vertex_buffer[j].position = q;
					}

					const v3 v01 = tgm_v3_sub(p_vertex_buffer[1].position, p_vertex_buffer[0].position);
					const v3 v02 = tgm_v3_sub(p_vertex_buffer[2].position, p_vertex_buffer[0].position);
					const v3 normal = tg__normalized_not_null(tgm_v3_cross(v01, v02));

					p_vertex_buffer[0].normal = normal;
					p_vertex_buffer[1].normal = normal;
					p_vertex_buffer[2].normal = normal;

					p_vertex_buffer += 3;
					*p_vertex_count += 3;
				}
			}
		}
	}
}

static void tg__build_transition(v3i octree_min_coordinates, v3i block_offset_in_octree, u8 lod, const i8* p_voxel_map, u8 direction, u16* p_vertex_count, tg_transvoxel_vertex* p_vertex_buffer)
{
	TG_ASSERT(lod > 0);

	const i32 scaled_block_stride_in_cells = TG_CELLS_PER_BLOCK_SIDE << lod;
	const i32 low_res_lod_scale = 1 << lod;
	const i32 high_res_lod_scale = 1 << (lod - 1);

	v3i p_sample_positions[13] = { 0 };
	i8 p_cell_samples[13] = { 0 };
	v3i p_cell_positions[13] = { 0 };

	for (u8 y = 0; y < TG_CELLS_PER_BLOCK_SIDE; y++)
	{
		const u8 yd = 2 * y;

		for (u8 x = 0; x < TG_CELLS_PER_BLOCK_SIDE; x++)
		{
			const u8 xd = 2 * x;

			p_sample_positions[0x0] = tg__transition_face_to_block(block_offset_in_octree, lod, xd    , yd    , direction);
			p_sample_positions[0x1] = tg__transition_face_to_block(block_offset_in_octree, lod, xd + 1, yd    , direction);
			p_sample_positions[0x2] = tg__transition_face_to_block(block_offset_in_octree, lod, xd + 2, yd    , direction);
			p_sample_positions[0x3] = tg__transition_face_to_block(block_offset_in_octree, lod, xd    , yd + 1, direction);
			p_sample_positions[0x4] = tg__transition_face_to_block(block_offset_in_octree, lod, xd + 1, yd + 1, direction);
			p_sample_positions[0x5] = tg__transition_face_to_block(block_offset_in_octree, lod, xd + 2, yd + 1, direction);
			p_sample_positions[0x6] = tg__transition_face_to_block(block_offset_in_octree, lod, xd    , yd + 2, direction);
			p_sample_positions[0x7] = tg__transition_face_to_block(block_offset_in_octree, lod, xd + 1, yd + 2, direction);
			p_sample_positions[0x8] = tg__transition_face_to_block(block_offset_in_octree, lod, xd + 2, yd + 2, direction);
			p_sample_positions[0x9] = p_sample_positions[0x0];
			p_sample_positions[0xa] = p_sample_positions[0x2];
			p_sample_positions[0xb] = p_sample_positions[0x6];
			p_sample_positions[0xc] = p_sample_positions[0x8];

			p_cell_samples[0x0] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x0]);
			p_cell_samples[0x1] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x1]);
			p_cell_samples[0x2] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x2]);
			p_cell_samples[0x3] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x3]);
			p_cell_samples[0x4] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x4]);
			p_cell_samples[0x5] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x5]);
			p_cell_samples[0x6] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x6]);
			p_cell_samples[0x7] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x7]);
			p_cell_samples[0x8] = TG_VOXEL_MAP_AT_V3I(p_voxel_map, p_sample_positions[0x8]);
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
			p_cell_positions[0x0] = tg__transition_face_to_block(cell_position_pad, lod, xd    , yd    , direction);
			p_cell_positions[0x1] = tg__transition_face_to_block(cell_position_pad, lod, xd + 1, yd    , direction);
			p_cell_positions[0x2] = tg__transition_face_to_block(cell_position_pad, lod, xd + 2, yd    , direction);
			p_cell_positions[0x3] = tg__transition_face_to_block(cell_position_pad, lod, xd    , yd + 1, direction);
			p_cell_positions[0x4] = tg__transition_face_to_block(cell_position_pad, lod, xd + 1, yd + 1, direction);
			p_cell_positions[0x5] = tg__transition_face_to_block(cell_position_pad, lod, xd + 2, yd + 1, direction);
			p_cell_positions[0x6] = tg__transition_face_to_block(cell_position_pad, lod, xd    , yd + 2, direction);
			p_cell_positions[0x7] = tg__transition_face_to_block(cell_position_pad, lod, xd + 1, yd + 2, direction);
			p_cell_positions[0x8] = tg__transition_face_to_block(cell_position_pad, lod, xd + 2, yd + 2, direction);
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
						const i8 d = TG_VOXEL_MAP_AT_V3I(p_voxel_map, midpoint);

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
					const i32 ti0 = t;
					const i32 ti1 = 0x100 - t;

					v3 q = { 0 };
					if (t & 0xff)
					{
						q = tgm_v3_divf(tgm_v3i_to_v3(tgm_v3i_add(tgm_v3i_muli(p0, ti0), tgm_v3i_muli(p1, ti1))), 256.0f);
					}
					else
					{
						q = tgm_v3i_to_v3(t == 0 ? p1 : p0);
					}
					p_vertex_buffer[j].position = q;
				}

				if (invert)
				{
					const v3 temp = p_vertex_buffer[2].position;
					p_vertex_buffer[2].position = p_vertex_buffer[1].position;
					p_vertex_buffer[1].position = temp;
				}

				const v3 v01 = tgm_v3_sub(p_vertex_buffer[1].position, p_vertex_buffer[0].position);
				const v3 v02 = tgm_v3_sub(p_vertex_buffer[2].position, p_vertex_buffer[0].position);
				const v3 normal = tg__normalized_not_null(tgm_v3_cross(v01, v02));

				p_vertex_buffer[0].normal = normal;
				p_vertex_buffer[1].normal = normal;
				p_vertex_buffer[2].normal = normal;

				p_vertex_buffer += 3;
				*p_vertex_count += 3;
			}
		}
	}
}

static void tg__build_node(tg_terrain* p_terrain, tg_terrain_octree* p_octree, tg_terrain_octree_node* p_node, v3i block_offset_in_octree, u8 lod)
{
	const u64 vertex_buffer_size = 15 * TG_CELLS_PER_BLOCK * sizeof(tg_transvoxel_vertex);
	tg_transvoxel_vertex* p_vertex_buffer = TG_MEMORY_STACK_ALLOC(vertex_buffer_size);

	p_node->flags |= TG_TRANSVOXEL_FLAG_LEAF;

	u16 vertex_count = 0;
	tg__build_block(p_octree->min_coordinates, block_offset_in_octree, lod, p_octree->p_voxel_map, &vertex_count, p_vertex_buffer);

	tg_terrain_block* p_block = &p_node->block;

	p_block->transition_mask = tg__get_transition_mask(p_terrain->p_camera->position, p_octree->min_coordinates, block_offset_in_octree, lod);

	if (vertex_count)
	{
		p_block->h_block_mesh = tg_mesh_create2(vertex_count, sizeof(tg_transvoxel_vertex), p_vertex_buffer, 0, TG_NULL);
		p_block->h_block_render_command = tg_render_command_create(p_block->h_block_mesh, p_terrain->h_material, V3(0), 0, TG_NULL);
	}

	if (lod > 0)
	{
		for (u8 i = 0; i < 6; i++)
		{
			vertex_count = 0;
			tg__build_transition(p_octree->min_coordinates, block_offset_in_octree, lod, p_octree->p_voxel_map, i, &vertex_count, p_vertex_buffer);

			if (vertex_count)
			{
				p_block->ph_transition_meshes[i] = tg_mesh_create2(vertex_count, sizeof(tg_transvoxel_vertex), p_vertex_buffer, 0, TG_NULL);
				p_block->ph_transition_render_commands[i] = tg_render_command_create(p_block->ph_transition_meshes[i], p_terrain->h_material, V3(0), 0, TG_NULL);
			}
		}
	}

	TG_MEMORY_STACK_FREE(vertex_buffer_size);
}

static void tg__build_node_children(tg_terrain* p_terrain, tg_terrain_octree* p_octree, tg_terrain_octree_node* p_node, v3i block_offset_in_octree, u8 lod)
{
	static void tg__build_nodes_recursively(tg_terrain * p_terrain, tg_terrain_octree * p_octree, tg_terrain_octree_node * p_node, v3i block_offset_in_octree, u8 lod);

	const i32 half_stride = TG_CELLS_PER_BLOCK_SIDE * (1 << (lod - 1)); // 2^lod / 2
	for (u8 z = 0; z < 2; z++)
	{
		for (u8 y = 0; y < 2; y++)
		{
			for (u8 x = 0; x < 2; x++)
			{
				const u8 i = 4 * z + 2 * y + x;
				if (!p_node->pp_children[i])
				{
					p_node->pp_children[i] = TG_MEMORY_ALLOC_NULLIFY(sizeof(tg_terrain_octree_node));
					const v3i child_block_offset_in_octree = tgm_v3i_add(block_offset_in_octree, tgm_v3i_muli((v3i) { x, y, z }, half_stride));
					tg__build_nodes_recursively(p_terrain, p_octree, p_node->pp_children[i], child_block_offset_in_octree, lod - 1);
				}
			}
		}
	}
}

static void tg__build_nodes_recursively(tg_terrain* p_terrain, tg_terrain_octree* p_octree, tg_terrain_octree_node* p_node, v3i block_offset_in_octree, u8 lod)
{
	tg_memory_nullify(sizeof(*p_node), p_node);

	if (tg__should_split_node(p_terrain->p_camera->position, tgm_v3i_add(p_octree->min_coordinates, block_offset_in_octree), lod))
	{
		tg__build_node_children(p_terrain, p_octree, p_node, block_offset_in_octree, lod);
	}
	else
	{
		tg__build_node(p_terrain, p_octree, p_node, block_offset_in_octree, lod);
	}
}

static void tg__build_octree(tg_terrain* p_terrain, tg_terrain_octree* p_octree, i32 x, i32 y, i32 z)
{
	p_octree->min_coordinates = (v3i) {
		x * TG_OCTREE_STRIDE_IN_CELLS,
		y * TG_OCTREE_STRIDE_IN_CELLS,
		z * TG_OCTREE_STRIDE_IN_CELLS
	};

	const u64 voxel_map_size = TG_VOXEL_MAP_VOXELS * sizeof(i8);
	p_octree->p_voxel_map = TG_MEMORY_ALLOC(voxel_map_size);

	char p_filename_buffer[TG_MAX_PATH] = { 0 };
	tg_string_format(sizeof(p_filename_buffer), p_filename_buffer, "terrain/octree_%i_%i_%i.txt", x, y, z);
	const b32 exists = tg_platform_file_exists(p_filename_buffer);
	if (!exists)
	{
		tg__fill_voxel_map(p_octree->min_coordinates, p_octree->p_voxel_map);

		u32 compressed_voxel_map_size;
		void* p_compressed_voxel_map_buffer = TG_MEMORY_STACK_ALLOC(voxel_map_size);
		tg__compress_voxel_map(p_octree->p_voxel_map, &compressed_voxel_map_size, p_compressed_voxel_map_buffer);

		tg_platform_create_file(p_filename_buffer, compressed_voxel_map_size, p_compressed_voxel_map_buffer, TG_FALSE);

		TG_MEMORY_STACK_FREE(voxel_map_size);
	}
	else
	{
		u32 size = 0;
		char* p_file_data = TG_NULL;
		tg_platform_read_file(p_filename_buffer, &size, &p_file_data);
		tg__decompress_voxel_map(p_file_data, p_octree->p_voxel_map);
		tg_platform_free_file(p_file_data);
	}

	p_octree->p_root = TG_MEMORY_ALLOC_NULLIFY(sizeof(*p_octree->p_root));
	tg__build_nodes_recursively(p_terrain, p_octree, p_octree->p_root, V3I(0), tg__determine_max_lod());
}

static void tg__destroy_nodes_recursively(tg_terrain_octree_node* p_node)
{
	if (p_node)
	{
		tg_terrain_block* p_block = &p_node->block;

		if (p_block->h_block_render_command)
		{
			tg_mesh_destroy(p_block->h_block_mesh);
			tg_render_command_destroy(p_block->h_block_render_command);
		}

		for (u8 i = 0; i < 6; i++)
		{
			if (p_block->ph_transition_render_commands[i])
			{
				tg_mesh_destroy(p_block->ph_transition_meshes[i]);
				tg_render_command_destroy(p_block->ph_transition_render_commands[i]);
			}
		}

		for (u8 i = 0; i < 8; i++)
		{
			if (p_node->pp_children[i])
			{
				tg__destroy_nodes_recursively(p_node->pp_children[i]);
			}
		}
		
		TG_MEMORY_FREE(p_node);
	}
}



static void tg__update(tg_terrain* p_terrain, tg_terrain_octree* p_octree, tg_terrain_octree_node* p_node, v3i block_offset_in_octree, u8 lod)
{
	if (p_node->flags & TG_TRANSVOXEL_FLAG_LEAF)
	{
		if (tg__should_split_node(p_terrain->p_camera->position, tgm_v3i_add(p_octree->min_coordinates, block_offset_in_octree), lod))
		{
			p_node->flags &= ~TG_TRANSVOXEL_FLAG_LEAF;
			tg__build_node_children(p_terrain, p_octree, p_node, block_offset_in_octree, lod);
		}
		else
		{
			p_node->block.transition_mask = tg__get_transition_mask(p_terrain->p_camera->position, p_octree->min_coordinates, block_offset_in_octree, lod);
		}
	}
	else
	{
		if (!tg__should_split_node(p_terrain->p_camera->position, tgm_v3i_add(p_octree->min_coordinates, block_offset_in_octree), lod))
		{
			if (!p_node->block.h_block_render_command)
			{
				tg__build_node(p_terrain, p_octree, p_node, block_offset_in_octree, lod);
			}
			else
			{
				p_node->flags |= TG_TRANSVOXEL_FLAG_LEAF;
				p_node->block.transition_mask = tg__get_transition_mask(p_terrain->p_camera->position, p_octree->min_coordinates, block_offset_in_octree, lod);
			}
		}
		else
		{
			const i32 half_stride = TG_CELLS_PER_BLOCK_SIDE * (1 << (lod - 1)); // 2^lod / 2
			for (u8 z = 0; z < 2; z++)
			{
				for (u8 y = 0; y < 2; y++)
				{
					for (u8 x = 0; x < 2; x++)
					{
						const u8 i = 4 * z + 2 * y + x;
						const v3i child_block_offset_in_octree = tgm_v3i_add(block_offset_in_octree, tgm_v3i_muli((v3i) { x, y, z }, half_stride));
						tg__update(p_terrain, p_octree, p_node->pp_children[i], child_block_offset_in_octree, lod - 1);
					}
				}
			}
		}
	}
}

static void tg__render(tg_terrain_octree_node* p_node, tg_renderer_h h_renderer)
{
	if (p_node->flags & TG_TRANSVOXEL_FLAG_LEAF)
	{
		if (p_node->block.h_block_render_command)
		{
			tg_renderer_execute(h_renderer, p_node->block.h_block_render_command);
		}
		for (u8 i = 0; i < 6; i++)
		{
			if ((p_node->block.transition_mask & (1 << i)) && p_node->block.ph_transition_render_commands[i])
			{
				tg_renderer_execute(h_renderer, p_node->block.ph_transition_render_commands[i]);
			}
		}
	}
	else
	{
		for (u8 i = 0; i < 8; i++)
		{
			if (p_node->pp_children[i])
			{
				tg__render(p_node->pp_children[i], h_renderer);
			}
		}
	}
}





tg_terrain tg_terrain_create(tg_camera* p_camera)
{
	tg_terrain terrain = { 0 };

	terrain.p_camera = p_camera;
	terrain.h_material = tg_material_create_deferred(tg_vertex_shader_get("shaders/deferred_flat_terrain.vert"), tg_fragment_shader_get("shaders/deferred_flat_terrain.frag"));

	for (i8 z = -TG_VIEW_DISTANCE_IN_OCTREES; z < TG_VIEW_DISTANCE_IN_OCTREES + 1; z++)
	{
		for (i8 x = -TG_VIEW_DISTANCE_IN_OCTREES; x < TG_VIEW_DISTANCE_IN_OCTREES + 1; x++)
		{
			const i8 y = 0;
			const u32 i = ((2 * TG_VIEW_DISTANCE_IN_OCTREES) + 1) * (z + TG_VIEW_DISTANCE_IN_OCTREES) + (x + TG_VIEW_DISTANCE_IN_OCTREES);

			terrain.pp_octrees[i] = TG_MEMORY_ALLOC(sizeof(*terrain.pp_octrees[i]));
			tg__build_octree(&terrain, terrain.pp_octrees[i], x, y, z);
		}
	}

	return terrain;
}

void tg_terrain_destroy(tg_terrain* p_terrain)
{
	TG_ASSERT(p_terrain);
	
	for (u8 i = 0; i < 9; i++)
	{
		tg_terrain_octree* p_octree = p_terrain->pp_octrees[i];
		if (p_octree)
		{
			tg__destroy_nodes_recursively(p_octree->p_root);
			TG_MEMORY_FREE(p_octree->p_voxel_map);
			TG_MEMORY_FREE(p_octree);
		}
	}
}

void tg_terrain_update(tg_terrain* p_terrain, f32 delta_ms)
{
	TG_ASSERT(p_terrain);

	const i32 half_stride = TG_CELLS_PER_BLOCK_SIDE * (1 << (tg__determine_max_lod() - 1));

	for (u8 i = 0; i < 9; i++)
	{
		tg_terrain_octree* p_octree = p_terrain->pp_octrees[i];
		if (p_octree)
		{
			tg_terrain_octree_node* p_root = p_octree->p_root;

			const f32 center_x = (f32)(p_octree->min_coordinates.x + half_stride);
			const f32 center_z = (f32)(p_octree->min_coordinates.z + half_stride);

			const f32 delta_x = p_terrain->p_camera->position.x - center_x;
			const f32 delta_z = p_terrain->p_camera->position.z - center_z;

			const f32 abs_delta_x = delta_x < 0 ? -delta_x : delta_x;
			const f32 abs_delta_z = delta_z < 0 ? -delta_z : delta_z;

			const b32 should_destroy_x = abs_delta_x > (f32)(TG_VIEW_DISTANCE_IN_OCTREES + 0.5f) * (f32)TG_OCTREE_STRIDE_IN_CELLS;
			const b32 should_destroy_z = abs_delta_z > (f32)(TG_VIEW_DISTANCE_IN_OCTREES + 0.5f) * (f32)TG_OCTREE_STRIDE_IN_CELLS;

			if (should_destroy_x || should_destroy_z)
			{
				tg__destroy_nodes_recursively(p_octree->p_root);
				TG_MEMORY_FREE(p_octree->p_voxel_map);
				TG_MEMORY_FREE(p_octree);
				p_terrain->pp_octrees[i] = TG_NULL;
			}
			else
			{
				tg__update(p_terrain, p_octree, p_octree->p_root, V3I(0), tg__determine_max_lod());
			}
		}
	}

	const i32 camera_octree_index_x = (i32)tgm_f32_floor(tgm_f32_floor(p_terrain->p_camera->position.x) / (f32)TG_OCTREE_STRIDE_IN_CELLS);
	const i32 camera_octree_index_z = (i32)tgm_f32_floor(tgm_f32_floor(p_terrain->p_camera->position.z) / (f32)TG_OCTREE_STRIDE_IN_CELLS);

	for (i8 relz = -TG_VIEW_DISTANCE_IN_OCTREES; relz < TG_VIEW_DISTANCE_IN_OCTREES + 1; relz++)
	{
		const i8 z = camera_octree_index_z + relz;
		for (i8 relx = -TG_VIEW_DISTANCE_IN_OCTREES; relx < TG_VIEW_DISTANCE_IN_OCTREES + 1; relx++)
		{
			const i8 x = camera_octree_index_x + relx;
			const v3i omc = tgm_v3i_muli((v3i) { x, 0, z }, TG_OCTREE_STRIDE_IN_CELLS);

			b32 found = TG_FALSE;
			i32 slot_index = -1;
			for (u8 i = 0; i < 9; i++)
			{
				if (p_terrain->pp_octrees[i] == TG_NULL)
				{
					slot_index = i;
				}
				else if (tgm_v3i_equal(p_terrain->pp_octrees[i]->min_coordinates, omc))
				{
					found = TG_TRUE;
					break;
				}
			}

			if (!found && slot_index != -1)
			{
				p_terrain->pp_octrees[slot_index] = TG_MEMORY_ALLOC(sizeof(*p_terrain->pp_octrees[slot_index]));
				tg__build_octree(p_terrain, p_terrain->pp_octrees[slot_index], x, 0, z);
			}
		}
	}
}

void tg_terrain_render(tg_terrain* p_terrain, tg_renderer_h h_renderer)
{
	for (u32 i = 0; i < 9; i++)
	{
		tg_terrain_octree* p_octree = p_terrain->pp_octrees[i];
		if (p_octree)
		{
			tg__render(p_octree->p_root, h_renderer);
		}
	}
}
