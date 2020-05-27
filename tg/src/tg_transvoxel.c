#include "tg_transvoxel.h"
#include "tg_transvoxel_lookup_tables.h"

#include "memory/tg_memory.h"
#include "util/tg_string.h"
#include "platform/tg_platform.h"



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



v3 tg_transvoxel_internal_generate_triangle_normal(const tg_transvoxel_triangle* p_triangle)
{
	const v3 v01 = tgm_v3_subtract(p_triangle->p_vertices[1].position, p_triangle->p_vertices[0].position);
	const v3 v02 = tgm_v3_subtract(p_triangle->p_vertices[2].position, p_triangle->p_vertices[0].position);
	const v3 result = tgm_v3_normalized(tgm_v3_cross(v01, v02));
	return result;
}

v3 tg_transvoxel_internal_generate_vertex_normal(v3 v, const tg_transvoxel_regular_cell* p_connecting_cells)
{
	v3 normal = { 0 };
	
	for (u32 nc = 0; nc < 27; nc++)
	{
		for (u32 nct = 0; nct < p_connecting_cells[nc].triangle_count; nct++)
		{
			if (tgm_v3_equal(v, p_connecting_cells[nc].p_triangles[nct].p_vertices[0].position) ||
				tgm_v3_equal(v, p_connecting_cells[nc].p_triangles[nct].p_vertices[1].position) ||
				tgm_v3_equal(v, p_connecting_cells[nc].p_triangles[nct].p_vertices[2].position))
			{
				const v3 n = tg_transvoxel_internal_generate_triangle_normal(&p_connecting_cells[nc].p_triangles[nct]);
				normal = tgm_v3_add(normal, n);
			}
		}
	}

	normal = tgm_v3_normalized(normal);
	return normal;
}

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

void tg_transvoxel_internal_scale_vertex(
	i32 x, i32 y, i32 z,
	u8 cx, u8 cy, u8 cz,
	u8 lod, u8 transition_faces,
	u8 x_start, u8 y_start, u8 z_start,
	u8 x_end, u8 y_end, u8 z_end,
	tg_vertex_3d* p_vertex)
{
	const u32 cell_index = x_end * y_end * cz + x_end * cy + cx;
	const u8 lodf = 1 << lod;
	const v3 normal = p_vertex->normal;

	m3 projection_matrix = { 0 };
	projection_matrix.m00 = 1.0f - (normal.x * normal.x);
	projection_matrix.m10 = -normal.x * normal.y;
	projection_matrix.m20 = -normal.x * normal.z;
	projection_matrix.m01 = -normal.x * normal.y;
	projection_matrix.m11 = 1.0f - (normal.y * normal.y);
	projection_matrix.m21 = -normal.y * normal.z;
	projection_matrix.m02 = -normal.x * normal.z;
	projection_matrix.m12 = -normal.y * normal.z;
	projection_matrix.m22 = 1.0f - (normal.z * normal.z);

	const v3 cell_base = { (f32)(16 * x + cx * lodf), (f32)(16 * y + cy * lodf), (f32)(16 * z + cz * lodf) };
	const v3 rel = tgm_v3_subtract(p_vertex->position, cell_base);
	v3 relf = tgm_v3_divide_f(rel, lodf);
	relf = tgm_v3_abs(relf);// TODO: do we need abs?

	const b32 scale_x_neg = cx == x_start && !(transition_faces & TG_TRANSVOXEL_FACE_X_NEG);
	const b32 scale_x_pos = cx == x_end - 1 && !(transition_faces & TG_TRANSVOXEL_FACE_X_POS);
	const b32 scale_y_neg = cy == y_start && !(transition_faces & TG_TRANSVOXEL_FACE_Y_NEG);
	const b32 scale_y_pos = cy == y_end - 1 && !(transition_faces & TG_TRANSVOXEL_FACE_Y_POS);
	const b32 scale_z_neg = cz == z_start && !(transition_faces & TG_TRANSVOXEL_FACE_Z_NEG);
	const b32 scale_z_pos = cz == z_end - 1 && !(transition_faces & TG_TRANSVOXEL_FACE_Z_POS);

	const f32 scale_x_factor = scale_x_neg ? relf.x : (scale_x_pos ? 1.0f - relf.x : 1.0f);
	const f32 scale_y_factor = scale_y_neg ? relf.y : (scale_y_pos ? 1.0f - relf.y : 1.0f);
	const f32 scale_z_factor = scale_z_neg ? relf.z : (scale_z_pos ? 1.0f - relf.z : 1.0f);

	v3 offset = { 0 };
	if ((transition_faces & TG_TRANSVOXEL_FACE_X_NEG) && cx == 0)
	{
		offset.x = 0.5f * lodf * (1.0f - relf.x) * scale_y_factor * scale_z_factor;
	}
	else if ((transition_faces & TG_TRANSVOXEL_FACE_X_POS) && cx == x_end - 1)
	{
		offset.x = -0.5f * lodf * relf.x * scale_y_factor * scale_z_factor;
	}
	if ((transition_faces & TG_TRANSVOXEL_FACE_Y_NEG) && cy == 0)
	{
		offset.y = 0.5f * lodf * (1.0f - relf.y) * scale_x_factor * scale_z_factor;
	}
	else if ((transition_faces & TG_TRANSVOXEL_FACE_Y_POS) && cy == y_end - 1)
	{
		offset.y = -0.5f * lodf * relf.y * scale_x_factor * scale_z_factor;
	}
	if ((transition_faces & TG_TRANSVOXEL_FACE_Z_NEG) && cz == 0)
	{
		offset.z = 0.5f * lodf * (1.0f - relf.z) * scale_x_factor * scale_y_factor;
	}
	if ((transition_faces & TG_TRANSVOXEL_FACE_Z_POS) && cz == z_end - 1)
	{
		offset.z = -0.5f * lodf * relf.z * scale_x_factor * scale_y_factor;
	}

	const v3 projected_offset = tgm_m3_multiply_v3(projection_matrix, offset);
	p_vertex->position = tgm_v3_add(p_vertex->position, projected_offset);
}

