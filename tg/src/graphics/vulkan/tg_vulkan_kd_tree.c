#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "util/tg_qsort.h"



#define TG_AREA(b)		       (((b).max.x - (b).min.x) * ((b).max.y - (b).min.y) * ((b).max.z - (b).min.z))
#define TG_SPLIT_AXIS(dims)    ((dims).x > (dims).y ? ((dims).x > (dims).z ? 0 : 2) : ((dims).y > (dims).z ? 1 : 2))
#define TG_GOLDEN_RATIO	       1.61803401f
#define TG_SWEEP_MIN_ONLY      0
#define TG_FAST_NUM_APPROX     0



typedef struct tg_construction_triangle
{
	v3           p0;
	v3           p1;
	v3           p2;
	tg_bounds    bounds;
} tg_construction_triangle;

typedef struct tg_stack_node
{
	tg_kd_node* p_node;
	tg_bounds p_bounds;
	u32 tri_count;
	const tg_construction_triangle* p_tris;
} tg_stack_node;



static void tg__split_count(u32 split_axis, f32 split_position, u32 tri_count, const tg_construction_triangle* p_tris, u32* p_n0, u32* p_n1)
{
	*p_n0 = 0;
	*p_n1 = 0;
	for (u32 i = 0; i < tri_count; i++)
	{
		if (p_tris[i].bounds.min.p_data[split_axis] <= split_position)
		{
			(*p_n0)++;
		}
		if (p_tris[i].bounds.max.p_data[split_axis] >= split_position)
		{
			(*p_n1)++;
		}
	}
}

static void tg__split_bounds(const tg_bounds* p_bounds, u32 split_axis, f32 split_position, tg_bounds* p_b0, tg_bounds* p_b1)
{
	*p_b0 = *p_bounds;
	*p_b1 = *p_bounds;
	p_b0->max.p_data[split_axis] = split_position;
	p_b1->min.p_data[split_axis] = split_position;
}

static f32 tg__surface_area_heuristic(tg_bounds* p_bounds, u32 split_axis, u32 split_index,
#if TG_SWEEP_MIN_ONLY == 0
	b32 min,
#endif
	u32 tri_count, const tg_construction_triangle* p_tris)
{
	const tg_construction_triangle* p_split_tri = &p_tris[split_index];

	const f32 split_position = 
#if TG_SWEEP_MIN_ONLY == 0
		(min ? p_split_tri->bounds.min : p_split_tri->bounds.max).p_data[split_axis];
#else
		p_split_tri->bounds.min.p_data[split_axis];
#endif

	tg_bounds b0, b1;
	tg__split_bounds(p_bounds, split_axis, split_position, &b0, &b1);

#if TG_FAST_NUM_APPROX == 1
	u32 n0 = split_index + 1;
	u32 n1 = tri_count - split_index;
#else
	u32 n0, n1;
	tg__split_count(split_axis, split_position, tri_count, p_tris, &n0, &n1);
#endif

	const f32 areab = TG_AREA(*p_bounds);
	const f32 result = 1.0f + 80.0f * (n0 * (TG_AREA(b0) / areab) + n1 * (TG_AREA(b1) / areab));
	return result;
}

#if TG_SWEEP_MIN_ONLY == 0
i32 tg__compare_by_max(const tg_construction_triangle* p_tri0, const tg_construction_triangle* p_tri1, u32* p_axis)
{
	const f32 d = p_tri0->bounds.max.p_data[*p_axis] - p_tri1->bounds.max.p_data[*p_axis];
	const i32 result = d < 0.0f ? -1 : (d > 0.0f ? 1 : 0);
	return result;
}
#endif

i32 tg__compare_by_min(const tg_construction_triangle* p_tri0, const tg_construction_triangle* p_tri1, u32* p_axis)
{
	const f32 d = p_tri0->bounds.min.p_data[*p_axis] - p_tri1->bounds.min.p_data[*p_axis];
	const i32 result = d < 0.0f ? -1 : (d > 0.0f ? 1 : 0);
	return result;
}

