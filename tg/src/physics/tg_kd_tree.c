#include "physics/tg_kd_tree.h"

#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"
#include "util/tg_qsort.h"



#define TG_SURFACE_AREA(b, result)                                                                   \
	const f32 w##result = (b).max.x - (b).min.x;                                                     \
	const f32 h##result = (b).max.y - (b).min.y;                                                     \
	const f32 d##result = (b).max.z - (b).min.z;                                                     \
	const f32 result = 2.0f * (w##result * w##result + h##result * h##result+ d##result * d##result)

#define TG_TIME_TRAVERSAL      1.0f
#define TG_TIME_INTERSECT      2.0f
#define TG_SPLIT_AXIS(dims)    ((dims).x > (dims).y ? ((dims).x > (dims).z ? 0 : 2) : ((dims).y > (dims).z ? 1 : 2))



typedef struct tg_construction_triangle
{
	u32          i0, i1, i2;
	tg_bounds    bounds;
} tg_construction_triangle; // TODO: put this in as a struct [0];

typedef struct tg_stack_node
{
	tg_bounds                    bounds;
	u32                          node_index;
	u32                          tri_count;
	tg_construction_triangle*    p_tris;
} tg_stack_node;

typedef struct tg_work_thread_info
{
	tg_kd_tree*       p_tree;
	tg_mutex_h        h_stack_mutex;
	tg_mutex_h        h_indices_mutex;
	i32               stack_node_count;
	tg_stack_node*    p_stack_nodes;
} tg_work_thread_info;

typedef struct tg_qsort_user_data
{
	tg_kd_tree*    p_tree;
	b32            compare_by_min;
	u32            split_axis;
} tg_qsort_user_data;



void tg__work_fn(volatile void* p_user_data);
static void tg__stack_push(tg_work_thread_info* p_work_thread_info, u32 node_index, tg_bounds bounds, u32 tri_count, tg_construction_triangle* p_tris)
{
	TG_MUTEX_LOCK(p_work_thread_info->h_stack_mutex);

	tg_stack_node* p_stack_node = &p_work_thread_info->p_stack_nodes[p_work_thread_info->stack_node_count++];
	p_stack_node->node_index = node_index;
	p_stack_node->bounds = bounds;
	p_stack_node->tri_count = tri_count;
	p_stack_node->p_tris = p_tris;

	TG_MUTEX_UNLOCK(p_work_thread_info->h_stack_mutex);
	tg_platform_work_queue_add_entry(tg__work_fn, p_work_thread_info);
}

static void tg__split_count(u32 split_axis, f32 split_position, u32 tri_count, const tg_construction_triangle* p_tris, u32* p_n0, u32* p_n1)
{
	*p_n0 = 0, *p_n1 = 0;
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

i32 tg__compare(const tg_construction_triangle* p_tri0, const tg_construction_triangle* p_tri1, tg_qsort_user_data* p_user_data)
{
#if 1 // TODO: further comparisons
	f32 f0, f1;
	if (p_user_data->compare_by_min)
	{
		f0 = p_tri0->bounds.min.p_data[p_user_data->split_axis];
		f1 = p_tri1->bounds.min.p_data[p_user_data->split_axis];
	}
	else
	{
		f0 = p_tri0->bounds.max.p_data[p_user_data->split_axis];
		f1 = p_tri1->bounds.max.p_data[p_user_data->split_axis];
	}
	const f32 d = f0 - f1;
	const i32 result = d < 0.0f ? -1 : (d > 0.0f ? 1 : 0);
	return result;
#else
	const f32 f0min = p_tri0->bounds.min.p_data[p_user_data->split_axis];
	const f32 f1min = p_tri1->bounds.min.p_data[p_user_data->split_axis];
	const f32 f0max = p_tri0->bounds.max.p_data[p_user_data->split_axis];
	const f32 f1max = p_tri1->bounds.max.p_data[p_user_data->split_axis];
	
	const f32 cbm = (f32)p_user_data->compare_by_min;
	const f32 f0 = cbm * f0min + (1.0f - cbm) * f0max;
	const f32 f1 = cbm * f1min + (1.0f - cbm) * f1max;
	
	const f32 d = f0 - f1;
	const i32 result = *(const i32*)&d;
	return result;
#endif
}



void tg__work_fn(volatile void* p_user_data)
{
	tg_work_thread_info* p_work_thread_info = (tg_work_thread_info*)p_user_data;

	u32 node_index = TG_U32_MAX;
	tg_bounds bounds = { 0 };
	u32 tri_count = 0;
	tg_construction_triangle* p_tris = TG_NULL;

	TG_MUTEX_LOCK(p_work_thread_info->h_stack_mutex);
	const b32 work_exists = p_work_thread_info->stack_node_count > 0;
	if (work_exists)
	{
		tg_stack_node* p_stack_node = &p_work_thread_info->p_stack_nodes[--p_work_thread_info->stack_node_count];
		node_index = p_stack_node->node_index;
		bounds = p_stack_node->bounds;
		tri_count = p_stack_node->tri_count;
		p_tris = p_stack_node->p_tris;
		*p_stack_node = (tg_stack_node){ 0 };
	}
	TG_MUTEX_UNLOCK(p_work_thread_info->h_stack_mutex);

	if (work_exists)
	{
		volatile tg_kd_node* p_node = &p_work_thread_info->p_tree->p_nodes[node_index];

		f32 split_cost = TG_F32_MAX;
		u32 split_index = TG_U32_MAX;
		f32 split_position = 0.0f;

		const v3 dims = tgm_v3_sub(bounds.max, bounds.min);
		u32 split_axis = TG_SPLIT_AXIS(dims);

		for (b32 compare_by_min = 0; compare_by_min < 2; compare_by_min++)
		{
			tg_qsort_user_data qsort_user_data = { 0 };
			qsort_user_data.p_tree = p_work_thread_info->p_tree;
			qsort_user_data.compare_by_min = compare_by_min;
			qsort_user_data.split_axis = split_axis;

			TG_QSORT(tri_count, p_tris, tg__compare, &qsort_user_data);

			u32 a = 0;
			u32 b = tri_count - 1;

			u32 c = (u32)((f32)b - (f32)(b - a) / TG_GOLDEN_RATIO);
			u32 d = (u32)((f32)a + (f32)(b - a) / TG_GOLDEN_RATIO);

			f32 f = 0.0f;
			while (d - c > 1)
			{
				const f32 fc = tg__surface_area_heuristic(&bounds, split_axis, c, compare_by_min, tri_count, p_tris);
				const f32 fd = tg__surface_area_heuristic(&bounds, split_axis, d, compare_by_min, tri_count, p_tris);

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
				split_position = (compare_by_min ? p_tris[c].bounds.min : p_tris[c].bounds.max).p_data[split_axis];
			}
		}

		u32 n0, n1;
		tg__split_count(split_axis, split_position, tri_count, p_tris, &n0, &n1);

		const b32 split_node = n0 != tri_count && n1 != tri_count && n0 + n1 < 2 * tri_count;
		if (split_node)
		{
			p_node->flags = 1 << split_axis;
			p_node->node.split_position = split_position;
			p_node->node.p_child_indices[0] = TG_INTERLOCKED_INCREMENT_I32(&p_work_thread_info->p_tree->node_count) - 1;
			p_node->node.p_child_indices[1] = TG_INTERLOCKED_INCREMENT_I32(&p_work_thread_info->p_tree->node_count) - 1;

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

			tg__stack_push(p_work_thread_info, p_node->node.p_child_indices[0], b0, n0, p_tris0);
			tg__stack_push(p_work_thread_info, p_node->node.p_child_indices[1], b1, n1, p_tris1);
		}
		else
		{
			p_node->flags = 0;
			p_node->leaf.index_count = 3 * tri_count;
			
			TG_MUTEX_LOCK(p_work_thread_info->h_indices_mutex);
			p_node->leaf.first_index_offset = p_work_thread_info->p_tree->index_count;
			p_work_thread_info->p_tree->index_count += p_node->leaf.index_count;
			const u64 required_index_capacity = (u64)p_work_thread_info->p_tree->index_count * sizeof(*p_work_thread_info->p_tree->p_indices);
			if (p_work_thread_info->p_tree->index_capacity < required_index_capacity)
			{
				p_work_thread_info->p_tree->index_capacity *= 2;
				p_work_thread_info->p_tree->p_indices = TG_MEMORY_REALLOC(p_work_thread_info->p_tree->index_capacity, p_work_thread_info->p_tree->p_indices);
			}
			TG_MUTEX_UNLOCK(p_work_thread_info->h_indices_mutex);

			u32* p_it = &p_work_thread_info->p_tree->p_indices[p_node->leaf.first_index_offset];
			const u32* p_end = &p_work_thread_info->p_tree->p_indices[p_node->leaf.first_index_offset + p_node->leaf.index_count];
			const tg_construction_triangle* p_it_tri = p_tris;
			do
			{
				*p_it++ = p_it_tri->i0;
				*p_it++ = p_it_tri->i1;
				*p_it++ = p_it_tri++->i2;
			} while (p_it < p_end);
		}

		TG_MEMORY_FREE(p_tris);
	}
}

tg_kd_tree* tg_kd_tree_create(tg_mesh_h h_mesh)
{
	TG_ASSERT(h_mesh);
	TG_ASSERT(tg_mesh_get_index_count(h_mesh) == 0); // TODO: ibo's not supported for now

	const u32 position_count = tg_mesh_get_vertex_count(h_mesh);
	TG_ASSERT(position_count);
	v3* p_positions = TG_MEMORY_STACK_ALLOC(position_count * sizeof(*p_positions));
	tg_mesh_copy_positions(h_mesh, 0, position_count, p_positions);
	const u32 initial_tri_count = position_count / 3;

	const u32 max_node_count = 2 * initial_tri_count - 1; // this is only true as long as each child contains at least one triangle less than it's parent
	tg_kd_tree* p_tree = TG_MEMORY_ALLOC_NULLIFY(sizeof(*p_tree) + max_node_count * sizeof(*p_tree->p_nodes));
	p_tree->h_mesh = h_mesh;
	p_tree->index_capacity = (u64)position_count * sizeof(*p_tree->p_indices);
	p_tree->index_count = 0;
	p_tree->node_count = 1;
	p_tree->p_indices = TG_MEMORY_ALLOC(p_tree->index_capacity);

	tg_work_thread_info work_thread_info = { 0 };
	work_thread_info.p_tree = p_tree;
	const u64 nodes_size = ((u64)initial_tri_count + 1LL) * sizeof(tg_stack_node);
	work_thread_info.h_stack_mutex = TG_MUTEX_CREATE();
	work_thread_info.h_indices_mutex = TG_MUTEX_CREATE();
	work_thread_info.p_stack_nodes = TG_MEMORY_STACK_ALLOC(nodes_size);
	tg_memory_nullify(nodes_size, work_thread_info.p_stack_nodes);
	work_thread_info.stack_node_count = 0;

	const u64 initial_tris_size = (u64)initial_tri_count * sizeof(tg_construction_triangle);
	tg_construction_triangle* p_initial_tris = TG_MEMORY_ALLOC(initial_tris_size);
	for (u32 i = 0; i < initial_tri_count; i++)
	{
		p_initial_tris[i].i0 = 3 * i;
		p_initial_tris[i].i1 = 3 * i + 1;
		p_initial_tris[i].i2 = 3 * i + 2;
		const v3 p0 = p_positions[3 * i];
		const v3 p1 = p_positions[3 * i + 1];
		const v3 p2 = p_positions[3 * i + 2];
		p_initial_tris[i].bounds.min = tgm_v3_min(tgm_v3_min(p0, p1), p2);
		p_initial_tris[i].bounds.max = tgm_v3_max(tgm_v3_max(p0, p1), p2);
	}

	tg__stack_push(&work_thread_info, 0, tg_mesh_get_bounds(p_tree->h_mesh), initial_tri_count, p_initial_tris);
	tg_platform_work_queue_wait_for_completion();

	TG_MEMORY_STACK_FREE(nodes_size);
	TG_MEMORY_STACK_FREE(position_count * sizeof(*p_positions));

	return p_tree;
}

void tg_kd_tree_destroy(tg_kd_tree* p_kd_tree)
{
	TG_ASSERT(p_kd_tree);

	TG_INVALID_CODEPATH();
}