tg_transvoxel_transition_cell tg_transvoxel_internal_create_transition_cell(u8 transition_face, i32 x, i32 y, i32 z, u8 cx, u8 cy, u8 cz, const tg_transvoxel_isolevels* p_isolevels, u8 lod, u8 transition_faces, tg_transvoxel_triangle* p_triangles)
{
	tg_transvoxel_transition_cell cell = { 0 };

	transition_faces &= ~transition_face;

	const u32 cell_scale = 1 << lod;
	const u8 neighbour_cell_lod = lod - 1;
	const u32 neighbour_cell_scale = 1 << neighbour_cell_lod;

	v3i p_indices[13] = { 0 };
	v3 p_positions[13] = { 0 };

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
		p_indices[0x0] = (v3i){ 0, cell_scale * cy                           , cell_scale * cz                            };
		p_indices[0x1] = (v3i){ 0, cell_scale * cy                           , cell_scale * cz + neighbour_cell_scale * 1 };
		p_indices[0x2] = (v3i){ 0, cell_scale * cy                           , cell_scale * cz + neighbour_cell_scale * 2 };
		p_indices[0x3] = (v3i){ 0, cell_scale * cy + neighbour_cell_scale * 1, cell_scale * cz                            };
		p_indices[0x4] = (v3i){ 0, cell_scale * cy + neighbour_cell_scale * 1, cell_scale * cz + neighbour_cell_scale * 1 };
		p_indices[0x5] = (v3i){ 0, cell_scale * cy + neighbour_cell_scale * 1, cell_scale * cz + neighbour_cell_scale * 2 };
		p_indices[0x6] = (v3i){ 0, cell_scale * cy + neighbour_cell_scale * 2, cell_scale * cz                            };
		p_indices[0x7] = (v3i){ 0, cell_scale * cy + neighbour_cell_scale * 2, cell_scale * cz + neighbour_cell_scale * 1 };
		p_indices[0x8] = (v3i){ 0, cell_scale * cy + neighbour_cell_scale * 2, cell_scale * cz + neighbour_cell_scale * 2 };
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
		p_indices[0x0] = (v3i){ 16, cell_scale * cy                           , cell_scale * cz                            };
		p_indices[0x1] = (v3i){ 16, cell_scale * cy + neighbour_cell_scale * 1, cell_scale * cz                            };
		p_indices[0x2] = (v3i){ 16, cell_scale * cy + neighbour_cell_scale * 2, cell_scale * cz                            };
		p_indices[0x3] = (v3i){ 16, cell_scale * cy                           , cell_scale * cz + neighbour_cell_scale * 1 };
		p_indices[0x4] = (v3i){ 16, cell_scale * cy + neighbour_cell_scale * 1, cell_scale * cz + neighbour_cell_scale * 1 };
		p_indices[0x5] = (v3i){ 16, cell_scale * cy + neighbour_cell_scale * 2, cell_scale * cz + neighbour_cell_scale * 1 };
		p_indices[0x6] = (v3i){ 16, cell_scale * cy                           , cell_scale * cz + neighbour_cell_scale * 2 };
		p_indices[0x7] = (v3i){ 16, cell_scale * cy + neighbour_cell_scale * 1, cell_scale * cz + neighbour_cell_scale * 2 };
		p_indices[0x8] = (v3i){ 16, cell_scale * cy + neighbour_cell_scale * 2, cell_scale * cz + neighbour_cell_scale * 2 };
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
		p_indices[0x0] = (v3i){ cell_scale * cx                           , 0, cell_scale * cz                            };
		p_indices[0x1] = (v3i){ cell_scale * cx + neighbour_cell_scale * 1, 0, cell_scale * cz                            };
		p_indices[0x2] = (v3i){ cell_scale * cx + neighbour_cell_scale * 2, 0, cell_scale * cz                            };
		p_indices[0x3] = (v3i){ cell_scale * cx                           , 0, cell_scale * cz + neighbour_cell_scale * 1 };
		p_indices[0x4] = (v3i){ cell_scale * cx + neighbour_cell_scale * 1, 0, cell_scale * cz + neighbour_cell_scale * 1 };
		p_indices[0x5] = (v3i){ cell_scale * cx + neighbour_cell_scale * 2, 0, cell_scale * cz + neighbour_cell_scale * 1 };
		p_indices[0x6] = (v3i){ cell_scale * cx                           , 0, cell_scale * cz + neighbour_cell_scale * 2 };
		p_indices[0x7] = (v3i){ cell_scale * cx + neighbour_cell_scale * 1, 0, cell_scale * cz + neighbour_cell_scale * 2 };
		p_indices[0x8] = (v3i){ cell_scale * cx + neighbour_cell_scale * 2, 0, cell_scale * cz + neighbour_cell_scale * 2 };
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
		p_indices[0x0] = (v3i){ cell_scale * cx                           , 16, cell_scale * cz                            };
		p_indices[0x1] = (v3i){ cell_scale * cx                           , 16, cell_scale * cz + neighbour_cell_scale * 1 };
		p_indices[0x2] = (v3i){ cell_scale * cx                           , 16, cell_scale * cz + neighbour_cell_scale * 2 };
		p_indices[0x3] = (v3i){ cell_scale * cx + neighbour_cell_scale * 1, 16, cell_scale * cz                            };
		p_indices[0x4] = (v3i){ cell_scale * cx + neighbour_cell_scale * 1, 16, cell_scale * cz + neighbour_cell_scale * 1 };
		p_indices[0x5] = (v3i){ cell_scale * cx + neighbour_cell_scale * 1, 16, cell_scale * cz + neighbour_cell_scale * 2 };
		p_indices[0x6] = (v3i){ cell_scale * cx + neighbour_cell_scale * 2, 16, cell_scale * cz                            };
		p_indices[0x7] = (v3i){ cell_scale * cx + neighbour_cell_scale * 2, 16, cell_scale * cz + neighbour_cell_scale * 1 };
		p_indices[0x8] = (v3i){ cell_scale * cx + neighbour_cell_scale * 2, 16, cell_scale * cz + neighbour_cell_scale * 2 };
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
		p_indices[0x0] = (v3i){ cell_scale * cx                           , cell_scale * cy                           , 0 };
		p_indices[0x1] = (v3i){ cell_scale * cx                           , cell_scale * cy + neighbour_cell_scale * 1, 0 };
		p_indices[0x2] = (v3i){ cell_scale * cx                           , cell_scale * cy + neighbour_cell_scale * 2, 0 };
		p_indices[0x3] = (v3i){ cell_scale * cx + neighbour_cell_scale * 1, cell_scale * cy                           , 0 };
		p_indices[0x4] = (v3i){ cell_scale * cx + neighbour_cell_scale * 1, cell_scale * cy + neighbour_cell_scale * 1, 0 };
		p_indices[0x5] = (v3i){ cell_scale * cx + neighbour_cell_scale * 1, cell_scale * cy + neighbour_cell_scale * 2, 0 };
		p_indices[0x6] = (v3i){ cell_scale * cx + neighbour_cell_scale * 2, cell_scale * cy                           , 0 };
		p_indices[0x7] = (v3i){ cell_scale * cx + neighbour_cell_scale * 2, cell_scale * cy + neighbour_cell_scale * 1, 0 };
		p_indices[0x8] = (v3i){ cell_scale * cx + neighbour_cell_scale * 2, cell_scale * cy + neighbour_cell_scale * 2, 0 };
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
		p_indices[0x0] = (v3i){ cell_scale * cx                           , cell_scale * cy                           , 16 };
		p_indices[0x1] = (v3i){ cell_scale * cx + neighbour_cell_scale * 1, cell_scale * cy                           , 16 };
		p_indices[0x2] = (v3i){ cell_scale * cx + neighbour_cell_scale * 2, cell_scale * cy                           , 16 };
		p_indices[0x3] = (v3i){ cell_scale * cx                           , cell_scale * cy + neighbour_cell_scale * 1, 16 };
		p_indices[0x4] = (v3i){ cell_scale * cx + neighbour_cell_scale * 1, cell_scale * cy + neighbour_cell_scale * 1, 16 };
		p_indices[0x5] = (v3i){ cell_scale * cx + neighbour_cell_scale * 2, cell_scale * cy + neighbour_cell_scale * 1, 16 };
		p_indices[0x6] = (v3i){ cell_scale * cx                           , cell_scale * cy + neighbour_cell_scale * 2, 16 };
		p_indices[0x7] = (v3i){ cell_scale * cx + neighbour_cell_scale * 1, cell_scale * cy + neighbour_cell_scale * 2, 16 };
		p_indices[0x8] = (v3i){ cell_scale * cx + neighbour_cell_scale * 2, cell_scale * cy + neighbour_cell_scale * 2, 16 };
	} break;
	}

	p_indices[0x9] = p_indices[0x0];
	p_indices[0xa] = p_indices[0x2];
	p_indices[0xb] = p_indices[0x6];
	p_indices[0xc] = p_indices[0x8];

	const u32 case_code =
		(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_indices[0x0]) < 0 ? 0x01  : 0) |
		(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_indices[0x1]) < 0 ? 0x02  : 0) |
		(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_indices[0x2]) < 0 ? 0x04  : 0) |
		(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_indices[0x3]) < 0 ? 0x80  : 0) |
		(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_indices[0x4]) < 0 ? 0x100 : 0) |
		(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_indices[0x5]) < 0 ? 0x08  : 0) |
		(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_indices[0x6]) < 0 ? 0x40  : 0) |
		(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_indices[0x7]) < 0 ? 0x20  : 0) |
		(TG_TRANSVOXEL_ISOLEVEL_AT_V3I(*p_isolevels, p_indices[0x8]) < 0 ? 0x10  : 0);

	if (case_code != 0 && case_code != 511)
	{
		const u8 equivalence_class_index = p_transition_cell_class[case_code];
		const u8 inverted = (equivalence_class_index & 0x80) >> 7;

		const tg_transition_cell_data* p_cell_data = &p_transition_cell_data[equivalence_class_index & 0x7f];
		const u16* p_vertex_data = p_transition_vertex_data[case_code];

		u32 triangle_count = TG_CELL_GET_TRIANGLE_COUNT(*p_cell_data);
		cell.p_triangles = p_triangles;

		for (u8 i = 0; i < 13; i++)
		{
			p_positions[i] = (v3){ (f32)(16 * x + p_indices[i].x), (f32)(16 * y + p_indices[i].y), (f32)(16 * z + p_indices[i].z) };
		}

		for (u32 i = 0; i < triangle_count; i++)
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

			const v3 vec0 = tg_transvoxel_internal_interpolate(p_isolevels, e0v0 < 9 ? neighbour_cell_lod : lod, p_indices[e0v0], p_indices[e0v1], p_positions[e0v0], p_positions[e0v1]);
			const v3 vec1 = tg_transvoxel_internal_interpolate(p_isolevels, e1v0 < 9 ? neighbour_cell_lod : lod, p_indices[e1v0], p_indices[e1v1], p_positions[e1v0], p_positions[e1v1]);
			const v3 vec2 = tg_transvoxel_internal_interpolate(p_isolevels, e2v0 < 9 ? neighbour_cell_lod : lod, p_indices[e2v0], p_indices[e2v1], p_positions[e2v0], p_positions[e2v1]);

			if (!inverted)
			{
				p_triangles[cell.triangle_count].p_vertices[0].position = vec0;
				p_triangles[cell.triangle_count].p_vertices[1].position = vec2;
				p_triangles[cell.triangle_count].p_vertices[2].position = vec1;
				cell.low_res_vertex_bitmap |= e0v0 >= 9 ? (1LL << (3LL * (u64)cell.triangle_count)) : 0LL;
				cell.low_res_vertex_bitmap |= e2v0 >= 9 ? (1LL << (3LL * (u64)cell.triangle_count + 1LL)) : 0LL;
				cell.low_res_vertex_bitmap |= e1v0 >= 9 ? (1LL << (3LL * (u64)cell.triangle_count + 2LL)) : 0LL;
			}
			else
			{
				p_triangles[cell.triangle_count].p_vertices[0].position = vec0;
				p_triangles[cell.triangle_count].p_vertices[1].position = vec1;
				p_triangles[cell.triangle_count].p_vertices[2].position = vec2;
				cell.low_res_vertex_bitmap |= e0v0 >= 9 ? (1LL << (3LL * (u64)cell.triangle_count      )) : 0LL;
				cell.low_res_vertex_bitmap |= e1v0 >= 9 ? (1LL << (3LL * (u64)cell.triangle_count + 1LL)) : 0LL;
				cell.low_res_vertex_bitmap |= e2v0 >= 9 ? (1LL << (3LL * (u64)cell.triangle_count + 2LL)) : 0LL;
			}

			cell.triangle_count++;
		}
	}

	return cell;
}

