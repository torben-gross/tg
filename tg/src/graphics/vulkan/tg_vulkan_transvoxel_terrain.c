#include "graphics/vulkan/tg_vulkan_transvoxel_terrain.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_transvoxel_lookup_tables.h"

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



// TODO: run length encoding
tg_transvoxel_terrain_internal_fill_voxel_map(tg_transvoxel_block* p_block)
{
	const i32 cell_scale = 1 << p_block->lod;

	const i32 bias = 1024; // TODO: why do it need bias? or rather, why do negative values for noise_x or noise_y result in cliffs?
	const i32 offset_x = TG_TRANSVOXEL_CELLS * p_block->coordinates.x + bias;
	const i32 offset_y = TG_TRANSVOXEL_CELLS * p_block->coordinates.y + bias;
	const i32 offset_z = TG_TRANSVOXEL_CELLS * p_block->coordinates.z + bias;

	for (i32 cz = -1; cz < TG_TRANSVOXEL_REQUIRED_VOXELS_PER_DIMENSION - 1; cz++)
	{
		for (i32 cy = -1; cy < TG_TRANSVOXEL_REQUIRED_VOXELS_PER_DIMENSION - 1; cy++)
		{
			for (i32 cx = -1; cx < TG_TRANSVOXEL_REQUIRED_VOXELS_PER_DIMENSION - 1; cx++)
			{
				const f32 cell_coordinate_x = (f32)(offset_x + cell_scale * cx);
				const f32 cell_coordinate_y = (f32)(offset_y + cell_scale * cy);
				const f32 cell_coordinate_z = (f32)(offset_z + cell_scale * cz);

				const f32 n_hills0 = tgm_noise(cell_coordinate_x * 0.008f, 0.0f, cell_coordinate_z * 0.008f);
				const f32 n_hills1 = tgm_noise(cell_coordinate_x * 0.2f, 0.0f, cell_coordinate_z * 0.2f);
				const f32 n_hills = n_hills0 + 0.005f * n_hills1;

				const f32 s_caves = 0.06f;
				const f32 n_caves = tgm_f32_clamp(tgm_noise(s_caves * cell_coordinate_x, s_caves * cell_coordinate_y, s_caves * cell_coordinate_z), -1.0f, 0.0f);

				const f32 n = n_hills;
				f32 noise = (n * 64.0f) - (f32)(cy + p_block->coordinates.y * 16);
				noise += 60.0f * n_caves;
				const f32 noise_clamped = tgm_f32_clamp(noise, -1.0f, 1.0f);
				const f32 f0 = (noise_clamped + 1.0f) * 0.5f;
				const f32 f1 = 254.0f * f0;
				i8 f2 = -(i8)(tgm_f32_round_to_i32(f1) - 127);
				TG_TRANSVOXEL_VOXEL_MAP_AT(p_block->voxel_map, cx, cy, cz) = f2;
			}
		}
	}
}

tg_transvoxel_terrain_internal_normals(const tg_transvoxel_block* p_block, b32 area_weighting)
{
	const u32 triangle_count = p_block->index_count / 3;
	for (u32 i = 0; i < triangle_count; i++)
	{
		tg_transvoxel_vertex* p_vertex0 = &p_block->p_vertices[p_block->p_indices[3 * i + 0]];
		tg_transvoxel_vertex* p_vertex1 = &p_block->p_vertices[p_block->p_indices[3 * i + 1]];
		tg_transvoxel_vertex* p_vertex2 = &p_block->p_vertices[p_block->p_indices[3 * i + 2]];

		const v3 vec0 = p_vertex0->position;
		const v3 vec1 = p_vertex1->position;
		const v3 vec2 = p_vertex2->position;

		const v3 vec01 = tgm_v3_sub(vec1, vec0);
		const v3 vec02 = tgm_v3_sub(vec2, vec0);
		
		v3 n = tgm_v3_cross(vec01, vec02);

		const v3 nvec0 = tgm_v3_normalized(vec0);
		const v3 nvec1 = tgm_v3_normalized(vec1);
		const v3 nvec2 = tgm_v3_normalized(vec2);

		const f32 a0 = 1.0f - tgm_v3_dot(tgm_v3_sub(nvec1, nvec0), tgm_v3_sub(nvec2, nvec0));
		const f32 a1 = 1.0f - tgm_v3_dot(tgm_v3_sub(nvec2, nvec1), tgm_v3_sub(nvec0, nvec1));
		const f32 a2 = 1.0f - tgm_v3_dot(tgm_v3_sub(nvec0, nvec2), tgm_v3_sub(nvec1, nvec2));

		if (!area_weighting && tgm_v3_mag(n) != 0.0f)
		{
			n = tgm_v3_normalized(n);
		}

		p_vertex0->normal = tgm_v3_add(p_vertex0->normal, tgm_v3_mulf(n, a0));
		p_vertex1->normal = tgm_v3_add(p_vertex1->normal, tgm_v3_mulf(n, a1));
		p_vertex2->normal = tgm_v3_add(p_vertex2->normal, tgm_v3_mulf(n, a1));
	}

	const u32 vertex_count = p_block->vertex_count;
	for (u32 i = 0; i < vertex_count; i++)
	{
		tg_transvoxel_vertex* p_vertex = &p_block->p_vertices[i];
		if (tgm_v3_mag(p_vertex->normal) != 0.0f)
		{
			p_vertex->normal = tgm_v3_normalized(p_vertex->normal);
		}
	}
}

