#include "tg_transvoxel.h"
#include "tg_transvoxel_lookup_tables.h"
#include "util/tg_string.h"
#include "platform/tg_platform.h"



v3 tg_transvoxel_internal_interpolate(i32 d0, i32 d1, const v3* p_p0, const v3* p_p1)
{
	const i32 t = (d1 << 8) / (d1 - d0);

	v3 q = { 0 };
	if ((t & 0x00ff) != 0)
	{
		// Vertex lies in the interior of the edge.
		const f32 f = (f32)d1 / ((f32)d1 - (f32)d0);
		q.x = (f * p_p0->x + (1.0f - f) * p_p1->x);
		q.y = (f * p_p0->y + (1.0f - f) * p_p1->y);
		q.z = (f * p_p0->z + (1.0f - f) * p_p1->z);
	}
	else if (t == 0)
	{
		// Vertex lies at the higher-numbered endpoint.
		q = *p_p1;
	}
	else
	{
		// Vertex lies at the lower-numbered endpoint.
		q = *p_p0;
	}

	return q;
}

v3 tg_transvoxel_internal_interpolate_alt(const tg_transvoxel_isolevels* p_isolevels, u8 lod, v3i i0, v3i i1, v3 p0, v3 p1)
{
	const u8 lodf = 1 << lod;

	i32 d0 = (i32)TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, i0);
	i32 d1 = (i32)TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, i1);
	i32 t = (d1 << 8) / (d1 - d0);

	v3 q = { 0 };
	if ((t & 0x00ff) != 0)
	{
		// Vertex lies in the interior of the edge.
		for (u8 i = 0; i < lod; i++)
		{
			v3i midpoint = { 0 };
			midpoint.x = (i0.x + i1.x) / 2;
			midpoint.y = (i0.y + i1.y) / 2;
			midpoint.z = (i0.z + i1.z) / 2;
			const i8 imid = TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, midpoint);

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
			else
			{
				TG_ASSERT(TG_FALSE); // TODO: remove this line m8
			}
		}

		const f32 f = (f32)d1 / ((f32)d1 - (f32)d0);
		q.x = (f * p0.x + (1.0f - f) * p1.x);
		q.y = (f * p0.y + (1.0f - f) * p1.y);
		q.z = (f * p0.z + (1.0f - f) * p1.z);
	}
	else if (t == 0)
	{
		// Vertex lies at the higher-numbered endpoint.
		q = p1;
	}
	else
	{
		// Vertex lies at the lower-numbered endpoint.
		q = p0;
	}

	return q;
}