void tg_transvoxel_internal_fill_regular_cell(tg_transvoxel_regular_cell* p_cell, i32 x, i32 y, i32 z, u8 cx, u8 cy, u8 cz, tg_transvoxel_isolevels* p_isolevels, u8 lod, tg_transvoxel_triangle* p_triangles)
{
	const u8 cell_scale = 1 << lod;

	const u8 cell_count_x = TG_TRANSVOXEL_CELLS_X / cell_scale;
	const u8 cell_count_y = TG_TRANSVOXEL_CELLS_Y / cell_scale;
	const u8 cell_count_z = TG_TRANSVOXEL_CELLS_Z / cell_scale;

	p_cell->triangle_count = 0;
	p_cell->p_triangles = p_triangles;

	const v3i p_isolevel_indices[8] = {
		{ (cx    ) * cell_scale, (cy    ) * cell_scale, (cz    ) * cell_scale },
		{ (cx + 1) * cell_scale, (cy    ) * cell_scale, (cz    ) * cell_scale },
		{ (cx    ) * cell_scale, (cy + 1) * cell_scale, (cz    ) * cell_scale },
		{ (cx + 1) * cell_scale, (cy + 1) * cell_scale, (cz    ) * cell_scale },
		{ (cx    ) * cell_scale, (cy    ) * cell_scale, (cz + 1) * cell_scale },
		{ (cx + 1) * cell_scale, (cy    ) * cell_scale, (cz + 1) * cell_scale },
		{ (cx    ) * cell_scale, (cy + 1) * cell_scale, (cz + 1) * cell_scale },
		{ (cx + 1) * cell_scale, (cy + 1) * cell_scale, (cz + 1) * cell_scale }
	};

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

		const u32 triangle_count = TG_CELL_GET_TRIANGLE_COUNT(*p_cell_data);

		const f32 offx = 16.0f * (f32)x;
		const f32 offy = 16.0f * (f32)y;
		const f32 offz = 16.0f * (f32)z;

		v3 p_positions[8] = { 0 };
		p_positions[0] = (v3){ offx + (f32)( cx      * cell_scale), offy + (f32)( cy      * cell_scale), offz + (f32)( cz      * cell_scale) };
		p_positions[1] = (v3){ offx + (f32)((cx + 1) * cell_scale), offy + (f32)( cy      * cell_scale), offz + (f32)( cz      * cell_scale) };
		p_positions[2] = (v3){ offx + (f32)( cx      * cell_scale), offy + (f32)((cy + 1) * cell_scale), offz + (f32)( cz      * cell_scale) };
		p_positions[3] = (v3){ offx + (f32)((cx + 1) * cell_scale), offy + (f32)((cy + 1) * cell_scale), offz + (f32)( cz      * cell_scale) };
		p_positions[4] = (v3){ offx + (f32)( cx      * cell_scale), offy + (f32)( cy      * cell_scale), offz + (f32)((cz + 1) * cell_scale) };
		p_positions[5] = (v3){ offx + (f32)((cx + 1) * cell_scale), offy + (f32)( cy      * cell_scale), offz + (f32)((cz + 1) * cell_scale) };
		p_positions[6] = (v3){ offx + (f32)( cx      * cell_scale), offy + (f32)((cy + 1) * cell_scale), offz + (f32)((cz + 1) * cell_scale) };
		p_positions[7] = (v3){ offx + (f32)((cx + 1) * cell_scale), offy + (f32)((cy + 1) * cell_scale), offz + (f32)((cz + 1) * cell_scale) };

		for (u8 i = 0; i < triangle_count; i++)
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

			v3 vec0 = tg_transvoxel_internal_interpolate(p_isolevels, lod, e00i, e01i, e00p, e01p);
			v3 vec1 = tg_transvoxel_internal_interpolate(p_isolevels, lod, e10i, e11i, e10p, e11p);
			v3 vec2 = tg_transvoxel_internal_interpolate(p_isolevels, lod, e20i, e21i, e20p, e21p);

			if (!tgm_v3_equal(vec0, vec1) && !tgm_v3_equal(vec0, vec2) && !tgm_v3_equal(vec1, vec2))
			{
				const v3 v01 = tgm_v3_subtract(vec1, vec0);
				const v3 v02 = tgm_v3_subtract(vec2, vec0);
				const v3 n = tgm_v3_normalized(tgm_v3_cross(v01, v02));

				p_cell->p_triangles[p_cell->triangle_count].p_vertices[0].position = vec0;
				p_cell->p_triangles[p_cell->triangle_count].p_vertices[1].position = vec1;
				p_cell->p_triangles[p_cell->triangle_count].p_vertices[2].position = vec2;
				p_cell->p_triangles[p_cell->triangle_count].p_vertices[0].normal = n;
				p_cell->p_triangles[p_cell->triangle_count].p_vertices[1].normal = n;
				p_cell->p_triangles[p_cell->triangle_count].p_vertices[2].normal = n;

				p_cell->triangle_count++;
			}
		}
	}
}