void tg_transvoxel_terrain_internal_make_vertex(tg_transvoxel_block* p_block, tg_transvoxel_history* p_history, i32 cx, i32 cy, i32 cz, const v3i* p_corner_coordinates, const v3* p_positions, u16 vertex_data)
{
	// indices for the two endpoints of the edge on which the vertex lies
	const u8 low_byte = vertex_data & 0xff;        // edgeCode
	const u8 low_byte_low_nibble = low_byte & 0xf; // v0 = (edgeCode >> 4) & 0x0F;
	const u8 low_byte_high_nibble = low_byte >> 4; // v1 = edgeCode & 0x0F;

	// vertex reuse data shown in Figure 3.8
	const u8 high_byte = vertex_data >> 8;
	const u8 high_byte_low_nibble = high_byte & 0xf;
	const u8 high_byte_high_nibble = high_byte >> 4;

	v3i i0 = p_corner_coordinates[low_byte_low_nibble];
	i32 d0 = TG_TRANSVOXEL_VOXEL_MAP_AT_V3(p_block->voxel_map, i0); // long d0 = corner[v0];
	
	v3i i1 = p_corner_coordinates[low_byte_high_nibble];
	i32 d1 = TG_TRANSVOXEL_VOXEL_MAP_AT_V3(p_block->voxel_map, i1); // long d1 = corner[v1];

	const i32 t = (d1 << 8) / (d1 - d0);

	if ((t & 0x00ff) != 0)
	{
		// vertex lies in the interior of the edge

		// long u = 0x0100 - t;
		// Q = t * P0 + u * P1;

		b32 could_reuse = TG_FALSE;
		if (!(high_byte_high_nibble & 8))
		{
			// TODO: temp condition
			if (cx != 0 && cy != 0)
			{
				const i32 preceding_cell_x = cx - (high_byte_high_nibble & 1);
				const i32 preceding_cell_y = cy - ((high_byte_high_nibble & 2) >> 1);
		
				const tg_transvoxel_deck* p_deck = (high_byte_high_nibble & 4) ? p_history->p_preceding_deck : p_history->p_deck;
				const u16* p_index = p_deck->ppp_indices[TG_TRANSVOXEL_CELLS * preceding_cell_y + preceding_cell_x][high_byte_low_nibble];
				if (p_index)
				{
					p_block->p_indices[p_block->index_count] = *p_index;
					p_block->index_count++;
					could_reuse = TG_TRUE;
				}
			}
		}

		if (!could_reuse)
		{
			v3 p0 = p_positions[low_byte_low_nibble];
			v3 p1 = p_positions[low_byte_high_nibble];

			for (u8 i = 0; i < p_block->lod; i++)
			{
				const v3i midpoint = {
					(i0.x + i1.x) / 2,
					(i0.y + i1.y) / 2,
					(i0.z + i1.z) / 2
				};
				const i8 imid = TG_TRANSVOXEL_VOXEL_MAP_AT_V3(p_block->voxel_map, midpoint);

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
			const v3 q = {
				(f * p0.x + (1.0f - f) * p1.x),
				(f * p0.y + (1.0f - f) * p1.y),
				(f * p0.z + (1.0f - f) * p1.z)
			};

			p_block->p_vertices[p_block->vertex_count].position = q;
			p_block->p_vertices[p_block->vertex_count].normal = (v3){ 0.0f, 0.0f, 0.0f };
			p_block->p_indices[p_block->index_count] = p_history->next_index++;
			if (high_byte_high_nibble & 8)
			{
				p_history->p_deck->ppp_indices[TG_TRANSVOXEL_CELLS * cy + cx][high_byte_low_nibble] = &p_block->p_indices[p_block->index_count];
			}
			p_block->vertex_count++;
			p_block->index_count++;
		}
	}
	else if (t == 0)
	{
		// vertex lies at the higher-numbered endpoint

		b32 could_reuse = TG_FALSE;

		if (low_byte_high_nibble != 7)
		{
			// try to reuse corner vertex from a preceding cell

			// TODO: temp condition
			if (cx > 0 && cy > 0)
			{
				const preceding_cell_x = cx - (high_byte_high_nibble & 1);
				const preceding_cell_y = cy - ((high_byte_high_nibble & 2) >> 1);

				const tg_transvoxel_deck* p_deck = (high_byte_high_nibble & 4) ? p_history->p_preceding_deck : p_history->p_deck;
				const u16* p_index = p_deck->ppp_indices[TG_TRANSVOXEL_CELLS * preceding_cell_y + preceding_cell_x][high_byte_low_nibble];
				if (p_index)
				{
					p_block->p_indices[p_block->index_count] = *p_index;
					p_block->index_count++;
					could_reuse = TG_TRUE;
				}
			}
		}

		if (!could_reuse)
		{
			// this cell owns the vertex

			const v3 q = p_positions[low_byte_high_nibble];
			p_block->p_vertices[p_block->vertex_count].position = q;
			p_block->p_vertices[p_block->vertex_count].normal = (v3){ 0.0f, 0.0f, 0.0f };
			p_block->p_indices[p_block->index_count] = p_history->next_index++;
			if (high_byte_high_nibble & 8)
			{
				p_history->p_deck->ppp_indices[TG_TRANSVOXEL_CELLS * cy + cx][high_byte_low_nibble] = &p_block->p_indices[p_block->index_count];
			}
			p_block->vertex_count++;
			p_block->index_count++;
		}
	}
	else
	{
		// vertes lies at the lower-numbered endpoint
		// always try to reuse corner vertex from a preceding cell

		b32 could_reuse = TG_FALSE;

		// TODO: temp condition
		if (cx > 0 && cy > 0)
		{
			const i32 preceding_cell_x = cx - (high_byte_high_nibble & 1);
			const i32 preceding_cell_y = cy - ((high_byte_high_nibble & 2) >> 1);

			const tg_transvoxel_deck* p_deck = (high_byte_high_nibble & 4) ? p_history->p_preceding_deck : p_history->p_deck;
			const u16* p_index = p_deck->ppp_indices[TG_TRANSVOXEL_CELLS * preceding_cell_y + preceding_cell_x][high_byte_low_nibble];
			if (p_index)
			{
				p_block->p_indices[p_block->index_count] = *p_index;
				p_block->index_count++;
				could_reuse = TG_TRUE;
			}
		}

		if (!could_reuse)
		{
			const v3 q = p_positions[low_byte_low_nibble];
			p_block->p_vertices[p_block->vertex_count].position = q;
			p_block->p_vertices[p_block->vertex_count].normal = (v3){ 0.0f, 0.0f, 0.0f };
			p_block->p_indices[p_block->index_count] = p_history->next_index++;
			if (high_byte_high_nibble & 8)
			{
				p_history->p_deck->ppp_indices[TG_TRANSVOXEL_CELLS * cy + cx][high_byte_low_nibble] = &p_block->p_indices[p_block->index_count];
			}
			p_block->vertex_count++;
			p_block->index_count++;
		}
	}
}

tg_transvoxel_terrain_internal_fill_cell(tg_transvoxel_block* p_block, tg_transvoxel_history* p_history, i32 cx, i32 cy, i32 cz)
{
	const v3i p_corner_coordinates[8] = {
		{ cx    , cy    , cz     },
		{ cx + 1, cy    , cz     },
		{ cx    , cy + 1, cz     },
		{ cx + 1, cy + 1, cz     },
		{ cx    , cy    , cz + 1 },
		{ cx + 1, cy    , cz + 1 },
		{ cx    , cy + 1, cz + 1 },
		{ cx + 1, cy + 1, cz + 1 }
	};

	const i8 p_corner_voxels[8] = {
		TG_TRANSVOXEL_VOXEL_MAP_AT_V3(p_block->voxel_map, p_corner_coordinates[0]),
		TG_TRANSVOXEL_VOXEL_MAP_AT_V3(p_block->voxel_map, p_corner_coordinates[1]),
		TG_TRANSVOXEL_VOXEL_MAP_AT_V3(p_block->voxel_map, p_corner_coordinates[2]),
		TG_TRANSVOXEL_VOXEL_MAP_AT_V3(p_block->voxel_map, p_corner_coordinates[3]),
		TG_TRANSVOXEL_VOXEL_MAP_AT_V3(p_block->voxel_map, p_corner_coordinates[4]),
		TG_TRANSVOXEL_VOXEL_MAP_AT_V3(p_block->voxel_map, p_corner_coordinates[5]),
		TG_TRANSVOXEL_VOXEL_MAP_AT_V3(p_block->voxel_map, p_corner_coordinates[6]),
		TG_TRANSVOXEL_VOXEL_MAP_AT_V3(p_block->voxel_map, p_corner_coordinates[7])
	};

	const i32 offset_x = 16 * p_block->coordinates.x;
	const i32 offset_y = 16 * p_block->coordinates.y;
	const i32 offset_z = 16 * p_block->coordinates.z;
	const i32 cell_scale = 1 << p_block->lod;

	const v3 p_positions[8] = {
		(v3){ (f32)(offset_x +  cx      * cell_scale), (f32)(offset_y +  cy      * cell_scale), (f32)(offset_z +  cz      * cell_scale) },
		(v3){ (f32)(offset_x + (cx + 1) * cell_scale), (f32)(offset_y +  cy      * cell_scale), (f32)(offset_z +  cz      * cell_scale) },
		(v3){ (f32)(offset_x +  cx      * cell_scale), (f32)(offset_y + (cy + 1) * cell_scale), (f32)(offset_z +  cz      * cell_scale) },
		(v3){ (f32)(offset_x + (cx + 1) * cell_scale), (f32)(offset_y + (cy + 1) * cell_scale), (f32)(offset_z +  cz      * cell_scale) },
		(v3){ (f32)(offset_x +  cx      * cell_scale), (f32)(offset_y +  cy      * cell_scale), (f32)(offset_z + (cz + 1) * cell_scale) },
		(v3){ (f32)(offset_x + (cx + 1) * cell_scale), (f32)(offset_y +  cy      * cell_scale), (f32)(offset_z + (cz + 1) * cell_scale) },
		(v3){ (f32)(offset_x +  cx      * cell_scale), (f32)(offset_y + (cy + 1) * cell_scale), (f32)(offset_z + (cz + 1) * cell_scale) },
		(v3){ (f32)(offset_x + (cx + 1) * cell_scale), (f32)(offset_y + (cy + 1) * cell_scale), (f32)(offset_z + (cz + 1) * cell_scale) }
	};

	const u32 case_code =
		((p_corner_voxels[0] >> 7) & 0x01) |
		((p_corner_voxels[1] >> 6) & 0x02) |
		((p_corner_voxels[2] >> 5) & 0x04) |
		((p_corner_voxels[3] >> 4) & 0x08) |
		((p_corner_voxels[4] >> 3) & 0x10) |
		((p_corner_voxels[5] >> 2) & 0x20) |
		((p_corner_voxels[6] >> 1) & 0x40) |
		((p_corner_voxels[7] >> 0) & 0x80);

	if ((case_code ^ ((p_corner_voxels[7] >> 7) & 0xff)) != 0)
	{
		const u8 equivalence_class_index = p_regular_cell_class[case_code];
		const tg_regular_cell_data* p_cell_data = &p_regular_cell_data[equivalence_class_index];
		const u16* p_vertex_data = p_regular_vertex_data[case_code];

		const u32 triangle_count = TG_TRANSVOXEL_CELL_GET_TRIANGLE_COUNT(*p_cell_data);
		const u32 vertex_count = TG_TRANSVOXEL_CELL_GET_VERTEX_COUNT(*p_cell_data);

		for (u8 i = 0; i < 3 * triangle_count; i++)
		{
			const u8 vertex_index = p_cell_data->p_vertex_indices[i];
			const u16 vertex_data = p_vertex_data[vertex_index];
			tg_transvoxel_terrain_internal_make_vertex(p_block, p_history, cx, cy, cz, p_corner_coordinates, p_positions, vertex_data);
		}
	}
}



tg_transvoxel_terrain_h tg_transvoxel_terrain_create(tg_camera_h camera_h)
{
	tg_transvoxel_terrain_h terrain_h = TG_MEMORY_ALLOC(sizeof(*terrain_h));

	terrain_h->block.vertex_count = 0;
	terrain_h->block.p_vertices = TG_MEMORY_ALLOC(15 * TG_TRANSVOXEL_CELLS_PER_CHUNK * sizeof(*terrain_h->block.p_vertices));
	terrain_h->block.index_count = 0;
	terrain_h->block.p_indices = TG_MEMORY_ALLOC(15 * TG_TRANSVOXEL_CELLS_PER_CHUNK * sizeof(*terrain_h->block.p_indices));

	tg_transvoxel_terrain_internal_fill_voxel_map(&terrain_h->block);

	tg_transvoxel_deck* p_deck0 = TG_MEMORY_STACK_ALLOC(sizeof(*p_deck0));
	tg_transvoxel_deck* p_deck1 = TG_MEMORY_STACK_ALLOC(sizeof(*p_deck1));
	tg_memory_nullify(sizeof(*p_deck0), p_deck0);
	tg_memory_nullify(sizeof(*p_deck1), p_deck1);
	tg_transvoxel_history history = { 0 };
	history.next_index = 0;
	history.p_preceding_deck = p_deck0;
	history.p_deck = p_deck1;

	for (i32 cz = 0; cz < TG_TRANSVOXEL_CELLS - 1; cz++)
	{
		for (i32 cy = 0; cy < TG_TRANSVOXEL_CELLS - 1; cy++)
		{
			for (i32 cx = 0; cx < TG_TRANSVOXEL_CELLS - 1; cx++)
			{
				tg_transvoxel_terrain_internal_fill_cell(&terrain_h->block, &history, cx, cy, cz);
			}
		}
		tg_transvoxel_deck* p_temp_deck = history.p_preceding_deck;
		history.p_preceding_deck = history.p_deck;
		history.p_deck = p_temp_deck;
		tg_memory_nullify(sizeof(*history.p_deck->ppp_indices), history.p_deck->ppp_indices);
	}
	tg_transvoxel_terrain_internal_normals(&terrain_h->block, TG_TRUE);

	TG_MEMORY_STACK_FREE(sizeof(*p_deck1));
	TG_MEMORY_STACK_FREE(sizeof(*p_deck0));

	tg_mesh_h mesh_h = tg_mesh_create2(terrain_h->block.vertex_count, sizeof(tg_transvoxel_vertex), terrain_h->block.p_vertices, terrain_h->block.index_count, terrain_h->block.p_indices);
	tg_vertex_shader_h vertex_shader_h = tg_vertex_shader_get("shaders/deferred_flat_terrain.vert");
	tg_fragment_shader_h fragment_shader_h = tg_fragment_shader_get("shaders/deferred_flat_terrain.frag");
	tg_material_h material_h = tg_material_create_deferred(vertex_shader_h, fragment_shader_h, 0, TG_NULL);
	terrain_h->entity = tg_entity_create(mesh_h, material_h);

	return terrain_h;
}

void tg_transvoxel_terrain_destroy(tg_transvoxel_terrain_h terrain_h)
{

}

void tg_transvoxel_terrain_update(tg_transvoxel_terrain_h terrain_h)
{

}

#endif
