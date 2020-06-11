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
	const i32 offset_x = TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE * p_block->coordinates.x + bias;
	const i32 offset_y = TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE * p_block->coordinates.y + bias;
	const i32 offset_z = TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE * p_block->coordinates.z + bias;

	for (i32 cz = 0; cz < TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE_WITH_PADDING; cz++)
	{
		for (i32 cy = 0; cy < TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE_WITH_PADDING; cy++)
		{
			for (i32 cx = 0; cx < TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE_WITH_PADDING; cx++)
			{
				const f32 cell_coordinate_x = (f32)(offset_x + cell_scale * (cx - 1));
				const f32 cell_coordinate_y = (f32)(offset_y + cell_scale * (cy - 1));
				const f32 cell_coordinate_z = (f32)(offset_z + cell_scale * (cz - 1));

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



tg_transvoxel_terrain_internal_get_border_mask(v3i position, i32 block_size)
{
	u8 mask = 0;

	for (u8 i = 0; i < 3; i++) {
		if (position.p_data[i] == 0) {
			mask |= (1 << (i * 2));
		}
		if (position.p_data[i] == block_size) {
			mask |= (1 << (i * 2 + 1));
		}
	}

	return mask;
}

v3 tg_transvoxel_terrain_internal_normalized_not_null(v3 v)
{
	const f32 magsqr = tgm_v3_magsqr(v);
	v3 result = { 0.0f, 1.0f, 0.0f };
	if (magsqr != 0.0f)
	{
		const f32 mag = tgm_f32_sqrt(magsqr);
		result = (v3){ v.x / mag, v.y / mag, v.z / mag };
	}
	return result;
}

#define TRANSITION_CELL_SCALE 0.5f
v3 tg_transvoxel_internal_get_border_offset(v3 position, u8 lod, i32 block_size)
{
	v3 delta = { 0 };

	const f32 p2k = (f32)(1 << lod);
	const f32 p2mk = 1.0f / p2k;

	const f32 wk = TRANSITION_CELL_SCALE * p2k;

	for (u8 i = 0; i < 3; ++i)
	{
		const f32 p = position.p_data[i];
		const f32 s = (f32)block_size;

		if (p < p2k)
		{
			delta.p_data[i] = (1.0f - p2mk * p) * wk;
		}
		else if (p > (p2k * (s - 1)))
		{
			delta.p_data[i] = ((p2k * s) - 1.0f - p) * wk;
		}
	}

	return delta;
}
#undef TRANSITION_CELL_SCALE

v3 tg_transvoxel_internal_project_border_offset(v3 delta, v3 normal)
{
	const v3 result = {
		(1.0f - normal.x * normal.x) * delta.x         - normal.y * normal.x  * delta.y         - normal.z * normal.x  * delta.z,
		      - normal.x * normal.y  * delta.x + (1.0f - normal.y * normal.y) * delta.y         - normal.z * normal.y  * delta.z,
		      - normal.x * normal.z  * delta.x         - normal.y * normal.z  * delta.y + (1.0f - normal.z * normal.z) * delta.z
	};
	return result;
}

v3 tg_transvoxel_terrain_internal_get_secondary_position(v3 primary, v3 normal, u8 lod, i32 block_size)
{
	v3 delta = tg_transvoxel_internal_get_border_offset(primary, lod, block_size);
	delta = tg_transvoxel_internal_project_border_offset(delta, normal);
	const v3 result = tgm_v3_add(primary, delta);
	return result;
}

u16 tg_transvoxel_terrain_internal_emit_vertex(tg_transvoxel_block* p_block, v3 primary, v3 normal, u16 border_mask, v3 secondary)
{
	const u16 result = p_block->vertex_count;

	p_block->p_vertices[p_block->vertex_count].position = primary;
	p_block->p_vertices[p_block->vertex_count].normal = normal;
	p_block->p_vertices[p_block->vertex_count].extra = tgm_v3_to_v4(secondary, (f32)border_mask);
	p_block->vertex_count++;

	return result;
}

#define DIRECTION_TO_PRECEDING_CELL(direction)    ((v3i){ -((direction) & 1), -(((direction) >> 1) & 1), -(((direction) >> 2) & 1) })
#define FIXED_FACTOR                              (1.0f / 256.0f)
tg_transvoxel_terrain_internal_build(tg_transvoxel_block* p_block)
{
	tg_transvoxel_reuse_cells* p_reuse_cells = TG_MEMORY_STACK_ALLOC(sizeof(*p_reuse_cells));
	tg_memory_set_all_bits(sizeof(*p_reuse_cells), p_reuse_cells);

	const i32 cells_per_block_side_scaled = TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE << p_block->lod;
	const i32 min_position = TG_TRANSVOXEL_MIN_PADDING;
	const i32 max_position = TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING - TG_TRANSVOXEL_MAX_PADDING;

	v3i p_corner_positions[8] = { 0 };
	i8 p_cell_samples[8] = { 0 };
	v3 p_corner_gradients[8] = { 0 };

	v3i position = { 0 };
	for (position.z = min_position; position.z < max_position; position.z++)
	{
		for (position.y = min_position; position.y < max_position; position.y++)
		{
			for (position.x = min_position; position.x < max_position; position.x++)
			{
				p_corner_positions[0] = (v3i){ position.x    , position.y    , position.z     };
				p_corner_positions[1] = (v3i){ position.x + 1, position.y    , position.z     };
				p_corner_positions[2] = (v3i){ position.x    , position.y + 1, position.z     };
				p_corner_positions[3] = (v3i){ position.x + 1, position.y + 1, position.z     };
				p_corner_positions[4] = (v3i){ position.x    , position.y    , position.z + 1 };
				p_corner_positions[5] = (v3i){ position.x + 1, position.y    , position.z + 1 };
				p_corner_positions[6] = (v3i){ position.x    , position.y + 1, position.z + 1 };
				p_corner_positions[7] = (v3i){ position.x + 1, position.y + 1, position.z + 1 };

				for (u8 i = 0; i < 8; i++)
				{
					p_cell_samples[i] = TG_TRANSVOXEL_VOXEL_MAP_AT_V3(p_block->voxel_map, p_corner_positions[i]);
				}

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

				tg_transvoxel_reuse_cell* p_reuse_cell = &TG_TRANSVOXEL_REUSE_CELL_AT_V3(*p_reuse_cells, position);
				p_reuse_cell->p_indices[0] = TG_U16_MAX;

				if (case_code == 0 || case_code == 255)
				{
					continue;
				}

				for (u32 i = 0; i < 8; i++) {

					const v3i p = p_corner_positions[i];

					const f32 nx = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_block->voxel_map, p.x - 1, p.y    , p.z    ));
					const f32 ny = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_block->voxel_map, p.x    , p.y - 1, p.z    ));
					const f32 nz = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_block->voxel_map, p.x    , p.y    , p.z - 1));
					const f32 px = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_block->voxel_map, p.x + 1, p.y    , p.z    ));
					const f32 py = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_block->voxel_map, p.x    , p.y + 1, p.z    ));
					const f32 pz = (f32)(255 - TG_TRANSVOXEL_VOXEL_MAP_AT(p_block->voxel_map, p.x    , p.y    , p.z + 1));

					p_corner_gradients[i].x = nx - px;
					p_corner_gradients[i].y = ny - py;
					p_corner_gradients[i].z = nz - pz;

					p_corner_positions[i].x = (p_corner_positions[i].x - min_position) << p_block->lod;
					p_corner_positions[i].y = (p_corner_positions[i].y - min_position) << p_block->lod;
					p_corner_positions[i].z = (p_corner_positions[i].z - min_position) << p_block->lod;
				}

				const u8 direction_validity_mask =
					((position.x > min_position ? 1 : 0) << 0) |
					((position.y > min_position ? 1 : 0) << 1) |
					((position.z > min_position ? 1 : 0) << 2);

				const u8 regular_cell_class_index = p_regular_cell_class[case_code];
				const tg_regular_cell_data* p_cell_data = &p_regular_cell_data[regular_cell_class_index];
				const u8 triangle_count = TG_TRANSVOXEL_CELL_GET_TRIANGLE_COUNT(*p_cell_data);
				const u8 vertex_count = TG_TRANSVOXEL_CELL_GET_VERTEX_COUNT(*p_cell_data);

				u16 p_cell_vertex_indices[12] = { TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX, TG_U16_MAX };

				const u8 cell_border_mask = tg_transvoxel_terrain_internal_get_border_mask(tgm_v3i_subi(position, min_position), TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE);

				for (u8 i = 0; i < vertex_count; i++)
				{
					const u16 regular_vertex_data = p_regular_vertex_data[case_code][i];
					const u8 edge_code_low = regular_vertex_data & 0xff;
					const u8 edge_code_high = (regular_vertex_data >> 8) & 0xff;

					const u8 v0 = (edge_code_low >> 4) & 0xf;
					const u8 v1 = (edge_code_low >> 0) & 0xf;
					TG_ASSERT(v0 < v1);

					const i32 sample0 = p_cell_samples[v0];
					const i32 sample1 = p_cell_samples[v1];
					TG_ASSERT(sample0 != sample1);

					const i32 t = (sample1 << 8) / (sample1 - sample0);

					const f32 t0 = (f32)t / 256.0f;
					const f32 t1 = (f32)(0x100 - t) / 256.0f;
					const i32 ti0 = t;
					const i32 ti1 = 0x100 - t;

					const v3i p0 = p_corner_positions[v0];
					const v3i p1 = p_corner_positions[v1];

					if (t & 0xff)
					{
						const u8 reuse_direction = (edge_code_high >> 4) & 0xf;
						const u8 reuse_vertex_index = edge_code_high & 0xf;

						const b32 present = (reuse_direction & direction_validity_mask) == reuse_direction;

						if (present)
						{
							const v3i cache_position = tgm_v3i_add(position, DIRECTION_TO_PRECEDING_CELL(reuse_direction));
							const tg_transvoxel_reuse_cell* p_preceding_cell = &TG_TRANSVOXEL_REUSE_CELL_AT_V3(*p_reuse_cells, cache_position);
							p_cell_vertex_indices[i] = p_preceding_cell->p_indices[reuse_vertex_index];
						}

						if (!present || p_cell_vertex_indices[i] == TG_U16_MAX)
						{
							// TODO: surface shifting interpolation

							const v3i primary = tgm_v3i_add(tgm_v3i_muli(p0, ti0), tgm_v3i_muli(p1, ti1));
							const v3 primaryf = tgm_v3_mulf(tgm_v3i_to_v3(primary), FIXED_FACTOR);
							const v3 normal = tg_transvoxel_terrain_internal_normalized_not_null(tgm_v3_add(tgm_v3_mulf(p_corner_gradients[v0], t0), tgm_v3_mulf(p_corner_gradients[v1], t1)));

							v3 secondary = { 0 };
							u16 border_mask = cell_border_mask;

							if (cell_border_mask > 0)
							{
								secondary = tg_transvoxel_terrain_internal_get_secondary_position(primaryf, normal, 0, cells_per_block_side_scaled);
								border_mask |= (tg_transvoxel_terrain_internal_get_border_mask(p0, cells_per_block_side_scaled) &
									tg_transvoxel_terrain_internal_get_border_mask(p1, cells_per_block_side_scaled)) << 6;
							}

							p_cell_vertex_indices[i] = tg_transvoxel_terrain_internal_emit_vertex(p_block, primaryf, normal, border_mask, secondary);

							if (reuse_direction & 8)
							{
								p_reuse_cell->p_indices[reuse_vertex_index] = p_cell_vertex_indices[i];
							}
						}
					}
					else if (t == 0 && v1 == 7)
					{
						const v3i primary = p1;
						const v3 primaryf = tgm_v3i_to_v3(primary);
						const v3 normal = tg_transvoxel_terrain_internal_normalized_not_null(p_corner_gradients[v1]);

						v3 secondary = { 0 };
						u16 border_mask = cell_border_mask;

						if (cell_border_mask > 0)
						{
							secondary = tg_transvoxel_terrain_internal_get_secondary_position(primaryf, normal, 0, cells_per_block_side_scaled);
							border_mask |= tg_transvoxel_terrain_internal_get_border_mask(p1, cells_per_block_side_scaled) << 6;
						}

						p_cell_vertex_indices[i] = tg_transvoxel_terrain_internal_emit_vertex(p_block, primaryf, normal, border_mask, secondary);

						p_reuse_cell->p_indices[0] = p_cell_vertex_indices[i];
					}
					else
					{
						const u8 reuse_direction = (t == 0 ? v1 ^ 7 : v0 ^ 7);
						const b32 present = (reuse_direction & direction_validity_mask) == reuse_direction;

						if (present)
						{
							const v3i cache_position = tgm_v3i_add(position, DIRECTION_TO_PRECEDING_CELL(reuse_direction));
							const tg_transvoxel_reuse_cell* p_preceding_cell = &TG_TRANSVOXEL_REUSE_CELL_AT_V3(*p_reuse_cells, cache_position);
							p_cell_vertex_indices[i] = p_preceding_cell->p_indices[0];
						}

						if (!present || p_cell_vertex_indices[i] == TG_U16_MAX)
						{
							const v3i primary = t == 0 ? p1 : p0;
							const v3 primaryf = tgm_v3i_to_v3(primary);
							const v3 normal = tg_transvoxel_terrain_internal_normalized_not_null(p_corner_gradients[t == 0 ? v1 : v0]);

							v3 secondary = { 0 };
							u16 border_mask = cell_border_mask;

							if (cell_border_mask > 0)
							{
								secondary = tg_transvoxel_terrain_internal_get_secondary_position(primaryf, normal, 0, cells_per_block_side_scaled);
								border_mask |= tg_transvoxel_terrain_internal_get_border_mask(primary, cells_per_block_side_scaled) >> 6;
							}

							p_cell_vertex_indices[i] = tg_transvoxel_terrain_internal_emit_vertex(p_block, primaryf, normal, border_mask, secondary);
						}
					}
				}

				for (u8 t = 0; t < triangle_count; t++)
				{
					for (u8 i = 0; i < 3; i++)
					{
						const u16 index = p_cell_vertex_indices[p_cell_data->p_vertex_indices[3 * t + i]];
						p_block->p_indices[p_block->index_count++] = index;
					}
				}
			}
		}
	}
}
#undef DIRECTION_TO_PRECEDING_CELL
#undef FIXED_FACTOR



tg_transvoxel_terrain_h tg_transvoxel_terrain_create(tg_camera_h camera_h)
{
	tg_transvoxel_terrain_h terrain_h = TG_MEMORY_ALLOC(sizeof(*terrain_h));

	terrain_h->block.vertex_count = 0;
	terrain_h->block.p_vertices = TG_MEMORY_ALLOC(15 * TG_TRANSVOXEL_CELLS_PER_BLOCK * sizeof(*terrain_h->block.p_vertices));
	terrain_h->block.index_count = 0;
	terrain_h->block.p_indices = TG_MEMORY_ALLOC(15 * TG_TRANSVOXEL_CELLS_PER_BLOCK * sizeof(*terrain_h->block.p_indices));

	tg_transvoxel_terrain_internal_fill_voxel_map(&terrain_h->block);
	tg_transvoxel_terrain_internal_build(&terrain_h->block);

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