void tg_transvoxel_internal_find_connecting_cells(tg_transvoxel_connecting_chunks* p_connecting_chunks, u8 cx, u8 cy, u8 cz, tg_transvoxel_triangle* p_transient_triangle_buffer, tg_transvoxel_regular_cell* p_connecting_cells)
{
	const tg_transvoxel_chunk* p_central_chunk = p_connecting_chunks->p_central_chunk;

	const u8 cell_scale = 1 << p_central_chunk->lod;

	const u8 cell_count_x = TG_TRANSVOXEL_CELLS_X / cell_scale;
	const u8 cell_count_y = TG_TRANSVOXEL_CELLS_Y / cell_scale;
	const u8 cell_count_z = TG_TRANSVOXEL_CELLS_Z / cell_scale;

	u32 connecting_cells_index = 0;
	for (i8 iz = -1; iz < 2; iz++)
	{
		for (i8 iy = -1; iy < 2; iy++)
		{
			for (i8 ix = -1; ix < 2; ix++)
			{
				const v3i connecting_chunk_offset = {
					cx + ix == -1 ? -1 : (cx + ix == cell_count_x ? 1 : 0),
					cy + iy == -1 ? -1 : (cy + iy == cell_count_y ? 1 : 0),
					cz + iz == -1 ? -1 : (cz + iz == cell_count_z ? 1 : 0)
				};
				const v3i connecting_cell_coordinates = {
					cx + ix == -1 ? cell_count_x - 1 : (cx + ix == cell_count_x ? 0 : cx + ix),
					cy + iy == -1 ? cell_count_y - 1 : (cy + iy == cell_count_y ? 0 : cy + iy),
					cz + iz == -1 ? cell_count_z - 1 : (cz + iz == cell_count_z ? 0 : cz + iz),
				};

				const u8 connecting_chunk_index = 9 * (1 + connecting_chunk_offset.z) + 3 * (1 + connecting_chunk_offset.y) + (1 + connecting_chunk_offset.x);
				tg_transvoxel_chunk* p_connecting_chunk = p_connecting_chunks->pp_chunks[connecting_chunk_index];

				TG_ASSERT(p_connecting_chunk);

				if (p_connecting_chunk)
				{
					if (p_connecting_chunk->lod == p_central_chunk->lod)
					{
						p_connecting_cells[connecting_cells_index] = TG_TRANSVOXEL_REGULAR_CELL_AT(*p_connecting_chunk, connecting_cell_coordinates.x, connecting_cell_coordinates.y, connecting_cell_coordinates.z);
					}
					else
					{
						tg_transvoxel_internal_fill_regular_cell(
							&p_connecting_cells[connecting_cells_index],
							p_connecting_chunk->coordinates.x, p_connecting_chunk->coordinates.y, p_connecting_chunk->coordinates.z,
							cx, cy, cz,
							&p_connecting_chunk->isolevels, p_central_chunk->lod, p_transient_triangle_buffer
						);
					}
				}
				connecting_cells_index++;
			}
		}
	}
}