static void tg__build(tg_kd_node* p_node, tg_bounds* p_bounds, u32 tri_count, tg_construction_triangle* p_tris)
{
	f32 split_cost = TG_F32_MAX;
	u32 split_index = TG_U32_MAX;
	f32 split_position = 0.0f;

	const v3 dims = tgm_v3_sub(p_bounds->max, p_bounds->min);
	u32 split_axis = TG_SPLIT_AXIS(dims);

#if TG_SWEEP_MIN_ONLY == 0
	for (b32 compare_min = 0; compare_min < 2; compare_min++)
	{
		TG_QSORT(tri_count, p_tris, compare_min ? tg__compare_by_min : tg__compare_by_max, &split_axis);
#else
		TG_QSORT(tri_count, p_tris, tg__compare_by_min, &split_axis);
#endif

		u32 a = 0;
		u32 b = tri_count - 1;
		
		u32 c = (u32)((f32)b - (f32)(b - a) / TG_GOLDEN_RATIO);
		u32 d = (u32)((f32)a + (f32)(b - a) / TG_GOLDEN_RATIO);

		f32 f = 0.0f;
		while (d - c > 1)
		{
#if TG_SWEEP_MIN_ONLY == 0
			const f32 fc = tg__surface_area_heuristic(p_bounds, split_axis, c, compare_min, tri_count, p_tris);
			const f32 fd = tg__surface_area_heuristic(p_bounds, split_axis, d, compare_min, tri_count, p_tris);
#else
			const f32 fc = tg__surface_area_heuristic(p_bounds, split_axis, c, tri_count, p_tris);
			const f32 fd = tg__surface_area_heuristic(p_bounds, split_axis, d, tri_count, p_tris);
#endif

			if (fc < fd)
			{
				b = d;
				f = fc;
			}
			else
			{
				a = c;
				f = fd;
			}

			c = (u32)((f32)b - (f32)(b - a) / TG_GOLDEN_RATIO);
			d = (u32)((f32)a + (f32)(b - a) / TG_GOLDEN_RATIO);
		}

		if (f < split_cost)
		{
			split_cost = f;
			split_index = c;
			split_position =
#if TG_SWEEP_MIN_ONLY == 0
				(compare_min ? p_tris[c].bounds.min : p_tris[c].bounds.max).p_data[split_axis];
#else
				p_tris[c].bounds.min.p_data[split_axis];
#endif
		}
#if TG_SWEEP_MIN_ONLY == 0
	}
#endif

	const f32 no_split_cost = 80.0f * 3 * tri_count;
	if (split_cost < no_split_cost)
	{
		p_node->flags = 1 << split_axis;
		p_node->node.split_position = split_position;
		p_node->node.pp_children[0] = TG_MEMORY_ALLOC(2 * sizeof(*p_node->node.pp_children[0]));
		p_node->node.pp_children[1] = &p_node->node.pp_children[0][1];

		tg_bounds b0, b1;
		tg__split_bounds(p_bounds, split_axis, split_position, &b0, &b1);

		u32 n0, n1;
		tg__split_count(split_axis, split_position, tri_count, p_tris, &n0, &n1);

		tg_construction_triangle* p_tris0 = TG_MEMORY_STACK_ALLOC(n0 * sizeof(*p_tris0));
		tg_construction_triangle* p_tris1 = TG_MEMORY_STACK_ALLOC(n1 * sizeof(*p_tris1));

		n0 = 0; n1 = 0;
		for (u32 i = 0; i < tri_count; i++)
		{
			if (p_tris[i].bounds.min.p_data[split_axis] <= split_position)
			{
				p_tris0[n0++] = p_tris[i];
			}
			if (p_tris[i].bounds.max.p_data[split_axis] >= split_position)
			{
				p_tris1[n1++] = p_tris[i];
			}
		}

		tg__build(p_node->node.pp_children[0], &b0, n0, p_tris0);
		tg__build(p_node->node.pp_children[1], &b1, n1, p_tris1);

		TG_MEMORY_STACK_FREE(n1 * sizeof(*p_tris1));
		TG_MEMORY_STACK_FREE(n0 * sizeof(*p_tris0));
	}
	else
	{
		p_node->flags = 0;
		p_node->leaf.vertex_count = 3 * tri_count;
		p_node->leaf.p_vertex_positions = TG_MEMORY_ALLOC(3LL * (u64)tri_count * sizeof(v3));
		for (u32 i = 0; i < tri_count; i++)
		{
			p_node->leaf.p_vertex_positions[3 * i    ] = p_tris[i].p0;
			p_node->leaf.p_vertex_positions[3 * i + 1] = p_tris[i].p1;
			p_node->leaf.p_vertex_positions[3 * i + 2] = p_tris[i].p2;
		}
	}
}



tg_kd_tree* tg_kd_tree_create(const tg_mesh_h h_mesh)
{
	TG_ASSERT(h_mesh && h_mesh->p_vertex_input_attribute_formats[0] == TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT);
	TG_ASSERT(h_mesh->index_count == 0); // TODO: ibo's not supported for now

	tg_kd_tree* p_tree = TG_MEMORY_ALLOC_NULLIFY(sizeof(*p_tree));

	p_tree->bounds = h_mesh->bounds;

	const u32 triangle_count = h_mesh->vertex_count / 3;
	tg_construction_triangle* p_tris = TG_MEMORY_STACK_ALLOC(triangle_count * sizeof(*p_tris));

	for (u32 i = 0; i < triangle_count; i++)
	{
		p_tris[i].p0 = h_mesh->p_vertex_positions[3 * i];
		p_tris[i].p1 = h_mesh->p_vertex_positions[3 * i + 1];
		p_tris[i].p2 = h_mesh->p_vertex_positions[3 * i + 2];
		p_tris[i].bounds.min = tgm_v3_min(tgm_v3_min(p_tris[i].p0, p_tris[i].p1), p_tris[i].p2);
		p_tris[i].bounds.max = tgm_v3_max(tgm_v3_max(p_tris[i].p0, p_tris[i].p1), p_tris[i].p2);
	}

	tg__build(&p_tree->root, &p_tree->bounds, triangle_count, p_tris);

	TG_MEMORY_STACK_FREE(triangle_count * sizeof(*p_tris));

	return p_tree;
}

void tg_kd_tree_destroy(tg_kd_tree* p_kd_tree)
{
	TG_ASSERT(p_kd_tree);

	TG_INVALID_CODEPATH();
}

#endif
