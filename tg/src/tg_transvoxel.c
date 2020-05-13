#include "tg_transvoxel.h"
#include "tg_transvoxel_lookup_tables.h"



typedef enum tg_transvoxel_face
{
	TG_TRANSVOXEL_FACE_X_NEG = (1 << 0),
	TG_TRANSVOXEL_FACE_X_POS = (1 << 1),
	TG_TRANSVOXEL_FACE_Y_NEG = (1 << 2),
	TG_TRANSVOXEL_FACE_Y_POS = (1 << 3),
	TG_TRANSVOXEL_FACE_Z_NEG = (1 << 4),
	TG_TRANSVOXEL_FACE_Z_POS = (1 << 5)
} tg_transvoxel_face;



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

u32 tg_transvoxel_internal_create_transition_face(i32 x, i32 y, i32 z, const tg_transvoxel_isolevels* p_isolevels, u8 lod, u8 transition_face, u8 transition_faces, tg_transvoxel_triangle* p_triangles)
{
	TG_ASSERT(p_isolevels && lod > 0 && p_triangles);

	transition_faces &= ~transition_face;

	u32 chunk_triangle_count = 0;
	const u32 lodf = 1 << lod;
	const u32 nlodf = 1 << (lod - 1);

	i8 p_corners[13] = { 0 };
	v3 p_positions[13] = { 0 };

	const u8 x_start = transition_face == TG_TRANSVOXEL_FACE_X_POS ? 16 / lodf - 1 : 0;
	const u8 y_start = transition_face == TG_TRANSVOXEL_FACE_Y_POS ? 16 / lodf - 1 : 0;
	const u8 z_start = transition_face == TG_TRANSVOXEL_FACE_Z_POS ? 16 / lodf - 1 : 0;

	const u8 x_end = transition_face == TG_TRANSVOXEL_FACE_X_NEG ? 1 : 16 / lodf;
	const u8 y_end = transition_face == TG_TRANSVOXEL_FACE_Y_NEG ? 1 : 16 / lodf;
	const u8 z_end = transition_face == TG_TRANSVOXEL_FACE_Z_NEG ? 1 : 16 / lodf;

	for (u8 cz = z_start; cz < z_end; cz++)
	{
		for (u8 cy = y_start; cy < y_end; cy++)
		{
			for (u8 cx = x_start; cx < x_end; cx++)
			{
				switch (transition_face)
				{
				case TG_TRANSVOXEL_FACE_X_NEG:
				{
					//                6       7       8     B               C
					//                 +------+------+       +-------------+
					//                 |      |      |       |             |
					//  y              |      |4     |       |             |
					//  ^            3 +------+------+ 5     |             |
					//  |              |      |      |       |             |
					//  |              |      |      |       |             |
					//  +-----> z      +------+------+       +-------------+
					//                0       1       2     9               A
					p_corners[0x0] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 0, lodf * cy            , lodf * cz            );
					p_corners[0x1] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 0, lodf * cy            , lodf * cz + nlodf * 1);
					p_corners[0x2] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 0, lodf * cy            , lodf * cz + nlodf * 2);
					p_corners[0x3] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 0, lodf * cy + nlodf * 1, lodf * cz            );
					p_corners[0x4] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 0, lodf * cy + nlodf * 1, lodf * cz + nlodf * 1);
					p_corners[0x5] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 0, lodf * cy + nlodf * 1, lodf * cz + nlodf * 2);
					p_corners[0x6] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 0, lodf * cy + nlodf * 2, lodf * cz            );
					p_corners[0x7] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 0, lodf * cy + nlodf * 2, lodf * cz + nlodf * 1);
					p_corners[0x8] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 0, lodf * cy + nlodf * 2, lodf * cz + nlodf * 2);
				} break;
				case TG_TRANSVOXEL_FACE_X_POS:
				{
					//                6       7       8     B               C
					//                 +------+------+       +-------------+
					//                 |      |      |       |             |
					//  z              |      |4     |       |             |
					//  ^            3 +------+------+ 5     |             |
					//  |              |      |      |       |             |
					//  |              |      |      |       |             |
					//  o-----> y      +------+------+       +-------------+
					//                0       1       2     9               A
					p_corners[0x0] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 16, lodf * cy            , lodf * cz            );
					p_corners[0x1] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 16, lodf * cy + nlodf * 1, lodf * cz            );
					p_corners[0x2] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 16, lodf * cy + nlodf * 2, lodf * cz            );
					p_corners[0x3] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 16, lodf * cy            , lodf * cz + nlodf * 1);
					p_corners[0x4] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 16, lodf * cy + nlodf * 1, lodf * cz + nlodf * 1);
					p_corners[0x5] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 16, lodf * cy + nlodf * 2, lodf * cz + nlodf * 1);
					p_corners[0x6] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 16, lodf * cy            , lodf * cz + nlodf * 2);
					p_corners[0x7] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 16, lodf * cy + nlodf * 1, lodf * cz + nlodf * 2);
					p_corners[0x8] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, 16, lodf * cy + nlodf * 2, lodf * cz + nlodf * 2);
				} break;
				case TG_TRANSVOXEL_FACE_Y_NEG:
				{
					//                6       7       8     B               C
					//                 +------+------+       +-------------+
					//                 |      |      |       |             |
					//  z              |      |4     |       |             |
					//  ^            3 +------+------+ 5     |             |
					//  |              |      |      |       |             |
					//  |              |      |      |       |             |
					//  +-----> x      +------+------+       +-------------+
					//                0       1       2     9               A
					p_corners[0x0] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx            , 0, lodf * cz            );
					p_corners[0x1] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 1, 0, lodf * cz            );
					p_corners[0x2] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 2, 0, lodf * cz            );
					p_corners[0x3] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx            , 0, lodf * cz + nlodf * 1);
					p_corners[0x4] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 1, 0, lodf * cz + nlodf * 1);
					p_corners[0x5] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 2, 0, lodf * cz + nlodf * 1);
					p_corners[0x6] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx            , 0, lodf * cz + nlodf * 2);
					p_corners[0x7] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 1, 0, lodf * cz + nlodf * 2);
					p_corners[0x8] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 2, 0, lodf * cz + nlodf * 2);
				} break;
				case TG_TRANSVOXEL_FACE_Y_POS:
				{
					//                6       7       8     B               C
					//                 +------+------+       +-------------+
					//                 |      |      |       |             |
					//  x              |      |4     |       |             |
					//  ^            3 +------+------+ 5     |             |
					//  |              |      |      |       |             |
					//  |              |      |      |       |             |
					//  o-----> z      +------+------+       +-------------+
					//                0       1       2     9               A
					p_corners[0x0] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx            , 16, lodf * cz            );
					p_corners[0x1] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx            , 16, lodf * cz + nlodf * 1);
					p_corners[0x2] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx            , 16, lodf * cz + nlodf * 2);
					p_corners[0x3] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 1, 16, lodf * cz            );
					p_corners[0x4] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 1, 16, lodf * cz + nlodf * 1);
					p_corners[0x5] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 1, 16, lodf * cz + nlodf * 2);
					p_corners[0x6] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 2, 16, lodf * cz            );
					p_corners[0x7] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 2, 16, lodf * cz + nlodf * 1);
					p_corners[0x8] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 2, 16, lodf * cz + nlodf * 2);
				} break;
				case TG_TRANSVOXEL_FACE_Z_NEG:
				{
					//                6       7       8     B               C
					//                 +------+------+       +-------------+
					//                 |      |      |       |             |
					//  x              |      |4     |       |             |
					//  ^            3 +------+------+ 5     |             |
					//  |              |      |      |       |             |
					//  |              |      |      |       |             |
					//  +-----> y      +------+------+       +-------------+
					//                0       1       2     9               A
					p_corners[0x0] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx            , lodf * cy            , 0);
					p_corners[0x1] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx            , lodf * cy + nlodf * 1, 0);
					p_corners[0x2] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx            , lodf * cy + nlodf * 2, 0);
					p_corners[0x3] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 1, lodf * cy            , 0);
					p_corners[0x4] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 1, lodf * cy + nlodf * 1, 0);
					p_corners[0x5] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 1, lodf * cy + nlodf * 2, 0);
					p_corners[0x6] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 2, lodf * cy            , 0);
					p_corners[0x7] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 2, lodf * cy + nlodf * 1, 0);
					p_corners[0x8] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 2, lodf * cy + nlodf * 2, 0);
				} break;
				case TG_TRANSVOXEL_FACE_Z_POS:
				{
					//                6       7       8     B               C
					//                 +------+------+       +-------------+
					//                 |      |      |       |             |
					//  y              |      |4     |       |             |
					//  ^            3 +------+------+ 5     |             |
					//  |              |      |      |       |             |
					//  |              |      |      |       |             |
					//  o-----> x      +------+------+       +-------------+
					//                0       1       2     9               A
					p_corners[0x0] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx            , lodf * cy            , 16);
					p_corners[0x1] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 1, lodf * cy            , 16);
					p_corners[0x2] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 2, lodf * cy            , 16);
					p_corners[0x3] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx            , lodf * cy + nlodf * 1, 16);
					p_corners[0x4] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 1, lodf * cy + nlodf * 1, 16);
					p_corners[0x5] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 2, lodf * cy + nlodf * 1, 16);
					p_corners[0x6] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx            , lodf * cy + nlodf * 2, 16);
					p_corners[0x7] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 1, lodf * cy + nlodf * 2, 16);
					p_corners[0x8] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, lodf * cx + nlodf * 2, lodf * cy + nlodf * 2, 16);
				} break;
				}

				p_corners[0x9] = p_corners[0x0];
				p_corners[0xa] = p_corners[0x2];
				p_corners[0xb] = p_corners[0x6];
				p_corners[0xc] = p_corners[0x8];

				const u32 case_code =
					(p_corners[0x0] < 0 ? 0x01 : 0) |
					(p_corners[0x1] < 0 ? 0x02 : 0) |
					(p_corners[0x2] < 0 ? 0x04 : 0) |
					(p_corners[0x3] < 0 ? 0x80 : 0) |
					(p_corners[0x4] < 0 ? 0x100 : 0) |
					(p_corners[0x5] < 0 ? 0x08 : 0) |
					(p_corners[0x6] < 0 ? 0x40 : 0) |
					(p_corners[0x7] < 0 ? 0x20 : 0) |
					(p_corners[0x8] < 0 ? 0x10 : 0);

				if (case_code != 0 && case_code != 511)
				{
					const u8 equivalence_class_index = p_transition_cell_class[case_code];
					const u8 inverted = (equivalence_class_index & 0x80) >> 7;

					const tg_transition_cell_data* p_cell_data = &p_transition_cell_data[equivalence_class_index & ~0x80];
					const u16* p_vertex_data = p_transition_vertex_data[case_code];

					const u32 vertex_count = TG_CELL_GET_VERTEX_COUNT(*p_cell_data);
					const u32 cell_triangle_count = TG_CELL_GET_TRIANGLE_COUNT(*p_cell_data);

					const f32 offx = 16.0f * (f32)x;
					const f32 offy = 16.0f * (f32)y;
					const f32 offz = 16.0f * (f32)z;
					
					switch (transition_face)
					{
					case TG_TRANSVOXEL_FACE_X_NEG:
					{
						p_positions[0x0] = (v3){ offx, offy + (f32)(lodf * cy            ), offz + (f32)(lodf * cz            ) };
						p_positions[0x1] = (v3){ offx, offy + (f32)(lodf * cy            ), offz + (f32)(lodf * cz + nlodf * 1) };
						p_positions[0x2] = (v3){ offx, offy + (f32)(lodf * cy            ), offz + (f32)(lodf * cz + nlodf * 2) };
						p_positions[0x3] = (v3){ offx, offy + (f32)(lodf * cy + nlodf * 1), offz + (f32)(lodf * cz            ) };
						p_positions[0x4] = (v3){ offx, offy + (f32)(lodf * cy + nlodf * 1), offz + (f32)(lodf * cz + nlodf * 1) };
						p_positions[0x5] = (v3){ offx, offy + (f32)(lodf * cy + nlodf * 1), offz + (f32)(lodf * cz + nlodf * 2) };
						p_positions[0x6] = (v3){ offx, offy + (f32)(lodf * cy + nlodf * 2), offz + (f32)(lodf * cz            ) };
						p_positions[0x7] = (v3){ offx, offy + (f32)(lodf * cy + nlodf * 2), offz + (f32)(lodf * cz + nlodf * 1) };
						p_positions[0x8] = (v3){ offx, offy + (f32)(lodf * cy + nlodf * 2), offz + (f32)(lodf * cz + nlodf * 2) };

						p_positions[0x9] = (v3){ offx + (f32)lodf * 0.5f, offy + (f32)(lodf * cy       ), offz + (f32)(lodf * cz       ) };
						p_positions[0xa] = (v3){ offx + (f32)lodf * 0.5f, offy + (f32)(lodf * cy       ), offz + (f32)(lodf * cz + lodf) };
						p_positions[0xb] = (v3){ offx + (f32)lodf * 0.5f, offy + (f32)(lodf * cy + lodf), offz + (f32)(lodf * cz       ) };
						p_positions[0xc] = (v3){ offx + (f32)lodf * 0.5f, offy + (f32)(lodf * cy + lodf), offz + (f32)(lodf * cz + lodf) };

						if (transition_faces & TG_TRANSVOXEL_FACE_Y_NEG && cy == 0)
						{
							p_positions[0x9].y += (f32)nlodf;
							p_positions[0xa].y += (f32)nlodf;
						}
						else if (transition_faces & TG_TRANSVOXEL_FACE_Y_POS && cy == 16 / lodf - 1)
						{
							p_positions[0xb].y -= (f32)nlodf;
							p_positions[0xc].y -= (f32)nlodf;
						}
						if (transition_faces & TG_TRANSVOXEL_FACE_Z_NEG && cz == 0)
						{
							p_positions[0x9].z += (f32)nlodf;
							p_positions[0xb].z += (f32)nlodf;
						}
						else if (transition_faces & TG_TRANSVOXEL_FACE_Z_POS && cz == 16 / lodf - 1)
						{
							p_positions[0xa].z -= (f32)nlodf;
							p_positions[0xc].z -= (f32)nlodf;
						}
					} break;
					case TG_TRANSVOXEL_FACE_X_POS:
					{
						p_positions[0x0] = (v3){ offx + 16.0f, offy + (f32)(lodf * cy            ), offz + (f32)(lodf * cz            ) };
						p_positions[0x1] = (v3){ offx + 16.0f, offy + (f32)(lodf * cy + nlodf * 1), offz + (f32)(lodf * cz            ) };
						p_positions[0x2] = (v3){ offx + 16.0f, offy + (f32)(lodf * cy + nlodf * 2), offz + (f32)(lodf * cz            ) };
						p_positions[0x3] = (v3){ offx + 16.0f, offy + (f32)(lodf * cy            ), offz + (f32)(lodf * cz + nlodf * 1) };
						p_positions[0x4] = (v3){ offx + 16.0f, offy + (f32)(lodf * cy + nlodf * 1), offz + (f32)(lodf * cz + nlodf * 1) };
						p_positions[0x5] = (v3){ offx + 16.0f, offy + (f32)(lodf * cy + nlodf * 2), offz + (f32)(lodf * cz + nlodf * 1) };
						p_positions[0x6] = (v3){ offx + 16.0f, offy + (f32)(lodf * cy            ), offz + (f32)(lodf * cz + nlodf * 2) };
						p_positions[0x7] = (v3){ offx + 16.0f, offy + (f32)(lodf * cy + nlodf * 1), offz + (f32)(lodf * cz + nlodf * 2) };
						p_positions[0x8] = (v3){ offx + 16.0f, offy + (f32)(lodf * cy + nlodf * 2), offz + (f32)(lodf * cz + nlodf * 2) };

						p_positions[0x9] = (v3){ offx + 16.0f - (f32)lodf * 0.5f, offy + (f32)(lodf * cy       ), offz + (f32)(lodf * cz       ) };
						p_positions[0xa] = (v3){ offx + 16.0f - (f32)lodf * 0.5f, offy + (f32)(lodf * cy + lodf), offz + (f32)(lodf * cz       ) };
						p_positions[0xb] = (v3){ offx + 16.0f - (f32)lodf * 0.5f, offy + (f32)(lodf * cy       ), offz + (f32)(lodf * cz + lodf) };
						p_positions[0xc] = (v3){ offx + 16.0f - (f32)lodf * 0.5f, offy + (f32)(lodf * cy + lodf), offz + (f32)(lodf * cz + lodf) };

						if (transition_faces & TG_TRANSVOXEL_FACE_Y_NEG && cy == 0)
						{
							p_positions[0x9].y += (f32)nlodf;
							p_positions[0xb].y += (f32)nlodf;
						}
						else if (transition_faces & TG_TRANSVOXEL_FACE_Y_POS && cy == 16 / lodf - 1)
						{
							p_positions[0xa].y -= (f32)nlodf;
							p_positions[0xc].y -= (f32)nlodf;
						}
						if (transition_faces & TG_TRANSVOXEL_FACE_Z_NEG && cz == 0)
						{
							p_positions[0x9].z += (f32)nlodf;
							p_positions[0xa].z += (f32)nlodf;
						}
						else if (transition_faces & TG_TRANSVOXEL_FACE_Z_POS && cz == 16 / lodf - 1)
						{
							p_positions[0xb].z -= (f32)nlodf;
							p_positions[0xc].z -= (f32)nlodf;
						}
					} break;
					case TG_TRANSVOXEL_FACE_Y_NEG:
					{
						p_positions[0x0] = (v3){ offx + (f32)(lodf * cx            ), offy, offz + (f32)(lodf * cz            ) };
						p_positions[0x1] = (v3){ offx + (f32)(lodf * cx + nlodf * 1), offy, offz + (f32)(lodf * cz            ) };
						p_positions[0x2] = (v3){ offx + (f32)(lodf * cx + nlodf * 2), offy, offz + (f32)(lodf * cz            ) };
						p_positions[0x3] = (v3){ offx + (f32)(lodf * cx            ), offy, offz + (f32)(lodf * cz + nlodf * 1) };
						p_positions[0x4] = (v3){ offx + (f32)(lodf * cx + nlodf * 1), offy, offz + (f32)(lodf * cz + nlodf * 1) };
						p_positions[0x5] = (v3){ offx + (f32)(lodf * cx + nlodf * 2), offy, offz + (f32)(lodf * cz + nlodf * 1) };
						p_positions[0x6] = (v3){ offx + (f32)(lodf * cx            ), offy, offz + (f32)(lodf * cz + nlodf * 2) };
						p_positions[0x7] = (v3){ offx + (f32)(lodf * cx + nlodf * 1), offy, offz + (f32)(lodf * cz + nlodf * 2) };
						p_positions[0x8] = (v3){ offx + (f32)(lodf * cx + nlodf * 2), offy, offz + (f32)(lodf * cz + nlodf * 2) };

						p_positions[0x9] = (v3){ offx + (f32)(lodf * cx       ), offy + (f32)lodf * 0.5f, offz + (f32)(lodf * cz       ) };
						p_positions[0xa] = (v3){ offx + (f32)(lodf * cx + lodf), offy + (f32)lodf * 0.5f, offz + (f32)(lodf * cz       ) };
						p_positions[0xb] = (v3){ offx + (f32)(lodf * cx       ), offy + (f32)lodf * 0.5f, offz + (f32)(lodf * cz + lodf) };
						p_positions[0xc] = (v3){ offx + (f32)(lodf * cx + lodf), offy + (f32)lodf * 0.5f, offz + (f32)(lodf * cz + lodf) };

						if (transition_faces & TG_TRANSVOXEL_FACE_X_NEG && cx == 0)
						{
							p_positions[0x9].x += (f32)nlodf;
							p_positions[0xb].x += (f32)nlodf;
						}
						else if (transition_faces & TG_TRANSVOXEL_FACE_X_POS && cx == 16 / lodf - 1)
						{
							p_positions[0xa].x -= (f32)nlodf;
							p_positions[0xc].x -= (f32)nlodf;
						}
						if (transition_faces & TG_TRANSVOXEL_FACE_Z_NEG && cz == 0)
						{
							p_positions[0x9].z += (f32)nlodf;
							p_positions[0xa].z += (f32)nlodf;
						}
						else if (transition_faces & TG_TRANSVOXEL_FACE_Z_POS && cz == 16 / lodf - 1)
						{
							p_positions[0xb].z -= (f32)nlodf;
							p_positions[0xc].z -= (f32)nlodf;
						}
					} break;
					case TG_TRANSVOXEL_FACE_Y_POS:
					{
						p_positions[0x0] = (v3){ offx + (f32)(lodf * cx            ), offy + 16.0f, offz + (f32)(lodf * cz            ) };
						p_positions[0x1] = (v3){ offx + (f32)(lodf * cx            ), offy + 16.0f, offz + (f32)(lodf * cz + nlodf * 1) };
						p_positions[0x2] = (v3){ offx + (f32)(lodf * cx            ), offy + 16.0f, offz + (f32)(lodf * cz + nlodf * 2) };
						p_positions[0x3] = (v3){ offx + (f32)(lodf * cx + nlodf * 1), offy + 16.0f, offz + (f32)(lodf * cz            ) };
						p_positions[0x4] = (v3){ offx + (f32)(lodf * cx + nlodf * 1), offy + 16.0f, offz + (f32)(lodf * cz + nlodf * 1) };
						p_positions[0x5] = (v3){ offx + (f32)(lodf * cx + nlodf * 1), offy + 16.0f, offz + (f32)(lodf * cz + nlodf * 2) };
						p_positions[0x6] = (v3){ offx + (f32)(lodf * cx + nlodf * 2), offy + 16.0f, offz + (f32)(lodf * cz            ) };
						p_positions[0x7] = (v3){ offx + (f32)(lodf * cx + nlodf * 2), offy + 16.0f, offz + (f32)(lodf * cz + nlodf * 1) };
						p_positions[0x8] = (v3){ offx + (f32)(lodf * cx + nlodf * 2), offy + 16.0f, offz + (f32)(lodf * cz + nlodf * 2) };

						p_positions[0x9] = (v3){ offx + (f32)(lodf * cx       ), offy + 16.0f - (f32)lodf * 0.5f, offz + (f32)(lodf * cz       ) };
						p_positions[0xa] = (v3){ offx + (f32)(lodf * cx       ), offy + 16.0f - (f32)lodf * 0.5f, offz + (f32)(lodf * cz + lodf) };
						p_positions[0xb] = (v3){ offx + (f32)(lodf * cx + lodf), offy + 16.0f - (f32)lodf * 0.5f, offz + (f32)(lodf * cz       ) };
						p_positions[0xc] = (v3){ offx + (f32)(lodf * cx + lodf), offy + 16.0f - (f32)lodf * 0.5f, offz + (f32)(lodf * cz + lodf) };

						if (transition_faces & TG_TRANSVOXEL_FACE_X_NEG && cx == 0)
						{
							p_positions[0x9].x += (f32)nlodf;
							p_positions[0xa].x += (f32)nlodf;
						}
						else if (transition_faces & TG_TRANSVOXEL_FACE_X_POS && cx == 16 / lodf - 1)
						{
							p_positions[0xb].x -= (f32)nlodf;
							p_positions[0xc].x -= (f32)nlodf;
						}
						if (transition_faces & TG_TRANSVOXEL_FACE_Z_NEG && cz == 0)
						{
							p_positions[0x9].z += (f32)nlodf;
							p_positions[0xb].z += (f32)nlodf;
						}
						else if (transition_faces & TG_TRANSVOXEL_FACE_Z_POS && cz == 16 / lodf - 1)
						{
							p_positions[0xa].z -= (f32)nlodf;
							p_positions[0xc].z -= (f32)nlodf;
						}
					} break;
					case TG_TRANSVOXEL_FACE_Z_NEG:
					{
						p_positions[0x0] = (v3){ offx + (f32)(lodf * cx            ), offy + (f32)(lodf * cy            ), offz };
						p_positions[0x1] = (v3){ offx + (f32)(lodf * cx            ), offy + (f32)(lodf * cy + nlodf * 1), offz };
						p_positions[0x2] = (v3){ offx + (f32)(lodf * cx            ), offy + (f32)(lodf * cy + nlodf * 2), offz };
						p_positions[0x3] = (v3){ offx + (f32)(lodf * cx + nlodf * 1), offy + (f32)(lodf * cy            ), offz };
						p_positions[0x4] = (v3){ offx + (f32)(lodf * cx + nlodf * 1), offy + (f32)(lodf * cy + nlodf * 1), offz };
						p_positions[0x5] = (v3){ offx + (f32)(lodf * cx + nlodf * 1), offy + (f32)(lodf * cy + nlodf * 2), offz };
						p_positions[0x6] = (v3){ offx + (f32)(lodf * cx + nlodf * 2), offy + (f32)(lodf * cy            ), offz };
						p_positions[0x7] = (v3){ offx + (f32)(lodf * cx + nlodf * 2), offy + (f32)(lodf * cy + nlodf * 1), offz };
						p_positions[0x8] = (v3){ offx + (f32)(lodf * cx + nlodf * 2), offy + (f32)(lodf * cy + nlodf * 2), offz };

						p_positions[0x9] = (v3){ offx + (f32)(lodf * cx       ), offy + (f32)(lodf * cy       ), offz + (f32)lodf * 0.5f };
						p_positions[0xa] = (v3){ offx + (f32)(lodf * cx       ), offy + (f32)(lodf * cy + lodf), offz + (f32)lodf * 0.5f };
						p_positions[0xb] = (v3){ offx + (f32)(lodf * cx + lodf), offy + (f32)(lodf * cy       ), offz + (f32)lodf * 0.5f };
						p_positions[0xc] = (v3){ offx + (f32)(lodf * cx + lodf), offy + (f32)(lodf * cy + lodf), offz + (f32)lodf * 0.5f };

						if (transition_faces & TG_TRANSVOXEL_FACE_X_NEG && cx == 0)
						{
							p_positions[0x9].x += (f32)nlodf;
							p_positions[0xa].x += (f32)nlodf;
						}
						else if (transition_faces & TG_TRANSVOXEL_FACE_X_POS && cx == 16 / lodf - 1)
						{
							p_positions[0xb].x -= (f32)nlodf;
							p_positions[0xc].x -= (f32)nlodf;
						}
						if (transition_faces & TG_TRANSVOXEL_FACE_Y_NEG && cy == 0)
						{
							p_positions[0x9].y += (f32)nlodf;
							p_positions[0xb].y += (f32)nlodf;
						}
						else if (transition_faces & TG_TRANSVOXEL_FACE_Y_POS && cy == 16 / lodf - 1)
						{
							p_positions[0xa].y -= (f32)nlodf;
							p_positions[0xc].y -= (f32)nlodf;
						}
					} break;
					case TG_TRANSVOXEL_FACE_Z_POS:
					{
						p_positions[0x0] = (v3){ offx + (f32)(lodf * cx            ), offy + (f32)(lodf * cy            ), offz + 16.0f };
						p_positions[0x1] = (v3){ offx + (f32)(lodf * cx + nlodf * 1), offy + (f32)(lodf * cy            ), offz + 16.0f };
						p_positions[0x2] = (v3){ offx + (f32)(lodf * cx + nlodf * 2), offy + (f32)(lodf * cy            ), offz + 16.0f };
						p_positions[0x3] = (v3){ offx + (f32)(lodf * cx            ), offy + (f32)(lodf * cy + nlodf * 1), offz + 16.0f };
						p_positions[0x4] = (v3){ offx + (f32)(lodf * cx + nlodf * 1), offy + (f32)(lodf * cy + nlodf * 1), offz + 16.0f };
						p_positions[0x5] = (v3){ offx + (f32)(lodf * cx + nlodf * 2), offy + (f32)(lodf * cy + nlodf * 1), offz + 16.0f };
						p_positions[0x6] = (v3){ offx + (f32)(lodf * cx            ), offy + (f32)(lodf * cy + nlodf * 2), offz + 16.0f };
						p_positions[0x7] = (v3){ offx + (f32)(lodf * cx + nlodf * 1), offy + (f32)(lodf * cy + nlodf * 2), offz + 16.0f };
						p_positions[0x8] = (v3){ offx + (f32)(lodf * cx + nlodf * 2), offy + (f32)(lodf * cy + nlodf * 2), offz + 16.0f };

						p_positions[0x9] = (v3){ offx + (f32)(lodf * cx       ), offy + (f32)(lodf * cy       ), offz + 16.0f - (f32)lodf * 0.5f };
						p_positions[0xa] = (v3){ offx + (f32)(lodf * cx + lodf), offy + (f32)(lodf * cy       ), offz + 16.0f - (f32)lodf * 0.5f };
						p_positions[0xb] = (v3){ offx + (f32)(lodf * cx       ), offy + (f32)(lodf * cy + lodf), offz + 16.0f - (f32)lodf * 0.5f };
						p_positions[0xc] = (v3){ offx + (f32)(lodf * cx + lodf), offy + (f32)(lodf * cy + lodf), offz + 16.0f - (f32)lodf * 0.5f };

						if (transition_faces & TG_TRANSVOXEL_FACE_X_NEG && cx == 0)
						{
							p_positions[0x9].x += (f32)nlodf;
							p_positions[0xb].x += (f32)nlodf;
						}
						else if (transition_faces & TG_TRANSVOXEL_FACE_X_POS && cx == 16 / lodf - 1)
						{
							p_positions[0xa].x -= (f32)nlodf;
							p_positions[0xc].x -= (f32)nlodf;
						}
						if (transition_faces & TG_TRANSVOXEL_FACE_Y_NEG && cy == 0)
						{
							p_positions[0x9].y += (f32)nlodf;
							p_positions[0xa].y += (f32)nlodf;
						}
						else if (transition_faces & TG_TRANSVOXEL_FACE_Y_POS && cy == 16 / lodf - 1)
						{
							p_positions[0xb].y -= (f32)nlodf;
							p_positions[0xc].y -= (f32)nlodf;
						}
					} break;
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

						if (!inverted)
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
	}

	return chunk_triangle_count;
}



