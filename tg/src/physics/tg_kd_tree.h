#ifndef TG_KD_TREE_H
#define TG_KD_TREE_H

#include "tg_common.h"

TG_DECLARE_HANDLE(tg_mesh);

typedef struct tg_kd_node tg_kd_node;
typedef struct tg_kd_node
{
	u32            flags; // 000 := leaf, 001 := x-axis, 010 := y-axis, 100 := z-axis
	union
	{
		struct
		{
			f32    split_position;
			u32    p_child_indices[2];
		} node;
		struct
		{
			u32    first_index_offset;
			u32    index_count;
		} leaf;
	};
} tg_kd_node;

typedef struct tg_kd_tree
{
	tg_mesh_h     h_mesh;
	u32           index_capacity;
	u32           index_count;
	u32           node_count;
	u32*          p_indices;
	tg_kd_node    p_nodes[0];
} tg_kd_tree;



tg_kd_tree*    tg_kd_tree_create(const tg_mesh_h h_mesh);
void           tg_kd_tree_destroy(tg_kd_tree* p_kd_tree);

#endif