u32 tg_transvoxel_internal_create_transition_chunk(u8 transition_face, i32 x, i32 y, i32 z, const tg_transvoxel_isolevels* p_isolevels, const tg_transvoxel_isolevels** pp_connecting_isolevels, u8 lod, u8 transition_faces, tg_transvoxel_triangle* p_triangles)
{
	u32 triangle_count = 0;

	const u8 cell_scale = 1 << lod;
	const u8 regular_cell_count = 16 / cell_scale;

	const u8 x_start = transition_face == TG_TRANSVOXEL_FACE_X_POS ? 16 / cell_scale - 1 : 0;
	const u8 y_start = transition_face == TG_TRANSVOXEL_FACE_Y_POS ? 16 / cell_scale - 1 : 0;
	const u8 z_start = transition_face == TG_TRANSVOXEL_FACE_Z_POS ? 16 / cell_scale - 1 : 0;

	const u8 x_end = transition_face == TG_TRANSVOXEL_FACE_X_NEG ? 1 : 16 / cell_scale;
	const u8 y_end = transition_face == TG_TRANSVOXEL_FACE_Y_NEG ? 1 : 16 / cell_scale;
	const u8 z_end = transition_face == TG_TRANSVOXEL_FACE_Z_NEG ? 1 : 16 / cell_scale;

	const u8 stride_x = x_end - x_start;
	const u8 stride_y = y_end - y_start;
	const u8 stride_z = z_end - z_start;

	const u64 cells_size = (u64)stride_x * (u64)stride_y * (u64)stride_z * sizeof(tg_transvoxel_transition_cell);
	tg_transvoxel_transition_cell* p_cells = TG_MEMORY_STACK_ALLOC(cells_size);
	u32 cell_index = 0;
	for (u8 cz = z_start; cz < z_end; cz++)
	{
		for (u8 cy = y_start; cy < y_end; cy++)
		{
			for (u8 cx = x_start; cx < x_end; cx++)
			{
				p_cells[cell_index] = tg_transvoxel_internal_create_transition_cell(transition_face, x, y, z, cx, cy, cz, p_isolevels, lod, transition_faces, &p_triangles[triangle_count]);
				triangle_count += p_cells[cell_index].triangle_count;
				cell_index++;
			}
		}
	}

	const v3i chunk_coordinates = { x, y, z };
	const u64 connecting_cells_triangles_size = 27 * TG_TRANSVOXEL_REGULAR_CELL_MAX_TRIANGLES * sizeof(tg_transvoxel_triangle);
	tg_transvoxel_triangle* p_connecting_cells_triangles = TG_MEMORY_STACK_ALLOC(connecting_cells_triangles_size);
	tg_transvoxel_regular_cell p_connecting_cells[27] = { 0 };
	cell_index = 0;
	for (u8 cz = z_start; cz < z_end; cz++)
	{
		for (u8 cy = y_start; cy < y_end; cy++)
		{
			for (u8 cx = x_start; cx < x_end; cx++)
			{
				// TODO: the whole loop can be combined with loop above
				for (i8 iz = -1; iz < 2; iz++)
				{
					for (i8 iy = -1; iy < 2; iy++)
					{
						for (i8 ix = -1; ix < 2; ix++)
						{
							const v3i regular_chunk_offset = {
								cx + ix == -1 ? -1 : (cx + ix == regular_cell_count ? 1 : 0),
								cy + iy == -1 ? -1 : (cy + iy == regular_cell_count ? 1 : 0),
								cz + iz == -1 ? -1 : (cz + iz == regular_cell_count ? 1 : 0)
							};
							const v3i connecting_regular_chunk_coordinates = {
								x + regular_chunk_offset.x,
								y + regular_chunk_offset.y,
								z + regular_chunk_offset.z
							};
							const v3i connecting_regular_cell_coordinates = {
								cx + ix == -1 ? regular_cell_count - 1 : (cx + ix == regular_cell_count ? 0 : cx + ix),
								cy + iy == -1 ? regular_cell_count - 1 : (cy + iy == regular_cell_count ? 0 : cy + iy),
								cz + iz == -1 ? regular_cell_count - 1 : (cz + iz == regular_cell_count ? 0 : cz + iz)
							};

							const u8 connecting_regular_cell_index = 9 * (iz + 1) + 3 * (iy + 1) + (ix + 1);
							const u8 connecting_isolevel_index = 9 * (regular_chunk_offset.z + 1) + 3 * (regular_chunk_offset.y + 1) + (regular_chunk_offset.x + 1);
							const tg_transvoxel_isolevels* p_connecting_isolevels = pp_connecting_isolevels[connecting_isolevel_index];
							if (p_connecting_isolevels)
							{
								//p_connecting_cells[connecting_regular_cell_index] = tg_transvoxel_internal_fill_regular_cell(
								//	connecting_regular_chunk_coordinates.x, connecting_regular_chunk_coordinates.y, connecting_regular_chunk_coordinates.z,
								//	connecting_regular_cell_coordinates.x, connecting_regular_cell_coordinates.y, connecting_regular_cell_coordinates.z,
								//	p_connecting_isolevels, lod, 0, &p_connecting_cells_triangles[connecting_regular_cell_index * TG_TRANSVOXEL_REGULAR_CELL_MAX_TRIANGLES]
								//);
							}
							else
							{
								p_connecting_cells[connecting_regular_cell_index].triangle_count = 0;
								p_connecting_cells[connecting_regular_cell_index].p_triangles = TG_NULL;
							}
						}
					}
				}

				for (u32 triangle_index = 0; triangle_index < p_cells[cell_index].triangle_count; triangle_index++)
				{
					for (u8 vertex_index = 0; vertex_index < 3; vertex_index++)
					{
						if (p_cells[cell_index].low_res_vertex_bitmap & (1 << (3 * triangle_index + vertex_index)))
						{
							const v3 vertex_normal = tg_transvoxel_internal_generate_vertex_normal(p_cells[cell_index].p_triangles[triangle_index].p_vertices[vertex_index].position, p_connecting_cells);
							p_cells[cell_index].p_triangles[triangle_index].p_vertices[vertex_index].normal = vertex_normal;
							tg_transvoxel_internal_scale_vertex(
								x, y, z,
								cx, cy, cz,
								lod, transition_faces,
								x_start, y_start, z_start,
								x_end, y_end, z_end,
								&p_cells[cell_index].p_triangles[triangle_index].p_vertices[vertex_index]
							);
						}
					}
					const v3 v01 = tgm_v3_subtract(p_cells[cell_index].p_triangles[triangle_index].p_vertices[1].position, p_cells[cell_index].p_triangles[triangle_index].p_vertices[0].position);
					const v3 v02 = tgm_v3_subtract(p_cells[cell_index].p_triangles[triangle_index].p_vertices[2].position, p_cells[cell_index].p_triangles[triangle_index].p_vertices[0].position);
					const v3 cross = tgm_v3_cross(v01, v02);
					if (tgm_v3_magnitude_squared(cross) != 0.0f)
					{
						const v3 triangle_normal = tg_transvoxel_internal_generate_triangle_normal(&p_cells[cell_index].p_triangles[triangle_index]);
						p_cells[cell_index].p_triangles[triangle_index].p_vertices[0].normal = triangle_normal;
						p_cells[cell_index].p_triangles[triangle_index].p_vertices[1].normal = triangle_normal;
						p_cells[cell_index].p_triangles[triangle_index].p_vertices[2].normal = triangle_normal;
					}
					else
					{
						// TODO: triangle is degenerate and can be removed!
					}
				}
				cell_index++;
			}
		}
	}
	TG_MEMORY_STACK_FREE(cells_size);
	TG_MEMORY_STACK_FREE(connecting_cells_triangles_size);

	return triangle_count;
}