u32 tg_transvoxel_create_chunk(i32 x, i32 y, i32 z, const tg_transvoxel_isolevels* p_isolevels, u8 lod, u8 transition_faces, tg_transvoxel_triangle* p_triangles)
{
	TG_ASSERT(p_isolevels && p_triangles);

	u32 chunk_triangle_count = 0;

	i8 p_corners[8] = { 0 };
	v3 p_positions[8] = { 0 };
	const u8 lodf = 1 << lod;

	if (transition_faces & TG_TRANSVOXEL_FACE_X_NEG)
	{
		chunk_triangle_count += tg_transvoxel_internal_create_transition_face(x, y, z, p_isolevels, lod, TG_TRANSVOXEL_FACE_X_NEG, transition_faces, &p_triangles[chunk_triangle_count]);
	}
	if (transition_faces & TG_TRANSVOXEL_FACE_X_POS)
	{
		chunk_triangle_count += tg_transvoxel_internal_create_transition_face(x, y, z, p_isolevels, lod, TG_TRANSVOXEL_FACE_X_POS, transition_faces, &p_triangles[chunk_triangle_count]);
	}
	if (transition_faces & TG_TRANSVOXEL_FACE_Y_NEG)
	{
		chunk_triangle_count += tg_transvoxel_internal_create_transition_face(x, y, z, p_isolevels, lod, TG_TRANSVOXEL_FACE_Y_NEG, transition_faces, &p_triangles[chunk_triangle_count]);
	}
	if (transition_faces & TG_TRANSVOXEL_FACE_Y_POS)
	{
		chunk_triangle_count += tg_transvoxel_internal_create_transition_face(x, y, z, p_isolevels, lod, TG_TRANSVOXEL_FACE_Y_POS, transition_faces, &p_triangles[chunk_triangle_count]);
	}
	if (transition_faces & TG_TRANSVOXEL_FACE_Z_NEG)
	{
		chunk_triangle_count += tg_transvoxel_internal_create_transition_face(x, y, z, p_isolevels, lod, TG_TRANSVOXEL_FACE_Z_NEG, transition_faces, &p_triangles[chunk_triangle_count]);
	}
	if (transition_faces & TG_TRANSVOXEL_FACE_Z_POS)
	{
		chunk_triangle_count += tg_transvoxel_internal_create_transition_face(x, y, z, p_isolevels, lod, TG_TRANSVOXEL_FACE_Z_POS, transition_faces, &p_triangles[chunk_triangle_count]);
	}

	for (u8 cz = 0; cz < 16 / lodf; cz++)
	{
		for (u8 cy = 0; cy < 16 / lodf; cy++)
		{
			for (u8 cx = 0; cx < 16 / lodf; cx++)
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

				p_corners[0] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, (cx    ) * lodf, (cy    ) * lodf, (cz    ) * lodf);
				p_corners[1] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, (cx + 1) * lodf, (cy    ) * lodf, (cz    ) * lodf);
				p_corners[2] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, (cx    ) * lodf, (cy + 1) * lodf, (cz    ) * lodf);
				p_corners[3] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, (cx + 1) * lodf, (cy + 1) * lodf, (cz    ) * lodf);
				p_corners[4] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, (cx    ) * lodf, (cy    ) * lodf, (cz + 1) * lodf);
				p_corners[5] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, (cx + 1) * lodf, (cy    ) * lodf, (cz + 1) * lodf);
				p_corners[6] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, (cx    ) * lodf, (cy + 1) * lodf, (cz + 1) * lodf);
				p_corners[7] = TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, (cx + 1) * lodf, (cy + 1) * lodf, (cz + 1) * lodf);

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

					const f32 offx = 16.0f * (f32)x;
					const f32 offy = 16.0f * (f32)y;
					const f32 offz = 16.0f * (f32)z;

					p_positions[0] = (v3){ offx + (f32)( cx      * lodf), offy + (f32)( cy      * lodf), offz + (f32)( cz      * lodf) };
					p_positions[1] = (v3){ offx + (f32)((cx + 1) * lodf), offy + (f32)( cy      * lodf), offz + (f32)( cz      * lodf) };
					p_positions[2] = (v3){ offx + (f32)( cx      * lodf), offy + (f32)((cy + 1) * lodf), offz + (f32)( cz      * lodf) };
					p_positions[3] = (v3){ offx + (f32)((cx + 1) * lodf), offy + (f32)((cy + 1) * lodf), offz + (f32)( cz      * lodf) };
					p_positions[4] = (v3){ offx + (f32)( cx      * lodf), offy + (f32)( cy      * lodf), offz + (f32)((cz + 1) * lodf) };
					p_positions[5] = (v3){ offx + (f32)((cx + 1) * lodf), offy + (f32)( cy      * lodf), offz + (f32)((cz + 1) * lodf) };
					p_positions[6] = (v3){ offx + (f32)( cx      * lodf), offy + (f32)((cy + 1) * lodf), offz + (f32)((cz + 1) * lodf) };
					p_positions[7] = (v3){ offx + (f32)((cx + 1) * lodf), offy + (f32)((cy + 1) * lodf), offz + (f32)((cz + 1) * lodf) };

					if (transition_faces & TG_TRANSVOXEL_FACE_X_NEG && cx == 0)
					{
						p_positions[0].x += 0.5f * (f32)lodf;
						p_positions[2].x += 0.5f * (f32)lodf;
						p_positions[4].x += 0.5f * (f32)lodf;
						p_positions[6].x += 0.5f * (f32)lodf;
					}
					else if (transition_faces & TG_TRANSVOXEL_FACE_X_POS && cx == 16 / lodf - 1)
					{
						p_positions[1].x -= 0.5f * (f32)lodf;
						p_positions[3].x -= 0.5f * (f32)lodf;
						p_positions[5].x -= 0.5f * (f32)lodf;
						p_positions[7].x -= 0.5f * (f32)lodf;
					}
					if (transition_faces & TG_TRANSVOXEL_FACE_Y_NEG && cy == 0)
					{
						p_positions[0].y += 0.5f * (f32)lodf;
						p_positions[1].y += 0.5f * (f32)lodf;
						p_positions[4].y += 0.5f * (f32)lodf;
						p_positions[5].y += 0.5f * (f32)lodf;
					}
					else if (transition_faces & TG_TRANSVOXEL_FACE_Y_POS && cy == 16 / lodf - 1)
					{
						p_positions[2].y -= 0.5f * (f32)lodf;
						p_positions[3].y -= 0.5f * (f32)lodf;
						p_positions[6].y -= 0.5f * (f32)lodf;
						p_positions[7].y -= 0.5f * (f32)lodf;
					}
					if (transition_faces & TG_TRANSVOXEL_FACE_Z_NEG && cz == 0)
					{
						p_positions[0].z += 0.5f * (f32)lodf;
						p_positions[1].z += 0.5f * (f32)lodf;
						p_positions[2].z += 0.5f * (f32)lodf;
						p_positions[3].z += 0.5f * (f32)lodf;
					}
					else if (transition_faces & TG_TRANSVOXEL_FACE_Z_POS && cz == 16 / lodf - 1)
					{
						p_positions[4].z -= 0.5f * (f32)lodf;
						p_positions[5].z -= 0.5f * (f32)lodf;
						p_positions[6].z -= 0.5f * (f32)lodf;
						p_positions[7].z -= 0.5f * (f32)lodf;
					}

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

void tg_transvoxel_fill_isolevels(i32 x, i32 y, i32 z, tg_transvoxel_isolevels* p_isolevels)
{
	TG_ASSERT(p_isolevels);

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
				
				TG_TRANSVOXEL_ISOLEVEL_AT(*p_isolevels, i, j, k) = isolevel;
			}
		}
	}
}
