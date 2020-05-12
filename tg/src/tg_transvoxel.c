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

u32 tg_transvoxel_create_chunk(const tg_transvoxel_isolevels* p_isolevels, i32 x, i32 y, i32 z, u8 lod, tg_transvoxel_triangle* p_triangles)
{
	TG_ASSERT(p_isolevels && p_triangles);

	u32 chunk_triangle_count = 0;

	i8 p_corners[8] = { 0 };
	v3 p_positions[8] = { 0 };
	const u8 lod_factor = 1 << lod;

	for (u8 cell_z = 0; cell_z < 16 / lod_factor; cell_z++)
	{
		for (u8 cell_y = 0; cell_y < 16 / lod_factor; cell_y++)
		{
			for (u8 cell_x = 0; cell_x < 16 / lod_factor; cell_x++)
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

				p_corners[0] = TG_ISOLEVEL_AT(*p_isolevels, (cell_x    ) * lod_factor, (cell_y    ) * lod_factor, (cell_z    ) * lod_factor);
				p_corners[1] = TG_ISOLEVEL_AT(*p_isolevels, (cell_x + 1) * lod_factor, (cell_y    ) * lod_factor, (cell_z    ) * lod_factor);
				p_corners[2] = TG_ISOLEVEL_AT(*p_isolevels, (cell_x    ) * lod_factor, (cell_y + 1) * lod_factor, (cell_z    ) * lod_factor);
				p_corners[3] = TG_ISOLEVEL_AT(*p_isolevels, (cell_x + 1) * lod_factor, (cell_y + 1) * lod_factor, (cell_z    ) * lod_factor);
				p_corners[4] = TG_ISOLEVEL_AT(*p_isolevels, (cell_x    ) * lod_factor, (cell_y    ) * lod_factor, (cell_z + 1) * lod_factor);
				p_corners[5] = TG_ISOLEVEL_AT(*p_isolevels, (cell_x + 1) * lod_factor, (cell_y    ) * lod_factor, (cell_z + 1) * lod_factor);
				p_corners[6] = TG_ISOLEVEL_AT(*p_isolevels, (cell_x    ) * lod_factor, (cell_y + 1) * lod_factor, (cell_z + 1) * lod_factor);
				p_corners[7] = TG_ISOLEVEL_AT(*p_isolevels, (cell_x + 1) * lod_factor, (cell_y + 1) * lod_factor, (cell_z + 1) * lod_factor);

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

					const u32 vertex_count = TG_CELL_GET_VERTEX_COUNT(*p_cell_data);
					const u32 cell_triangle_count = TG_CELL_GET_TRIANGLE_COUNT(*p_cell_data);

					const f32 offset_x = 16.0f * (f32)x;
					const f32 offset_y = 16.0f * (f32)y;
					const f32 offset_z = 16.0f * (f32)z;

					p_positions[0] = (v3){ offset_x + (f32)( cell_x      * lod_factor), offset_y + (f32)( cell_y      * lod_factor), offset_z + (f32)( cell_z      * lod_factor) };
					p_positions[1] = (v3){ offset_x + (f32)((cell_x + 1) * lod_factor), offset_y + (f32)( cell_y      * lod_factor), offset_z + (f32)( cell_z      * lod_factor) };
					p_positions[2] = (v3){ offset_x + (f32)( cell_x      * lod_factor), offset_y + (f32)((cell_y + 1) * lod_factor), offset_z + (f32)( cell_z      * lod_factor) };
					p_positions[3] = (v3){ offset_x + (f32)((cell_x + 1) * lod_factor), offset_y + (f32)((cell_y + 1) * lod_factor), offset_z + (f32)( cell_z      * lod_factor) };
					p_positions[4] = (v3){ offset_x + (f32)( cell_x      * lod_factor), offset_y + (f32)( cell_y      * lod_factor), offset_z + (f32)((cell_z + 1) * lod_factor) };
					p_positions[5] = (v3){ offset_x + (f32)((cell_x + 1) * lod_factor), offset_y + (f32)( cell_y      * lod_factor), offset_z + (f32)((cell_z + 1) * lod_factor) };
					p_positions[6] = (v3){ offset_x + (f32)( cell_x      * lod_factor), offset_y + (f32)((cell_y + 1) * lod_factor), offset_z + (f32)((cell_z + 1) * lod_factor) };
					p_positions[7] = (v3){ offset_x + (f32)((cell_x + 1) * lod_factor), offset_y + (f32)((cell_y + 1) * lod_factor), offset_z + (f32)((cell_z + 1) * lod_factor) };

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

u32 tg_transvoxel_create_transition_face(const tg_transvoxel_isolevels* p_isolevels, i32 x, i32 y, i32 z, tg_transvoxel_face face, tg_transvoxel_triangle* p_triangles)
{
	TG_ASSERT(p_isolevels && face == TG_TRANSVOXEL_FACE_Z_POS && p_triangles); // TODO: support all faces!

	u32 chunk_triangle_count = 0;

	i8 p_corners[13] = { 0 };
	v3 p_positions[13] = { 0 };

	for (u8 cell_y = 0; cell_y < 8; cell_y++)
	{
		for (u8 cell_x = 0; cell_x < 8; cell_x++)
		{
			//                6       7       8     B               C
			//                 +------+------+       +-------------+
			//                 |      |      |       |             |
			//  y              |      |4     |       |             |
			//  ^            3 +------+------+ 5     |             |
			//  |              |      |      |       |             |
			//  |              |      |      |       |             |
			//  +-----> x      +------+------+       +-------------+
			//                0       1       2     9               A

			if (face == TG_TRANSVOXEL_FACE_Z_POS) // TODO: switch?
			{
				p_corners[0x0] = TG_ISOLEVEL_AT(*p_isolevels, 2 * cell_x    , 2 * cell_y    , 16);
				p_corners[0x1] = TG_ISOLEVEL_AT(*p_isolevels, 2 * cell_x + 1, 2 * cell_y    , 16);
				p_corners[0x2] = TG_ISOLEVEL_AT(*p_isolevels, 2 * cell_x + 2, 2 * cell_y    , 16);
				p_corners[0x3] = TG_ISOLEVEL_AT(*p_isolevels, 2 * cell_x    , 2 * cell_y + 1, 16);
				p_corners[0x4] = TG_ISOLEVEL_AT(*p_isolevels, 2 * cell_x + 1, 2 * cell_y + 1, 16);
				p_corners[0x5] = TG_ISOLEVEL_AT(*p_isolevels, 2 * cell_x + 2, 2 * cell_y + 1, 16);
				p_corners[0x6] = TG_ISOLEVEL_AT(*p_isolevels, 2 * cell_x    , 2 * cell_y + 2, 16);
				p_corners[0x7] = TG_ISOLEVEL_AT(*p_isolevels, 2 * cell_x + 1, 2 * cell_y + 2, 16);
				p_corners[0x8] = TG_ISOLEVEL_AT(*p_isolevels, 2 * cell_x + 2, 2 * cell_y + 2, 16);

				p_corners[0x9] = p_corners[0x0];
				p_corners[0xa] = p_corners[0x2];
				p_corners[0xb] = p_corners[0x6];
				p_corners[0xc] = p_corners[0x8];
			}
			else
			{
				TG_ASSERT(TG_FALSE);
			}

			const u32 case_code =
				(p_corners[0x0] < 0 ? 0x01  : 0) |
				(p_corners[0x1] < 0 ? 0x02  : 0) |
				(p_corners[0x2] < 0 ? 0x04  : 0) |
				(p_corners[0x3] < 0 ? 0x80  : 0) |
				(p_corners[0x4] < 0 ? 0x100 : 0) |
				(p_corners[0x5] < 0 ? 0x08  : 0) |
				(p_corners[0x6] < 0 ? 0x40  : 0) |
				(p_corners[0x7] < 0 ? 0x20  : 0) |
				(p_corners[0x8] < 0 ? 0x10  : 0);

			if (case_code != 0 && case_code != 511)
			{
				const u8 equivalence_class_index = p_transition_cell_class[case_code];
				const u8 inverted = (equivalence_class_index & 0x80) >> 7;

				const tg_transition_cell_data* p_cell_data = &p_transition_cell_data[equivalence_class_index & ~0x80];
				const u16* p_vertex_data = p_transition_vertex_data[case_code];

				const u32 vertex_count = TG_CELL_GET_VERTEX_COUNT(*p_cell_data);
				const u32 cell_triangle_count = TG_CELL_GET_TRIANGLE_COUNT(*p_cell_data);

				const f32 offset_x = 16.0f * (f32)x;
				const f32 offset_y = 16.0f * (f32)y;
				const f32 offset_z = 16.0f * (f32)z;

				if (face == TG_TRANSVOXEL_FACE_Z_POS)
				{
					p_positions[0x0] = (v3){ offset_x + 2.0f * (f32)cell_x       , offset_y + 2.0f * (f32)cell_y       , offset_z + 16.0f };
					p_positions[0x1] = (v3){ offset_x + 2.0f * (f32)cell_x + 1.0f, offset_y + 2.0f * (f32)cell_y       , offset_z + 16.0f };
					p_positions[0x2] = (v3){ offset_x + 2.0f * (f32)cell_x + 2.0f, offset_y + 2.0f * (f32)cell_y       , offset_z + 16.0f };
					p_positions[0x3] = (v3){ offset_x + 2.0f * (f32)cell_x       , offset_y + 2.0f * (f32)cell_y + 1.0f, offset_z + 16.0f };
					p_positions[0x4] = (v3){ offset_x + 2.0f * (f32)cell_x + 1.0f, offset_y + 2.0f * (f32)cell_y + 1.0f, offset_z + 16.0f };
					p_positions[0x5] = (v3){ offset_x + 2.0f * (f32)cell_x + 2.0f, offset_y + 2.0f * (f32)cell_y + 1.0f, offset_z + 16.0f };
					p_positions[0x6] = (v3){ offset_x + 2.0f * (f32)cell_x       , offset_y + 2.0f * (f32)cell_y + 2.0f, offset_z + 16.0f };
					p_positions[0x7] = (v3){ offset_x + 2.0f * (f32)cell_x + 1.0f, offset_y + 2.0f * (f32)cell_y + 2.0f, offset_z + 16.0f };
					p_positions[0x8] = (v3){ offset_x + 2.0f * (f32)cell_x + 2.0f, offset_y + 2.0f * (f32)cell_y + 2.0f, offset_z + 16.0f };

					p_positions[0x9] = (v3){ offset_x + 2.0f * (f32)cell_x       , offset_y + 2.0f * (f32)cell_y       , offset_z + 17.0f }; // TODO: this is probably gonna be 17.0f aswell. of scale lower lod
					p_positions[0xa] = (v3){ offset_x + 2.0f * (f32)cell_x + 2.0f, offset_y + 2.0f * (f32)cell_y       , offset_z + 17.0f };
					p_positions[0xb] = (v3){ offset_x + 2.0f * (f32)cell_x       , offset_y + 2.0f * (f32)cell_y + 2.0f, offset_z + 17.0f };
					p_positions[0xc] = (v3){ offset_x + 2.0f * (f32)cell_x + 2.0f, offset_y + 2.0f * (f32)cell_y + 2.0f, offset_z + 17.0f };
				}
				else
				{
					TG_ASSERT(TG_FALSE);
				}

				for (u32 i = 0; i < cell_triangle_count; i++)
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
					
					if (inverted)
					{
						p_triangles[chunk_triangle_count].positions[0] = v0;
						p_triangles[chunk_triangle_count].positions[1] = v2;
						p_triangles[chunk_triangle_count].positions[2] = v1;
					}
					else
					{
						p_triangles[chunk_triangle_count].positions[0] = v0;
						p_triangles[chunk_triangle_count].positions[1] = v1;
						p_triangles[chunk_triangle_count].positions[2] = v2;
					}

					chunk_triangle_count++;
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

	for (i32 i = -1; i < 18; i++)
	{
		for (i32 j = -1; j < 18; j++)
		{
			for (i32 k = -1; k < 18; k++)
			{
				const f32 nx = (offset_x + (f32)i) * 0.1f;
				const f32 ny = (offset_y + (f32)j) * 0.1f;
				const f32 nz = (offset_z + (f32)k) * 0.1f;

				const f32 n = tgm_noise(nx, ny, nz);
				const f32 n_clamped = tgm_f32_clamp(n, -1.0f, 1.0f);
				const i8 isolevel = (i8)(127.0f * n_clamped);
				
				TG_ISOLEVEL_AT(*p_isolevels, i, j, k) = isolevel;
			}
		}
	}
}