u32 tg_transvoxel_internal_create_regular_chunk(i32 x, i32 y, i32 z, const tg_transvoxel_isolevels* p_isolevels, const tg_transvoxel_isolevels** pp_connecting_isolevels, u8 lod, u8 transition_faces, tg_transvoxel_triangle* p_triangles)
{
	//u32 triangle_count = 0;
	//
	//const u8 cell_scale = 1 << lod;
	//const u8 cell_count = 16 / cell_scale;
	//
	//const u64 cells_size = (u64)cell_count * (u64)cell_count * (u64)cell_count * sizeof(tg_transvoxel_regular_cell);
	//tg_transvoxel_regular_cell* p_cells = TG_MEMORY_STACK_ALLOC(cells_size);
	//
	//const u64 temp_triangles_size = 27 * 5 * sizeof(tg_transvoxel_triangle);
	//tg_transvoxel_triangle* p_temp_triangles = TG_MEMORY_STACK_ALLOC(temp_triangles_size);
	//
	//const u64 temp_cells_size = 27 * sizeof(tg_transvoxel_regular_cell);
	//tg_transvoxel_regular_cell* p_temp_cells = TG_MEMORY_STACK_ALLOC(temp_cells_size);
	//
	//for (u8 i = 0; i < 27; i++)
	//{
	//	p_temp_cells[i].p_triangles = &p_temp_triangles[5 * i];
	//}
	//
	//for (u8 cz = 0; cz < cell_count; cz++)
	//{
	//	for (u8 cy = 0; cy < cell_count; cy++)
	//	{
	//		for (u8 cx = 0; cx < cell_count; cx++)
	//		{
	//			const u32 cell_index = cell_count * cell_count * cz + cell_count * cy + cx;
	//			//p_cells[cell_index] = tg_transvoxel_internal_fill_regular_cell(x, y, z, cx, cy, cz, p_isolevels, lod, transition_faces, &p_triangles[triangle_count]);
	//			triangle_count += p_cells[cell_index].triangle_count;
	//		}
	//	}
	//}
	//
	//tg_transvoxel_regular_cell p_connecting_cells[27] = { 0 };
	//for (u8 cz = 0; cz < cell_count; cz++)
	//{
	//	const b32 cond_z = ((transition_faces & TG_TRANSVOXEL_FACE_Z_NEG) && cz == 0) || ((transition_faces & TG_TRANSVOXEL_FACE_Z_POS) && cz == cell_count - 1);
	//	for (u8 cy = 0; cy < cell_count; cy++)
	//	{
	//		const b32 cond_y = ((transition_faces & TG_TRANSVOXEL_FACE_Y_NEG) && cy == 0) || ((transition_faces & TG_TRANSVOXEL_FACE_Y_POS) && cy == cell_count - 1);
	//		for (u8 cx = 0; cx < cell_count; cx++)
	//		{
	//			const b32 cond_x = ((transition_faces & TG_TRANSVOXEL_FACE_X_NEG) && cx == 0) || ((transition_faces & TG_TRANSVOXEL_FACE_X_POS) && cx == cell_count - 1);
	//			if (cond_x || cond_y || cond_z)
	//			{
	//				tg_transvoxel_internal_find_connecting_cells(x, y, z, cx, cy, cz, lod, pp_connecting_isolevels, p_cells, cell_count, cell_count, cell_count, p_temp_cells, p_connecting_cells);
	//
	//				const u32 cell_index = cell_count * cell_count * cz + cell_count * cy + cx;
	//				for (u32 t = 0; t < p_cells[cell_index].triangle_count; t++)
	//				{
	//					const v3 n0 = tg_transvoxel_internal_generate_vertex_normal(p_cells[cell_index].p_triangles[t].p_vertices[0].position, p_connecting_cells);
	//					const v3 n1 = tg_transvoxel_internal_generate_vertex_normal(p_cells[cell_index].p_triangles[t].p_vertices[1].position, p_connecting_cells);
	//					const v3 n2 = tg_transvoxel_internal_generate_vertex_normal(p_cells[cell_index].p_triangles[t].p_vertices[2].position, p_connecting_cells);
	//
	//					p_cells[cell_index].p_triangles[t].p_vertices[0].normal = n0;
	//					p_cells[cell_index].p_triangles[t].p_vertices[1].normal = n1;
	//					p_cells[cell_index].p_triangles[t].p_vertices[2].normal = n2;
	//				}
	//			}
	//		}
	//	}
	//}
	//
	//for (u8 cz = 0; cz < cell_count; cz++)
	//{
	//	const b32 cond_z = ((transition_faces & TG_TRANSVOXEL_FACE_Z_NEG) && cz == 0) || ((transition_faces & TG_TRANSVOXEL_FACE_Z_POS) && cz == cell_count - 1);
	//	for (u8 cy = 0; cy < cell_count; cy++)
	//	{
	//		const b32 cond_y = ((transition_faces & TG_TRANSVOXEL_FACE_Y_NEG) && cy == 0) || ((transition_faces & TG_TRANSVOXEL_FACE_Y_POS) && cy == cell_count - 1);
	//		for (u8 cx = 0; cx < cell_count; cx++)
	//		{
	//			const b32 cond_x = ((transition_faces & TG_TRANSVOXEL_FACE_X_NEG) && cx == 0) || ((transition_faces & TG_TRANSVOXEL_FACE_X_POS) && cx == cell_count - 1);
	//			if (cond_x || cond_y || cond_z)
	//			{
	//				const u32 cell_index = cell_count * cell_count * cz + cell_count * cy + cx;
	//				for (u32 t = 0; t < p_cells[cell_index].triangle_count; t++)
	//				{
	//					tg_transvoxel_internal_scale_vertex(x, y, z, cx, cy, cz, lod, transition_faces, 0, 0, 0, cell_count, cell_count, cell_count, &p_cells[cell_index].p_triangles[t].p_vertices[0]);
	//					tg_transvoxel_internal_scale_vertex(x, y, z, cx, cy, cz, lod, transition_faces, 0, 0, 0, cell_count, cell_count, cell_count, &p_cells[cell_index].p_triangles[t].p_vertices[1]);
	//					tg_transvoxel_internal_scale_vertex(x, y, z, cx, cy, cz, lod, transition_faces, 0, 0, 0, cell_count, cell_count, cell_count, &p_cells[cell_index].p_triangles[t].p_vertices[2]);
	//
	//					const v3 n = tg_transvoxel_internal_generate_triangle_normal(&p_cells[cell_index].p_triangles[t]);
	//					p_cells[cell_index].p_triangles[t].p_vertices[0].normal = n;
	//					p_cells[cell_index].p_triangles[t].p_vertices[1].normal = n;
	//					p_cells[cell_index].p_triangles[t].p_vertices[2].normal = n;
	//				}
	//			}
	//		}
	//	}
	//}
	//
	//TG_MEMORY_STACK_FREE(temp_cells_size);
	//TG_MEMORY_STACK_FREE(temp_triangles_size);
	//TG_MEMORY_STACK_FREE(cells_size);
	//
	//return triangle_count;
	return 0;
}

