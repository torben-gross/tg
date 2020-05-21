#include "tg_transvoxel.h"
#include "tg_transvoxel_lookup_tables.h"

#include "memory/tg_memory.h"
#include "util/tg_string.h"
#include "platform/tg_platform.h"



typedef struct tg_transvoxel_cell
{
	u32                        triangle_count;
	tg_transvoxel_triangle*    p_triangles;
} tg_transvoxel_cell;



v3 tg_transvoxel_internal_interpolate(const tg_transvoxel_isolevels* p_isolevels, u8 lod, v3i i0, v3i i1, v3 p0, v3 p1)
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

					const tg_transition_cell_data* p_cell_data = &p_transition_cell_data[equivalence_class_index & 0x7f];
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

						const v3 v0 = tg_transvoxel_internal_interpolate(p_isolevels, e0v0 < 9 ? nlod : lod, p_isolevel_indices[e0v0], p_isolevel_indices[e0v1], p_positions[e0v0], p_positions[e0v1]);
						const v3 v1 = tg_transvoxel_internal_interpolate(p_isolevels, e1v0 < 9 ? nlod : lod, p_isolevel_indices[e1v0], p_isolevel_indices[e1v1], p_positions[e1v0], p_positions[e1v1]);
						const v3 v2 = tg_transvoxel_internal_interpolate(p_isolevels, e2v0 < 9 ? nlod : lod, p_isolevel_indices[e2v0], p_isolevel_indices[e2v1], p_positions[e2v0], p_positions[e2v1]);

						if (!inverted)
						{
							p_triangles[chunk_triangle_count].p_vertices[0].position = v0;
							p_triangles[chunk_triangle_count].p_vertices[1].position = v2;
							p_triangles[chunk_triangle_count].p_vertices[2].position = v1;
						}
						else
						{
							p_triangles[chunk_triangle_count].p_vertices[0].position = v0;
							p_triangles[chunk_triangle_count].p_vertices[1].position = v1;
							p_triangles[chunk_triangle_count].p_vertices[2].position = v2;
						}

						chunk_triangle_count++;
					}
				}
			}
		}
	}

	return chunk_triangle_count;
}

u32 tg_transvoxel_internal_create_transition_faces(i32 x, i32 y, i32 z, const tg_transvoxel_isolevels* p_isolevels, u8 lod, u8 transition_faces, tg_transvoxel_triangle* p_triangles)
{
	u32 triangle_count = 0;

	if (transition_faces & TG_TRANSVOXEL_FACE_X_NEG)
	{
		triangle_count += tg_transvoxel_internal_create_transition_face(x, y, z, p_isolevels, lod, TG_TRANSVOXEL_FACE_X_NEG, transition_faces, &p_triangles[triangle_count]);
	}
	if (transition_faces & TG_TRANSVOXEL_FACE_X_POS)
	{
		triangle_count += tg_transvoxel_internal_create_transition_face(x, y, z, p_isolevels, lod, TG_TRANSVOXEL_FACE_X_POS, transition_faces, &p_triangles[triangle_count]);
	}
	if (transition_faces & TG_TRANSVOXEL_FACE_Y_NEG)
	{
		triangle_count += tg_transvoxel_internal_create_transition_face(x, y, z, p_isolevels, lod, TG_TRANSVOXEL_FACE_Y_NEG, transition_faces, &p_triangles[triangle_count]);
	}
	if (transition_faces & TG_TRANSVOXEL_FACE_Y_POS)
	{
		triangle_count += tg_transvoxel_internal_create_transition_face(x, y, z, p_isolevels, lod, TG_TRANSVOXEL_FACE_Y_POS, transition_faces, &p_triangles[triangle_count]);
	}
	if (transition_faces & TG_TRANSVOXEL_FACE_Z_NEG)
	{
		triangle_count += tg_transvoxel_internal_create_transition_face(x, y, z, p_isolevels, lod, TG_TRANSVOXEL_FACE_Z_NEG, transition_faces, &p_triangles[triangle_count]);
	}
	if (transition_faces & TG_TRANSVOXEL_FACE_Z_POS)
	{
		triangle_count += tg_transvoxel_internal_create_transition_face(x, y, z, p_isolevels, lod, TG_TRANSVOXEL_FACE_Z_POS, transition_faces, &p_triangles[triangle_count]);
	}

	return triangle_count;
}

