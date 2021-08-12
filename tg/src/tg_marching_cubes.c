#include "tg_marching_cubes.h"

#include "tg_transvoxel_lookup_tables.h"



#define TG_TERRAIN_VOXEL_MAP_AT(p_voxel_map, voxel_map_size, sample_x, sample_y, sample_z) \
	(p_voxel_map)[(sample_z) * (voxel_map_size).x * (voxel_map_size).y + (sample_y) * (voxel_map_size).x + (sample_x)]



u32 tg_marching_cubes_create_mesh(v3i voxel_map_size, const i8* p_voxel_map, v3 scale, u32 vertex_buffer_size, u32 vertex_position_offset, u32 vertex_stride, f32* p_vertex_buffer)
{
	TG_ASSERT(voxel_map_size.x && voxel_map_size.y && voxel_map_size.z);
	TG_ASSERT(p_voxel_map);
	TG_ASSERT(scale.x && scale.y && scale.z);
	TG_ASSERT(vertex_buffer_size);
	TG_ASSERT(vertex_position_offset % 4 == 0);
	TG_ASSERT(vertex_stride);
	TG_ASSERT(p_vertex_buffer);

	u32 vertex_count = 0;

	p_vertex_buffer = (f32*)&((u8*)p_vertex_buffer)[vertex_position_offset];
	const v3 center = tgm_v3_divf(tgm_v3_mul(tgm_v3i_to_v3(voxel_map_size), scale), 2.0f);

	v3 p_positions[8] = { 0 };
	i8 p_samples[8] = { 0 };

	v3i position = { 0 };
	for (position.z = 0; position.z < voxel_map_size.z - 1; position.z++)
	{
		for (position.y = 0; position.y < voxel_map_size.y - 1; position.y++)
		{
			for (position.x = 0; position.x < voxel_map_size.x - 1; position.x++)
			{
				const v3 position_pad = tgm_v3_sub(tgm_v3_mul(tgm_v3i_to_v3(position), scale), center);
				p_positions[0] = tgm_v3_add(position_pad, (v3) {    0.0f,    0.0f,    0.0f });
				p_positions[1] = tgm_v3_add(position_pad, (v3) { scale.x,    0.0f,    0.0f });
				p_positions[2] = tgm_v3_add(position_pad, (v3) {    0.0f, scale.y,    0.0f });
				p_positions[3] = tgm_v3_add(position_pad, (v3) { scale.x, scale.y,    0.0f });
				p_positions[4] = tgm_v3_add(position_pad, (v3) {    0.0f,    0.0f, scale.z });
				p_positions[5] = tgm_v3_add(position_pad, (v3) { scale.x,    0.0f, scale.z });
				p_positions[6] = tgm_v3_add(position_pad, (v3) {    0.0f, scale.y, scale.z });
				p_positions[7] = tgm_v3_add(position_pad, (v3) { scale.x, scale.y, scale.z });

				p_samples[0] = TG_TERRAIN_VOXEL_MAP_AT(p_voxel_map, voxel_map_size, position.x    , position.y    , position.z    );
				p_samples[1] = TG_TERRAIN_VOXEL_MAP_AT(p_voxel_map, voxel_map_size, position.x + 1, position.y    , position.z    );
				p_samples[2] = TG_TERRAIN_VOXEL_MAP_AT(p_voxel_map, voxel_map_size, position.x    , position.y + 1, position.z    );
				p_samples[3] = TG_TERRAIN_VOXEL_MAP_AT(p_voxel_map, voxel_map_size, position.x + 1, position.y + 1, position.z    );
				p_samples[4] = TG_TERRAIN_VOXEL_MAP_AT(p_voxel_map, voxel_map_size, position.x    , position.y    , position.z + 1);
				p_samples[5] = TG_TERRAIN_VOXEL_MAP_AT(p_voxel_map, voxel_map_size, position.x + 1, position.y    , position.z + 1);
				p_samples[6] = TG_TERRAIN_VOXEL_MAP_AT(p_voxel_map, voxel_map_size, position.x    , position.y + 1, position.z + 1);
				p_samples[7] = TG_TERRAIN_VOXEL_MAP_AT(p_voxel_map, voxel_map_size, position.x + 1, position.y + 1, position.z + 1);

				const u8 case_code =
					((p_samples[0] >> 7) & 0x01) |
					((p_samples[1] >> 6) & 0x02) |
					((p_samples[2] >> 5) & 0x04) |
					((p_samples[3] >> 4) & 0x08) |
					((p_samples[4] >> 3) & 0x10) |
					((p_samples[5] >> 2) & 0x20) |
					((p_samples[6] >> 1) & 0x40) |
					((p_samples[7] >> 0) & 0x80);

				TG_ASSERT(case_code != 256);

				if (case_code == 0 || case_code == 255)
				{
					continue;
				}

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

						i32 d0 = p_samples[v0];
						i32 d1 = p_samples[v1];

						v3 p0 = p_positions[v0];
						v3 p1 = p_positions[v1];

						TG_ASSERT(d0 != d1);

						const i32 t = (d1 << 8) / (d1 - d0);
						const f32 t0 = (f32)t / 256.0f;
						const f32 t1 = (f32)(0x100 - t) / 256.0f;

						v3 q = { 0 };
						if (t & 0xff)
						{
							q = tgm_v3_add(tgm_v3_mulf(p0, t0), tgm_v3_mulf(p1, t1));
						}
						else
						{
							q = t == 0 ? p1 : p0;
						}
						
						for (u8 k = 0; k < 3; k++)
						{
							p_vertex_buffer[k] = q.p_data[k];
						}
						p_vertex_buffer = (f32*)&((u8*)p_vertex_buffer)[vertex_stride];
						vertex_count++;
					}
				}
			}
		}
	}

	return vertex_count;
}