void tg_transvoxel_internal_scale_chunk(tg_transvoxel_connecting_chunks* p_connecting_chunks)
{
	tg_transvoxel_chunk* p_central_chunk = p_connecting_chunks->p_central_chunk;

	const i32 x = p_central_chunk->coordinates.x;
	const i32 y = p_central_chunk->coordinates.y;
	const i32 z = p_central_chunk->coordinates.z;
	const u8 lod = p_central_chunk->lod;
	const u8 transition_faces_bitmap = p_central_chunk->transition_faces_bitmap;

	const u8 cell_scale = 1 << lod;
	const u8 cell_count = 16 / cell_scale;

	tg_transvoxel_regular_cell p_connecting_cells[27] = { 0 };
	const u64 transient_triangle_buffer_size = 27 * TG_TRANSVOXEL_REGULAR_CELL_MAX_TRIANGLES * sizeof(tg_transvoxel_triangle);
	tg_transvoxel_triangle* p_transient_triangle_buffer = TG_MEMORY_STACK_ALLOC(transient_triangle_buffer_size);
	for (u8 cz = 0; cz < cell_count; cz++)
	{
		const b32 cond_z = ((transition_faces_bitmap & TG_TRANSVOXEL_FACE_Z_NEG) && cz == 0) || ((transition_faces_bitmap & TG_TRANSVOXEL_FACE_Z_POS) && cz == cell_count - 1);
		for (u8 cy = 0; cy < cell_count; cy++)
		{
			const b32 cond_y = ((transition_faces_bitmap & TG_TRANSVOXEL_FACE_Y_NEG) && cy == 0) || ((transition_faces_bitmap & TG_TRANSVOXEL_FACE_Y_POS) && cy == cell_count - 1);
			for (u8 cx = 0; cx < cell_count; cx++)
			{
				const b32 cond_x = ((transition_faces_bitmap & TG_TRANSVOXEL_FACE_X_NEG) && cx == 0) || ((transition_faces_bitmap & TG_TRANSVOXEL_FACE_X_POS) && cx == cell_count - 1);
				if (cond_x || cond_y || cond_z)
				{
					tg_transvoxel_regular_cell* p_cell = &TG_TRANSVOXEL_REGULAR_CELL_AT(*p_central_chunk, cx, cy, cz);
					if (p_cell->triangle_count)
					{
						b32 breakhere = 0;
						for (u32 t = 0; t < p_cell->triangle_count; t++)
						{
							if (p_cell->p_triangles[t].p_vertices[0].position.x == -32.0f && p_cell->p_triangles[t].p_vertices[0].position.z == -44.0f ||
								p_cell->p_triangles[t].p_vertices[1].position.x == -32.0f && p_cell->p_triangles[t].p_vertices[1].position.z == -44.0f ||
								p_cell->p_triangles[t].p_vertices[2].position.x == -32.0f && p_cell->p_triangles[t].p_vertices[2].position.z == -44.0f)
							{
								breakhere = 1;
							}
						}

						if (breakhere)
						{
							breakhere = 0;
						}

						tg_transvoxel_internal_find_connecting_cells(p_connecting_chunks, cx, cy, cz, p_transient_triangle_buffer, p_connecting_cells);

						for (u32 t = 0; t < p_cell->triangle_count; t++)
						{
							const v3 n0 = tg_transvoxel_internal_generate_vertex_normal(p_cell->p_triangles[t].p_vertices[0].position, p_connecting_cells);
							const v3 n1 = tg_transvoxel_internal_generate_vertex_normal(p_cell->p_triangles[t].p_vertices[1].position, p_connecting_cells);
							const v3 n2 = tg_transvoxel_internal_generate_vertex_normal(p_cell->p_triangles[t].p_vertices[2].position, p_connecting_cells);
					
							p_cell->p_triangles[t].p_vertices[0].normal = n0;
							p_cell->p_triangles[t].p_vertices[1].normal = n1;
							p_cell->p_triangles[t].p_vertices[2].normal = n2;
						}
					}
				}
			}
		}
	}
	TG_MEMORY_STACK_FREE(transient_triangle_buffer_size);

	for (u8 cz = 0; cz < cell_count; cz++)
	{
		const b32 cond_z = ((transition_faces_bitmap & TG_TRANSVOXEL_FACE_Z_NEG) && cz == 0) || ((transition_faces_bitmap & TG_TRANSVOXEL_FACE_Z_POS) && cz == cell_count - 1);
		for (u8 cy = 0; cy < cell_count; cy++)
		{
			const b32 cond_y = ((transition_faces_bitmap & TG_TRANSVOXEL_FACE_Y_NEG) && cy == 0) || ((transition_faces_bitmap & TG_TRANSVOXEL_FACE_Y_POS) && cy == cell_count - 1);
			for (u8 cx = 0; cx < cell_count; cx++)
			{
				const b32 cond_x = ((transition_faces_bitmap & TG_TRANSVOXEL_FACE_X_NEG) && cx == 0) || ((transition_faces_bitmap & TG_TRANSVOXEL_FACE_X_POS) && cx == cell_count - 1);
				if (cond_x || cond_y || cond_z)
				{
					tg_transvoxel_regular_cell* p_cell = &TG_TRANSVOXEL_REGULAR_CELL_AT(*p_central_chunk, cx, cy, cz);
					for (u32 t = 0; t < p_cell->triangle_count; t++)
					{
						tg_transvoxel_internal_scale_vertex(x, y, z, cx, cy, cz, lod, transition_faces_bitmap, 0, 0, 0, cell_count, cell_count, cell_count, &p_cell->p_triangles[t].p_vertices[0]);
						tg_transvoxel_internal_scale_vertex(x, y, z, cx, cy, cz, lod, transition_faces_bitmap, 0, 0, 0, cell_count, cell_count, cell_count, &p_cell->p_triangles[t].p_vertices[1]);
						tg_transvoxel_internal_scale_vertex(x, y, z, cx, cy, cz, lod, transition_faces_bitmap, 0, 0, 0, cell_count, cell_count, cell_count, &p_cell->p_triangles[t].p_vertices[2]);
	
						const v3 n = tg_transvoxel_internal_generate_triangle_normal(&p_cell->p_triangles[t]);
						p_cell->p_triangles[t].p_vertices[0].normal = n;
						p_cell->p_triangles[t].p_vertices[1].normal = n;
						p_cell->p_triangles[t].p_vertices[2].normal = n;
					}
				}
			}
		}
	}
}



