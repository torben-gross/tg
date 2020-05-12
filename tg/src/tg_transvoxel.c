#include "tg_transvoxel.h"
#include "tg_transvoxel_lookup_tables.h"

v3 tg_transvoxel_internal_interpolate(i32 d0, i32 d1, const v3* p_p0, const v3* p_p1)
{
	const i32 t = (d1 << 8) / (d1 - d0);

	v3 q = { 0 };
	if ((t & 0x00ff) != 0)
	{
		// Vertex lies in the interior of the edge.
		const i32 u = 0x0100 - t;
		q.x = ((f32)t * p_p0->x + (f32)u * p_p1->x) / 256.0f;
		q.y = ((f32)t * p_p0->y + (f32)u * p_p1->y) / 256.0f;
		q.z = ((f32)t * p_p0->z + (f32)u * p_p1->z) / 256.0f;
	}
	else if (t == 0)
	{
		// Vertex lies at the higher-numbered endpoint.
		q = *p_p0;
	}
	else
	{
		// Vertex lies at the lower-numbered endpoint.
		q = *p_p1;
	}

	return q;
}

u32 tg_transvoxel_create_chunk(const tg_transvoxel_isolevels* p_isolevels, i32 x, i32 y, i32 z, tg_transvoxel_triangle* p_triangles)
{
	u32 chunk_triangle_count = 0;

	i8 p_corners[8] = { 0 };
	v3 p_positions[8] = { 0 };
	
	for (u32 cell_z = 0; cell_z < 16; cell_z++)
	{
		for (u32 cell_y = 0; cell_y < 16; cell_y++)
		{
			for (u32 cell_x = 0; cell_x < 16; cell_x++)
			{
				// Figure 3.7. Voxels at the corners of a cell are numbered as shown.
				//    2 _____________ 3
				//     /|           /|
				//    / |          / |
				// 6 /__|_________/7 |
				//  |   |        |   |
				//  |  0|________|___|1
				//  |  /         |  /
				//  | /          | /
				//  |/___________|/
				// 4              5

				p_corners[0] = TG_ISOLEVEL_AT(*p_isolevels, 1 + cell_x    , 1 + cell_y    , 1 + cell_z    );
				p_corners[1] = TG_ISOLEVEL_AT(*p_isolevels, 1 + cell_x + 1, 1 + cell_y    , 1 + cell_z    );
				p_corners[2] = TG_ISOLEVEL_AT(*p_isolevels, 1 + cell_x    , 1 + cell_y + 1, 1 + cell_z    );
				p_corners[3] = TG_ISOLEVEL_AT(*p_isolevels, 1 + cell_x + 1, 1 + cell_y + 1, 1 + cell_z    );
				p_corners[4] = TG_ISOLEVEL_AT(*p_isolevels, 1 + cell_x    , 1 + cell_y    , 1 + cell_z + 1);
				p_corners[5] = TG_ISOLEVEL_AT(*p_isolevels, 1 + cell_x + 1, 1 + cell_y    , 1 + cell_z + 1);
				p_corners[6] = TG_ISOLEVEL_AT(*p_isolevels, 1 + cell_x    , 1 + cell_y + 1, 1 + cell_z + 1);
				p_corners[7] = TG_ISOLEVEL_AT(*p_isolevels, 1 + cell_x + 1, 1 + cell_y + 1, 1 + cell_z + 1);

				const u32 case_code =
					((p_corners[0] >> 7) & 0x01) |
					((p_corners[1] >> 6) & 0x02) |
					((p_corners[2] >> 5) & 0x04) |
					((p_corners[3] >> 4) & 0x08) |
					((p_corners[4] >> 3) & 0x10) |
					((p_corners[5] >> 2) & 0x20) |
					((p_corners[6] >> 1) & 0x40) |
					((p_corners[7]     ) & 0x80);

				if ((case_code ^ ((p_corners[7] >> 7) & 0xff)) != 0)
				{
					// Cell has a nontrivial triangulation.

					const u8 equivalence_class_index = p_regular_cell_class[case_code];
					const tg_regular_cell_data* p_cell_data = &p_regular_cell_data[equivalence_class_index];
					const u16* p_vertex_data = p_regular_vertex_data[case_code];

					i32 vertex_count = TG_CELL_GET_VERTEX_COUNT(*p_cell_data);
					i32 cell_triangle_count = TG_CELL_GET_TRIANGLE_COUNT(*p_cell_data);

					const f32 offset_x = 16.0f * (f32)x;
					const f32 offset_y = 16.0f * (f32)y;
					const f32 offset_z = 16.0f * (f32)z;

					p_positions[0] = (v3){ offset_x + (f32)cell_x       , offset_y + (f32)cell_y       , offset_z + (f32)cell_z        };
					p_positions[1] = (v3){ offset_x + (f32)cell_x + 1.0f, offset_y + (f32)cell_y       , offset_z + (f32)cell_z        };
					p_positions[2] = (v3){ offset_x + (f32)cell_x       , offset_y + (f32)cell_y + 1.0f, offset_z + (f32)cell_z        };
					p_positions[3] = (v3){ offset_x + (f32)cell_x + 1.0f, offset_y + (f32)cell_y + 1.0f, offset_z + (f32)cell_z        };
					p_positions[4] = (v3){ offset_x + (f32)cell_x       , offset_y + (f32)cell_y       , offset_z + (f32)cell_z + 1.0f };
					p_positions[5] = (v3){ offset_x + (f32)cell_x + 1.0f, offset_y + (f32)cell_y       , offset_z + (f32)cell_z + 1.0f };
					p_positions[6] = (v3){ offset_x + (f32)cell_x       , offset_y + (f32)cell_y + 1.0f, offset_z + (f32)cell_z + 1.0f };
					p_positions[7] = (v3){ offset_x + (f32)cell_x + 1.0f, offset_y + (f32)cell_y + 1.0f, offset_z + (f32)cell_z + 1.0f };

					for (u8 i = 0; i < cell_triangle_count; i++)
					{
						const u8 i0 = p_cell_data->vertex_indices[3 * i + 0];
						const u8 i1 = p_cell_data->vertex_indices[3 * i + 1];
						const u8 i2 = p_cell_data->vertex_indices[3 * i + 2];

						const u16 e0 = p_vertex_data[i0];
						const u16 e1 = p_vertex_data[i1];
						const u16 e2 = p_vertex_data[i2];

						const u16 e0v0 = (e0 >> 4) & 0x0f;
						const u16 e0v1 = e0 & 0x0f;
						const u16 e1v0 = (e1 >> 4) & 0x0f;
						const u16 e1v1 = e1 & 0x0f;
						const u16 e2v0 = (e2 >> 4) & 0x0f;
						const u16 e2v1 = e2 & 0x0f;

						const v3 v0 = tg_transvoxel_internal_interpolate(p_corners[e0v0], p_corners[e0v1], &p_positions[e0v0], &p_positions[e0v1]);
						const v3 v1 = tg_transvoxel_internal_interpolate(p_corners[e1v0], p_corners[e1v1], &p_positions[e1v0], &p_positions[e1v1]);
						const v3 v2 = tg_transvoxel_internal_interpolate(p_corners[e2v0], p_corners[e2v1], &p_positions[e2v0], &p_positions[e2v1]);

						p_triangles[chunk_triangle_count].positions[0] = v0;
						p_triangles[chunk_triangle_count].positions[1] = v1;
						p_triangles[chunk_triangle_count].positions[2] = v2;

						chunk_triangle_count++;
					}
				}
			}
		}
	}

	return chunk_triangle_count;
}

void tg_transvoxel_fill_isolevels(tg_transvoxel_isolevels* p_isolevels, i32 x, i32 y, i32 z)
{
	const f32 offset_x = 16.0f * (f32)x;
	const f32 offset_y = 16.0f * (f32)y;
	const f32 offset_z = 16.0f * (f32)z;

	for (u32 i = 0; i < 19; i++)
	{
		for (u32 j = 0; j < 19; j++)
		{
			for (u32 k = 0; k < 19; k++)
			{
				const f32 nx = (offset_x + (f32)i - 1.0f) * 0.1f;
				const f32 ny = (offset_y + (f32)j - 1.0f) * 0.1f;
				const f32 nz = (offset_z + (f32)k - 1.0f) * 0.1f;

				const f32 n = tgm_noise(nx, ny, nz);
				const f32 n_clamped = tgm_f32_clamp(n, -1.0f, 1.0f);
				const i8 isolevel = (i8)(127.0f * n_clamped);
				
				TG_ISOLEVEL_AT(*p_isolevels, i, j, k) = isolevel;
			}
		}
	}
}