tg_transvoxel_internal_create_cell(i32 x, i32 y, i32 z, i32 cx, i32 cy, i32 cz, const tg_transvoxel_isolevels* p_isolevels, u8 lod, u8 transition_faces, tg_transvoxel_triangle* p_triangles)
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

	u32 triangle_count = 0;
	const u8 lodf = 1 << lod;

	v3i p_isolevel_indices[8] = { 0 };
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

		v3 p_positions[8] = { 0 };
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

			const v3i e00i = p_isolevel_indices[e0v0];
			const v3i e01i = p_isolevel_indices[e0v1];
			const v3i e10i = p_isolevel_indices[e1v0];
			const v3i e11i = p_isolevel_indices[e1v1];
			const v3i e20i = p_isolevel_indices[e2v0];
			const v3i e21i = p_isolevel_indices[e2v1];

			const v3 e00p = p_positions[e0v0];
			const v3 e01p = p_positions[e0v1];
			const v3 e10p = p_positions[e1v0];
			const v3 e11p = p_positions[e1v1];
			const v3 e20p = p_positions[e2v0];
			const v3 e21p = p_positions[e2v1];

			v3 v0 = tg_transvoxel_internal_interpolate(p_isolevels, lod, e00i, e01i, e00p, e01p);
			v3 v1 = tg_transvoxel_internal_interpolate(p_isolevels, lod, e10i, e11i, e10p, e11p);
			v3 v2 = tg_transvoxel_internal_interpolate(p_isolevels, lod, e20i, e21i, e20p, e21p);

			if (!tgm_v3_equal(&v0, &v1) && !tgm_v3_equal(&v0, &v2) && !tgm_v3_equal(&v1, &v2))
			{
				const v3 v01 = tgm_v3_subtract_v3(&v1, &v0);
				const v3 v02 = tgm_v3_subtract_v3(&v2, &v0);
				v3 n = tgm_v3_cross(&v01, &v02);
				n = tgm_v3_normalized(&n);

				p_triangles[triangle_count].p_vertices[0].position = v0;
				p_triangles[triangle_count].p_vertices[1].position = v1;
				p_triangles[triangle_count].p_vertices[2].position = v2;
				p_triangles[triangle_count].p_vertices[0].normal = n;
				p_triangles[triangle_count].p_vertices[1].normal = n;
				p_triangles[triangle_count].p_vertices[2].normal = n;

				triangle_count++;
			}
		}
	}

	return triangle_count;
}

v3 tg_transvoxel_internal_generate_normal(const tg_transvoxel_triangle* p_triangle)
{
	v3 result = { 0 };

	const v3 v01 = tgm_v3_subtract_v3(&p_triangle->p_vertices[1].position, &p_triangle->p_vertices[0].position);
	const v3 v02 = tgm_v3_subtract_v3(&p_triangle->p_vertices[2].position, &p_triangle->p_vertices[0].position);
	result = tgm_v3_cross(&v01, &v02);
	result = tgm_v3_normalized(&result);

	return result;
}