u32 tg_transvoxel_internal_create_transition_face(i32 x, i32 y, i32 z, const tg_transvoxel_isolevels* p_isolevels, u8 lod, u8 transition_face, u8 transition_faces, tg_transvoxel_triangle* p_triangles)
{
	TG_ASSERT(p_isolevels && lod > 0 && p_triangles);

	transition_faces &= ~transition_face;

	u32 chunk_triangle_count = 0;
	const u32 lodf = 1 << lod;
	const u8 nlod = lod - 1;
	const u32 nlodf = 1 << nlod;

	v3i p_isolevel_indices[13] = { 0 };
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
					p_isolevel_indices[0x0] = (v3i){ 0, lodf * cy            , lodf * cz             };
					p_isolevel_indices[0x1] = (v3i){ 0, lodf * cy            , lodf * cz + nlodf * 1 };
					p_isolevel_indices[0x2] = (v3i){ 0, lodf * cy            , lodf * cz + nlodf * 2 };
					p_isolevel_indices[0x3] = (v3i){ 0, lodf * cy + nlodf * 1, lodf * cz             };
					p_isolevel_indices[0x4] = (v3i){ 0, lodf * cy + nlodf * 1, lodf * cz + nlodf * 1 };
					p_isolevel_indices[0x5] = (v3i){ 0, lodf * cy + nlodf * 1, lodf * cz + nlodf * 2 };
					p_isolevel_indices[0x6] = (v3i){ 0, lodf * cy + nlodf * 2, lodf * cz             };
					p_isolevel_indices[0x7] = (v3i){ 0, lodf * cy + nlodf * 2, lodf * cz + nlodf * 1 };
					p_isolevel_indices[0x8] = (v3i){ 0, lodf * cy + nlodf * 2, lodf * cz + nlodf * 2 };
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
					p_isolevel_indices[0x0] = (v3i){ 16, lodf * cy            , lodf * cz             };
					p_isolevel_indices[0x1] = (v3i){ 16, lodf * cy + nlodf * 1, lodf * cz             };
					p_isolevel_indices[0x2] = (v3i){ 16, lodf * cy + nlodf * 2, lodf * cz             };
					p_isolevel_indices[0x3] = (v3i){ 16, lodf * cy            , lodf * cz + nlodf * 1 };
					p_isolevel_indices[0x4] = (v3i){ 16, lodf * cy + nlodf * 1, lodf * cz + nlodf * 1 };
					p_isolevel_indices[0x5] = (v3i){ 16, lodf * cy + nlodf * 2, lodf * cz + nlodf * 1 };
					p_isolevel_indices[0x6] = (v3i){ 16, lodf * cy            , lodf * cz + nlodf * 2 };
					p_isolevel_indices[0x7] = (v3i){ 16, lodf * cy + nlodf * 1, lodf * cz + nlodf * 2 };
					p_isolevel_indices[0x8] = (v3i){ 16, lodf * cy + nlodf * 2, lodf * cz + nlodf * 2 };
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
					p_isolevel_indices[0x0] = (v3i){ lodf * cx            , 0, lodf * cz             };
					p_isolevel_indices[0x1] = (v3i){ lodf * cx + nlodf * 1, 0, lodf * cz             };
					p_isolevel_indices[0x2] = (v3i){ lodf * cx + nlodf * 2, 0, lodf * cz             };
					p_isolevel_indices[0x3] = (v3i){ lodf * cx            , 0, lodf * cz + nlodf * 1 };
					p_isolevel_indices[0x4] = (v3i){ lodf * cx + nlodf * 1, 0, lodf * cz + nlodf * 1 };
					p_isolevel_indices[0x5] = (v3i){ lodf * cx + nlodf * 2, 0, lodf * cz + nlodf * 1 };
					p_isolevel_indices[0x6] = (v3i){ lodf * cx            , 0, lodf * cz + nlodf * 2 };
					p_isolevel_indices[0x7] = (v3i){ lodf * cx + nlodf * 1, 0, lodf * cz + nlodf * 2 };
					p_isolevel_indices[0x8] = (v3i){ lodf * cx + nlodf * 2, 0, lodf * cz + nlodf * 2 };
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
					p_isolevel_indices[0x0] = (v3i){ lodf * cx            , 16, lodf * cz             };
					p_isolevel_indices[0x1] = (v3i){ lodf * cx            , 16, lodf * cz + nlodf * 1 };
					p_isolevel_indices[0x2] = (v3i){ lodf * cx            , 16, lodf * cz + nlodf * 2 };
					p_isolevel_indices[0x3] = (v3i){ lodf * cx + nlodf * 1, 16, lodf * cz             };
					p_isolevel_indices[0x4] = (v3i){ lodf * cx + nlodf * 1, 16, lodf * cz + nlodf * 1 };
					p_isolevel_indices[0x5] = (v3i){ lodf * cx + nlodf * 1, 16, lodf * cz + nlodf * 2 };
					p_isolevel_indices[0x6] = (v3i){ lodf * cx + nlodf * 2, 16, lodf * cz             };
					p_isolevel_indices[0x7] = (v3i){ lodf * cx + nlodf * 2, 16, lodf * cz + nlodf * 1 };
					p_isolevel_indices[0x8] = (v3i){ lodf * cx + nlodf * 2, 16, lodf * cz + nlodf * 2 };
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
					p_isolevel_indices[0x0] = (v3i){ lodf * cx            , lodf * cy            , 0 };
					p_isolevel_indices[0x1] = (v3i){ lodf * cx            , lodf * cy + nlodf * 1, 0 };
					p_isolevel_indices[0x2] = (v3i){ lodf * cx            , lodf * cy + nlodf * 2, 0 };
					p_isolevel_indices[0x3] = (v3i){ lodf * cx + nlodf * 1, lodf * cy            , 0 };
					p_isolevel_indices[0x4] = (v3i){ lodf * cx + nlodf * 1, lodf * cy + nlodf * 1, 0 };
					p_isolevel_indices[0x5] = (v3i){ lodf * cx + nlodf * 1, lodf * cy + nlodf * 2, 0 };
					p_isolevel_indices[0x6] = (v3i){ lodf * cx + nlodf * 2, lodf * cy            , 0 };
					p_isolevel_indices[0x7] = (v3i){ lodf * cx + nlodf * 2, lodf * cy + nlodf * 1, 0 };
					p_isolevel_indices[0x8] = (v3i){ lodf * cx + nlodf * 2, lodf * cy + nlodf * 2, 0 };
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
					p_isolevel_indices[0x0] = (v3i){ lodf * cx            , lodf * cy            , 16 };
					p_isolevel_indices[0x1] = (v3i){ lodf * cx + nlodf * 1, lodf * cy            , 16 };
					p_isolevel_indices[0x2] = (v3i){ lodf * cx + nlodf * 2, lodf * cy            , 16 };
					p_isolevel_indices[0x3] = (v3i){ lodf * cx            , lodf * cy + nlodf * 1, 16 };
					p_isolevel_indices[0x4] = (v3i){ lodf * cx + nlodf * 1, lodf * cy + nlodf * 1, 16 };
					p_isolevel_indices[0x5] = (v3i){ lodf * cx + nlodf * 2, lodf * cy + nlodf * 1, 16 };
					p_isolevel_indices[0x6] = (v3i){ lodf * cx            , lodf * cy + nlodf * 2, 16 };
					p_isolevel_indices[0x7] = (v3i){ lodf * cx + nlodf * 1, lodf * cy + nlodf * 2, 16 };
					p_isolevel_indices[0x8] = (v3i){ lodf * cx + nlodf * 2, lodf * cy + nlodf * 2, 16 };
				} break;
				}

				p_isolevel_indices[0x9] = p_isolevel_indices[0x0];
				p_isolevel_indices[0xa] = p_isolevel_indices[0x2];
				p_isolevel_indices[0xb] = p_isolevel_indices[0x6];
				p_isolevel_indices[0xc] = p_isolevel_indices[0x8];

				const u32 case_code =
					(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[0x0]) < 0 ? 0x01  : 0) |
					(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[0x1]) < 0 ? 0x02  : 0) |
					(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[0x2]) < 0 ? 0x04  : 0) |
					(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[0x3]) < 0 ? 0x80  : 0) |
					(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[0x4]) < 0 ? 0x100 : 0) |
					(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[0x5]) < 0 ? 0x08  : 0) |
					(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[0x6]) < 0 ? 0x40  : 0) |
					(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[0x7]) < 0 ? 0x20  : 0) |
					(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[0x8]) < 0 ? 0x10  : 0);

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

						p_positions[0x9] = (v3){ offx, offy + (f32)(lodf * cy       ), offz + (f32)(lodf * cz       ) };
						p_positions[0xa] = (v3){ offx, offy + (f32)(lodf * cy       ), offz + (f32)(lodf * cz + lodf) };
						p_positions[0xb] = (v3){ offx, offy + (f32)(lodf * cy + lodf), offz + (f32)(lodf * cz       ) };
						p_positions[0xc] = (v3){ offx, offy + (f32)(lodf * cy + lodf), offz + (f32)(lodf * cz + lodf) };
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

						p_positions[0x9] = (v3){ offx + 16.0f, offy + (f32)(lodf * cy       ), offz + (f32)(lodf * cz       ) };
						p_positions[0xa] = (v3){ offx + 16.0f, offy + (f32)(lodf * cy + lodf), offz + (f32)(lodf * cz       ) };
						p_positions[0xb] = (v3){ offx + 16.0f, offy + (f32)(lodf * cy       ), offz + (f32)(lodf * cz + lodf) };
						p_positions[0xc] = (v3){ offx + 16.0f, offy + (f32)(lodf * cy + lodf), offz + (f32)(lodf * cz + lodf) };
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

						p_positions[0x9] = (v3){ offx + (f32)(lodf * cx       ), offy, offz + (f32)(lodf * cz       ) };
						p_positions[0xa] = (v3){ offx + (f32)(lodf * cx + lodf), offy, offz + (f32)(lodf * cz       ) };
						p_positions[0xb] = (v3){ offx + (f32)(lodf * cx       ), offy, offz + (f32)(lodf * cz + lodf) };
						p_positions[0xc] = (v3){ offx + (f32)(lodf * cx + lodf), offy, offz + (f32)(lodf * cz + lodf) };
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

						p_positions[0x9] = (v3){ offx + (f32)(lodf * cx       ), offy + 16.0f, offz + (f32)(lodf * cz       ) };
						p_positions[0xa] = (v3){ offx + (f32)(lodf * cx       ), offy + 16.0f, offz + (f32)(lodf * cz + lodf) };
						p_positions[0xb] = (v3){ offx + (f32)(lodf * cx + lodf), offy + 16.0f, offz + (f32)(lodf * cz       ) };
						p_positions[0xc] = (v3){ offx + (f32)(lodf * cx + lodf), offy + 16.0f, offz + (f32)(lodf * cz + lodf) };
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

						p_positions[0x9] = (v3){ offx + (f32)(lodf * cx       ), offy + (f32)(lodf * cy       ), offz };
						p_positions[0xa] = (v3){ offx + (f32)(lodf * cx       ), offy + (f32)(lodf * cy + lodf), offz };
						p_positions[0xb] = (v3){ offx + (f32)(lodf * cx + lodf), offy + (f32)(lodf * cy       ), offz };
						p_positions[0xc] = (v3){ offx + (f32)(lodf * cx + lodf), offy + (f32)(lodf * cy + lodf), offz };
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

						p_positions[0x9] = (v3){ offx + (f32)(lodf * cx       ), offy + (f32)(lodf * cy       ), offz + 16.0f };
						p_positions[0xa] = (v3){ offx + (f32)(lodf * cx + lodf), offy + (f32)(lodf * cy       ), offz + 16.0f };
						p_positions[0xb] = (v3){ offx + (f32)(lodf * cx       ), offy + (f32)(lodf * cy + lodf), offz + 16.0f };
						p_positions[0xc] = (v3){ offx + (f32)(lodf * cx + lodf), offy + (f32)(lodf * cy + lodf), offz + 16.0f };
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

						TG_ASSERT(e0v0 < 9 && e0v1 < 9 || e0v0 >= 9 && e0v1 >= 9);
						TG_ASSERT(e1v0 < 9 && e1v1 < 9 || e1v0 >= 9 && e1v1 >= 9);
						TG_ASSERT(e2v0 < 9 && e2v1 < 9 || e2v0 >= 9 && e2v1 >= 9);

						const v3 v0 = tg_transvoxel_internal_interpolate_alt(p_isolevels, e0v0 < 9 ? nlod : lod, p_isolevel_indices[e0v0], p_isolevel_indices[e0v1], p_positions[e0v0], p_positions[e0v1]);
						const v3 v1 = tg_transvoxel_internal_interpolate_alt(p_isolevels, e1v0 < 9 ? nlod : lod, p_isolevel_indices[e1v0], p_isolevel_indices[e1v1], p_positions[e1v0], p_positions[e1v1]);
						const v3 v2 = tg_transvoxel_internal_interpolate_alt(p_isolevels, e2v0 < 9 ? nlod : lod, p_isolevel_indices[e2v0], p_isolevel_indices[e2v1], p_positions[e2v0], p_positions[e2v1]);

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

	v3i p_isolevel_indices[8] = { 0 };
	v3 p_positions[8] = { 0 };
	const u8 lodf = 1 << lod;

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

				p_isolevel_indices[0] = (v3i){ (cx    ) * lodf, (cy    ) * lodf, (cz    ) * lodf };
				p_isolevel_indices[1] = (v3i){ (cx + 1) * lodf, (cy    ) * lodf, (cz    ) * lodf };
				p_isolevel_indices[2] = (v3i){ (cx    ) * lodf, (cy + 1) * lodf, (cz    ) * lodf };
				p_isolevel_indices[3] = (v3i){ (cx + 1) * lodf, (cy + 1) * lodf, (cz    ) * lodf };
				p_isolevel_indices[4] = (v3i){ (cx    ) * lodf, (cy    ) * lodf, (cz + 1) * lodf };
				p_isolevel_indices[5] = (v3i){ (cx + 1) * lodf, (cy    ) * lodf, (cz + 1) * lodf };
				p_isolevel_indices[6] = (v3i){ (cx    ) * lodf, (cy + 1) * lodf, (cz + 1) * lodf };
				p_isolevel_indices[7] = (v3i){ (cx + 1) * lodf, (cy + 1) * lodf, (cz + 1) * lodf };

				const u32 case_code =
					((TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[0]) >> 7) & 0x01) |
					((TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[1]) >> 6) & 0x02) |
					((TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[2]) >> 5) & 0x04) |
					((TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[3]) >> 4) & 0x08) |
					((TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[4]) >> 3) & 0x10) |
					((TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[5]) >> 2) & 0x20) |
					((TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[6]) >> 1) & 0x40) |
					((TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[7])     ) & 0x80);

				if ((case_code ^ ((TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_isolevel_indices[7]) >> 7) & 0xff)) != 0)
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

						// TODO: p_corners should be selected for the closest transition from negative to positive. also, scale transitions...
						const v3 v0 = tg_transvoxel_internal_interpolate_alt(p_isolevels, lod, p_isolevel_indices[e0v0], p_isolevel_indices[e0v1], p_positions[e0v0], p_positions[e0v1]);
						const v3 v1 = tg_transvoxel_internal_interpolate_alt(p_isolevels, lod, p_isolevel_indices[e1v0], p_isolevel_indices[e1v1], p_positions[e1v0], p_positions[e1v1]);
						const v3 v2 = tg_transvoxel_internal_interpolate_alt(p_isolevels, lod, p_isolevel_indices[e2v0], p_isolevel_indices[e2v1], p_positions[e2v0], p_positions[e2v1]);

						const v3 v01 = tgm_v3_subtract_v3(&v1, &v0);
						const v3 v02 = tgm_v3_subtract_v3(&v2, &v0);
						const v3 normal = tgm_v3_cross(&v01, &v02);
						const v3 up = { 0.0f, 1.0f, 0.0f };
						const f32 dot = tgm_v3_dot(&normal, &up);

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