void tg_transvoxel_fill_chunk(tg_transvoxel_chunk* p_chunk)
{
	TG_ASSERT(p_chunk);

	const u8 cell_scale = 1 << p_chunk->lod;
	const u8 cell_count = 16 / cell_scale;

	p_chunk->triangle_count = 0;
	for (u8 cz = 0; cz < cell_count; cz++)
	{
		for (u8 cy = 0; cy < cell_count; cy++)
		{
			for (u8 cx = 0; cx < cell_count; cx++)
			{
				tg_transvoxel_internal_fill_regular_cell(
					&TG_TRANSVOXEL_REGULAR_CELL_AT(*p_chunk, cx, cy, cz),
					p_chunk->coordinates.x, p_chunk->coordinates.y, p_chunk->coordinates.z,
					cx, cy, cz,
					&p_chunk->isolevels, p_chunk->lod, &p_chunk->p_triangles[p_chunk->triangle_count]
				);
				p_chunk->triangle_count += TG_TRANSVOXEL_REGULAR_CELL_AT(*p_chunk, cx, cy, cz).triangle_count;
			}
		}
	}
}

void tg_transvoxel_fill_transitions(tg_transvoxel_connecting_chunks* p_connecting_chunks)
{
	TG_ASSERT(p_connecting_chunks);

	if (p_connecting_chunks->p_central_chunk->transition_faces_bitmap)
	{
		tg_transvoxel_internal_scale_chunk(p_connecting_chunks);
	}

	//if (transition_faces & (1 << i))
	//{
	//	tg_transvoxel_internal_create_transition_chunk((1 << i), x, y, z, p_isolevels, pp_connecting_isolevels, lod, transition_faces, &p_triangles[triangle_count]);
	//}
}