u32 tg_transvoxel_create_chunk(i32 x, i32 y, i32 z, const tg_transvoxel_isolevels* p_isolevels, const tg_transvoxel_isolevels** pp_connecting_isolevels, u8 lod, u8 transition_faces, tg_transvoxel_triangle* p_triangles)
{
	TG_ASSERT(p_isolevels && p_triangles);

	const u64 cells_size = 16 * 16 * 16 * sizeof(tg_transvoxel_cell);
	const u64 temp_triangles_size = 27 * 5 * sizeof(tg_transvoxel_triangle);
	const u64 temp_cells_size = 27 * sizeof(tg_transvoxel_cell);
	const u64 total_allocation_size = cells_size + temp_triangles_size + temp_cells_size;
	void* p_data = TG_MEMORY_ALLOC(total_allocation_size);

	tg_transvoxel_cell* p_cells = p_data;

	u32 triangle_count = 0;// tg_transvoxel_internal_create_transition_faces(x, y, z, p_isolevels, lod, transition_faces, p_triangles);
	const u8 lodf = 1 << lod;
	const u8 stride = 16 / lodf;

	for (i8 cz = 0; cz < stride; cz++)
	{
		for (i8 cy = 0; cy < stride; cy++)
		{
			for (i8 cx = 0; cx < stride; cx++)
			{
				const u32 cell_index = stride * stride * cz + stride * cy + cx;
				p_cells[cell_index].triangle_count = tg_transvoxel_internal_create_cell(x, y, z, cx, cy, cz, p_isolevels, lod, transition_faces, &p_triangles[triangle_count]);
				p_cells[cell_index].p_triangles = &p_triangles[triangle_count];

				triangle_count += p_cells[cell_index].triangle_count;
			}
		}
	}

	tg_transvoxel_triangle* p_temp_triangles = (tg_transvoxel_triangle*)&((u8*)p_data)[cells_size];
	tg_transvoxel_cell* p_temp_cells = (tg_transvoxel_cell*)&((u8*)p_data)[cells_size + temp_triangles_size];
	for (u8 i = 0; i < 27; i++)
	{
		p_temp_cells[i].p_triangles = &p_temp_triangles[5 * i];
	}

	v3i p_connecting_chunk_coordinates[27] = { 0 };
	v3i p_connecting_cell_coordinates[27] = { 0 };
	const tg_transvoxel_cell* pp_connecting_cells[27] = { 0 };

	for (i8 cz = 0; cz < stride; cz++)
	{
		for (i8 cy = 0; cy < stride; cy++)
		{
			for (i8 cx = 0; cx < stride; cx++)
			{
				b32 break_here = TG_FALSE;
				if (x == 1 && y == -1 && z == -2 && cx == 15 && cy == 15 && cz == 1)
				{
					break_here = TG_TRUE;
					int iiiii = 0;
				}
				else if (x == 1 && y == -1 && z == -2 && cx == 15 && cy == 15 && cz == 2)
				{
					break_here = TG_TRUE;
					int iiiii = 0;
				}
				const v3i chunk_coordinates = { x, y, z };
				for (i8 iz = -1; iz < 2; iz++)
				{
					for (i8 iy = -1; iy < 2; iy++)
					{
						for (i8 ix = -1; ix < 2; ix++)
						{
							const u8 i = 9 * (iz + 1) + 3 * (iy + 1) + (ix + 1);

							if (break_here && (i == 14 || i == 5))
							{
								int iiii = 0;
							}

							p_connecting_chunk_coordinates[i] = (v3i){
								cx + ix == -1 ? x - 1 : (cx + ix == stride ? x + 1 : x),
								cy + iy == -1 ? y - 1 : (cy + iy == stride ? y + 1 : y),
								cz + iz == -1 ? z - 1 : (cz + iz == stride ? z + 1 : z)
							};
							p_connecting_cell_coordinates[i] = (v3i){
								cx + ix == -1 ? stride - 1 : (cx + ix == stride ? 0 : cx + ix),
								cy + iy == -1 ? stride - 1 : (cy + iy == stride ? 0 : cy + iy),
								cz + iz == -1 ? stride - 1 : (cz + iz == stride ? 0 : cz + iz)
							};

							if (tgm_v3i_equal(&p_connecting_chunk_coordinates[i], &chunk_coordinates))
							{
								const u32 idx = stride * stride * p_connecting_cell_coordinates[i].z + stride * p_connecting_cell_coordinates[i].y + p_connecting_cell_coordinates[i].x;
								pp_connecting_cells[i] = &p_cells[idx];
							}
							else
							{
								if (break_here)
								{
									int iii = 0;
								}
								const v3i chunk_offset = {
									cx + ix == -1 ? -1 : (cx + ix == stride ? 1 : 0),
									cy + iy == -1 ? -1 : (cy + iy == stride ? 1 : 0),
									cz + iz == -1 ? -1 : (cz + iz == stride ? 1 : 0)
								};
								const u32 isolevel_index = 9 * (chunk_offset.z + 1) + 3 * (chunk_offset.y + 1) + (chunk_offset.x + 1);
								const tg_transvoxel_isolevels* p_isolevel = pp_connecting_isolevels[isolevel_index];
								if (p_isolevel)
								{
									p_temp_cells[i].triangle_count = tg_transvoxel_internal_create_cell(
										p_connecting_chunk_coordinates[i].x, p_connecting_chunk_coordinates[i].y, p_connecting_chunk_coordinates[i].z,
										p_connecting_cell_coordinates[i].x, p_connecting_cell_coordinates[i].y, p_connecting_cell_coordinates[i].z,
										p_isolevel, lod, 0, p_temp_cells[i].p_triangles
									);
									pp_connecting_cells[i] = &p_temp_cells[i];
								}
							}
						}
					}
				}

				const u32 cell_index = stride * stride * cz + stride * cy + cx;
				for (u32 t = 0; t < p_cells[cell_index].triangle_count; t++)
				{
					for (u32 v = 0; v < 3; v++)
					{
						const v3 pos = p_cells[cell_index].p_triangles[t].p_vertices[v].position;
						if (pos.x == 32.0f && pos.z == -30.0f)
						{
							int iii = 0;
						}
						v3 normal = { 0 };
						for (u32 nc = 0; nc < 27; nc++)
						{
							if (pp_connecting_cells[nc])
							{
								for (u32 nct = 0; nct < pp_connecting_cells[nc]->triangle_count; nct++)
								{
									if (tgm_v3_similar(&p_cells[cell_index].p_triangles[t].p_vertices[v].position, &pp_connecting_cells[nc]->p_triangles[nct].p_vertices[0].position, 0.0f) ||
										tgm_v3_similar(&p_cells[cell_index].p_triangles[t].p_vertices[v].position, &pp_connecting_cells[nc]->p_triangles[nct].p_vertices[1].position, 0.0f) ||
										tgm_v3_similar(&p_cells[cell_index].p_triangles[t].p_vertices[v].position, &pp_connecting_cells[nc]->p_triangles[nct].p_vertices[2].position, 0.0f))
									{
										const v3 n = tg_transvoxel_internal_generate_normal(&pp_connecting_cells[nc]->p_triangles[nct]);
										if (pos.x == 32.0f && pos.z == -30.0f)
										{
											int iii = 0;
										}
										normal = tgm_v3_add_v3(&normal, &n);
									}
								}
							}
						}
						normal = tgm_v3_normalized(&normal);
						p_cells[cell_index].p_triangles[t].p_vertices[v].normal = normal;
					}
				}
			}
		}
	}

	for (i8 cz = 0; cz < stride; cz++)
	{
		for (i8 cy = 0; cy < stride; cy++)
		{
			for (i8 cx = 0; cx < stride; cx++)
			{
				if (!((transition_faces & TG_TRANSVOXEL_FACE_X_NEG) && cx == 0))
				{
					continue;
				}

				const u32 cell_index = stride * stride * cz + stride * cy + cx;
				for (u32 t = 0; t < p_cells[cell_index].triangle_count; t++)
				{
					for (u32 v = 0; v < 3; v++)
					{
						const v3 pos = p_cells[cell_index].p_triangles[t].p_vertices[v].position;
						const v3 normal = p_cells[cell_index].p_triangles[t].p_vertices[v].normal;

						m3 mat = { 0 };
						mat.m00 = 1.0f - (normal.x * normal.x);
						mat.m10 = -normal.x * normal.y;
						mat.m20 = -normal.x * normal.z;
						mat.m01 = -normal.x * normal.y;
						mat.m11 = 1.0f - (normal.y * normal.y);
						mat.m21 = -normal.y * normal.z;
						mat.m02 = -normal.x * normal.z;
						mat.m12 = -normal.y * normal.z;
						mat.m22 = 1.0f - (normal.z * normal.z);

						const f32 offx = 16.0f * (f32)x;
						const f32 offy = 16.0f * (f32)y;
						const f32 offz = 16.0f * (f32)z;
						const v3 cell_base = { offx + (f32)(cx * lodf), offy + (f32)(cy * lodf), offz + (f32)(cz * lodf) };
						const v3 rel = tgm_v3_subtract_v3(&p_cells[cell_index].p_triangles[t].p_vertices[v].position, &cell_base);
						const v3 relf = tgm_v3_divide_f(&rel, lodf);

						v3 off = { 0 };
						if ((transition_faces & TG_TRANSVOXEL_FACE_X_NEG) && cx == 0)
						{
							off.x = 0.5f * lodf * (1.0f - relf.x);
							const v3 proj = tgm_m3_multiply_v3(&mat, &off);
							if (pos.x == 32.0f && pos.z == -30.0f)
							{
								int iii = 0;
							}
							p_cells[cell_index].p_triangles[t].p_vertices[v].position = tgm_v3_add_v3(&p_cells[cell_index].p_triangles[t].p_vertices[v].position, &proj);
						}
					}
				}
			}
		}
	}

	TG_MEMORY_FREE(p_data);

	return triangle_count;
}
