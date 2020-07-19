#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "util/tg_qsort.h"



#define TG_SURFACE_AREA(b, result)                                                                   \
	const f32 w##result = (b).max.x - (b).min.x;                                                     \
	const f32 h##result = (b).max.y - (b).min.y;                                                     \
	const f32 d##result = (b).max.z - (b).min.z;                                                     \
	const f32 result = 2.0f * (w##result * w##result + h##result * h##result+ d##result * d##result)

#define TG_TIME_TRAVERSAL                     1.0f
#define TG_TIME_INTERSECT                     2.0f
#define TG_SPLIT_AXIS(dims)                   ( (dims).x > (dims).y ? ((dims).x > (dims).z ? 0 : 2) : ((dims).y > (dims).z ? 1 : 2) )
#define TG_STACK_PUSH(p_n, b, tc, p_t)        ( (void) (++p_top, (p_top->p_node = (p_n)), (p_top->bounds = (b)), (p_top->tri_count = (tc)), (p_top->p_tris = (p_t))) )
#define TG_STACK_POP_INTO(p_n, b, tc, p_t)    ( (void) (((p_n) = p_top->p_node), ((b) = p_top->bounds), ((tc) = p_top->tri_count), ((p_t) = p_top->p_tris), --p_top) )
#define TG_STACK_NOT_EMPTY                    ( p_stack < p_top )



typedef struct tg_construction_triangle
{
	v3           p0;
	v3           p1;
	v3           p2;
	tg_bounds    bounds;
} tg_construction_triangle;

typedef struct tg_stack_node
{
	tg_kd_node*                  p_node;
	tg_bounds                    bounds;
	u32                          tri_count;
	tg_construction_triangle*    p_tris;
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

static f32 tg__surface_area_heuristic(const tg_bounds* p_bounds, u32 split_axis, u32 split_index, b32 min, u32 tri_count, const tg_construction_triangle* p_tris)
{
	const tg_construction_triangle* p_split_tri = &p_tris[split_index];

	const f32 split_position = (min ? p_split_tri->bounds.min : p_split_tri->bounds.max).p_data[split_axis];

	tg_bounds b0, b1;
	tg__split_bounds(p_bounds, split_axis, split_position, &b0, &b1);

	u32 n0, n1;
	tg__split_count(split_axis, split_position, tri_count, p_tris, &n0, &n1);

	TG_SURFACE_AREA(*p_bounds, a);
	TG_SURFACE_AREA(b0, a0);
	TG_SURFACE_AREA(b1, a1);

	const f32 result = TG_TIME_TRAVERSAL + TG_TIME_INTERSECT * ((a0 / a) * n0 + (a1 / a) * n1);
	return result;
}

i32 tg__compare_by_max(const tg_construction_triangle* p_tri0, const tg_construction_triangle* p_tri1, u32* p_axis)
{
	const f32 d = p_tri0->bounds.max.p_data[*p_axis] - p_tri1->bounds.max.p_data[*p_axis];
	const i32 result = d < 0.0f ? -1 : (d > 0.0f ? 1 : 0);
	return result;
}

i32 tg__compare_by_min(const tg_construction_triangle* p_tri0, const tg_construction_triangle* p_tri1, u32* p_axis)
{
	const f32 d = p_tri0->bounds.min.p_data[*p_axis] - p_tri1->bounds.min.p_data[*p_axis];
	const i32 result = d < 0.0f ? -1 : (d > 0.0f ? 1 : 0);
	return result;
}


static int node_count = 0;
tg_kd_tree* tg_kd_tree_create(const tg_mesh_h h_mesh)
{
	TG_ASSERT(h_mesh && h_mesh->p_vertex_input_attribute_formats[0] == TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT);
	TG_ASSERT(h_mesh->index_count == 0); // TODO: ibo's not supported for now

	tg_kd_tree* p_tree = TG_MEMORY_ALLOC_NULLIFY(sizeof(*p_tree));

	p_tree->bounds = h_mesh->bounds;

	const u32 initial_tri_count = h_mesh->vertex_count / 3;

	// this is only true as long as each child contains at least one triangle less than it's parent
	const u32 max_node_count = 2 * initial_tri_count - 1;
	const u64 initial_tris_size = (u64)initial_tri_count * sizeof(tg_construction_triangle);
	const u64 stack_size = ((u64)initial_tri_count + 1LL) * sizeof(tg_stack_node);

	tg_stack_node* p_stack = TG_MEMORY_STACK_ALLOC(stack_size);
	tg_construction_triangle* p_initial_tris = TG_MEMORY_ALLOC(initial_tris_size);

	for (u32 i = 0; i < initial_tri_count; i++)
	{
		p_initial_tris[i].p0 = h_mesh->p_vertex_positions[3 * i];
		p_initial_tris[i].p1 = h_mesh->p_vertex_positions[3 * i + 1];
		p_initial_tris[i].p2 = h_mesh->p_vertex_positions[3 * i + 2];
		p_initial_tris[i].bounds.min = tgm_v3_min(tgm_v3_min(p_initial_tris[i].p0, p_initial_tris[i].p1), p_initial_tris[i].p2);
		p_initial_tris[i].bounds.max = tgm_v3_max(tgm_v3_max(p_initial_tris[i].p0, p_initial_tris[i].p1), p_initial_tris[i].p2);
	}

	tg_stack_node* p_top = p_stack;
	TG_STACK_PUSH(&p_tree->root, p_tree->bounds, initial_tri_count, p_initial_tris);

	tg_kd_node* p_node = TG_NULL;
	tg_bounds bounds = { 0 };
	u32 tri_count = 0;
	tg_construction_triangle* p_tris = TG_NULL;
	while (TG_STACK_NOT_EMPTY)
	{
		TG_STACK_POP_INTO(p_node, bounds, tri_count, p_tris);
		node_count++;
		f32 split_cost = TG_F32_MAX;
		u32 split_index = TG_U32_MAX;
		f32 split_position = 0.0f;

		const v3 dims = tgm_v3_sub(bounds.max, bounds.min);
		u32 split_axis = TG_SPLIT_AXIS(dims);

		for (b32 compare_min = 0; compare_min < 2; compare_min++)
		{
			TG_QSORT(tri_count, p_tris, compare_min ? tg__compare_by_min : tg__compare_by_max, &split_axis);

			u32 a = 0;
			u32 b = tri_count - 1;
		
			u32 c = (u32)((f32)b - (f32)(b - a) / TG_GOLDEN_RATIO);
			u32 d = (u32)((f32)a + (f32)(b - a) / TG_GOLDEN_RATIO);

			f32 f = 0.0f;
			while (d - c > 1)
			{
				const f32 fc = tg__surface_area_heuristic(&bounds, split_axis, c, compare_min, tri_count, p_tris);
				const f32 fd = tg__surface_area_heuristic(&bounds, split_axis, d, compare_min, tri_count, p_tris);

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
				split_position = (compare_min ? p_tris[c].bounds.min : p_tris[c].bounds.max).p_data[split_axis];
			}
		}

		u32 n0, n1;
		tg__split_count(split_axis, split_position, tri_count, p_tris, &n0, &n1);

		const b32 split = n0 != tri_count && n1 != tri_count && n0 + n1 < 2 * tri_count;
		if (split)
		{
			p_node->flags = 1 << split_axis;
			p_node->node.split_position = split_position;
			p_node->node.pp_children[0] = TG_MEMORY_ALLOC(2 * sizeof(*p_node->node.pp_children[0]));
			p_node->node.pp_children[1] = &p_node->node.pp_children[0][1];

			tg_bounds b0, b1;
			tg__split_bounds(&bounds, split_axis, split_position, &b0, &b1);

			tg_construction_triangle* p_tris0 = TG_MEMORY_ALLOC(n0 * sizeof(tg_construction_triangle));
			tg_construction_triangle* p_tris1 = TG_MEMORY_ALLOC(n1 * sizeof(tg_construction_triangle));

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

			TG_STACK_PUSH(p_node->node.pp_children[0], b0, n0, p_tris0);
			TG_STACK_PUSH(p_node->node.pp_children[1], b1, n1, p_tris1);
		}
		
		if (!split)
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

		TG_MEMORY_FREE(p_tris);
	}

	TG_MEMORY_STACK_FREE(stack_size);

	return p_tree;
}

void tg_kd_tree_destroy(tg_kd_tree* p_kd_tree)
{
	TG_ASSERT(p_kd_tree);

	TG_INVALID_CODEPATH();
}

#endif
