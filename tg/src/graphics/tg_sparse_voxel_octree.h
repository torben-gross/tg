#ifndef TG_SPARSE_VOXEL_OCTREE_H
#define TG_SPARSE_VOXEL_OCTREE_H

#include "graphics/tg_graphics_core.h"

typedef struct tg_svo_inner_node
{
    u16    child_pointer; // Offset of the first child node, relative from this node, measured in number of nodes. Children are stored consecutively in memory
    u8     valid_mask;    // Whether the given child exists
    u8     leaf_mask;     // Whether the given child is a leaf
} tg_svo_inner_node;

typedef struct tg_svo_leaf_node_data
{
    u16    instance_count;
    u16    p_instance_ids[8];
    u32    block_pointer;     // Pointer into voxel array. Size of block is implicit by level in hierarchy
} tg_svo_leaf_node_data;

typedef struct tg_svo_leaf_node
{
    u32    data_pointer;
} tg_svo_leaf_node;

typedef union tg_svo_node
{
    tg_svo_inner_node    inner;
    tg_svo_leaf_node     leaf;
} tg_svo_node;

typedef struct tg_svo_header
{
    v3i                       min;
    v3i                       max;
    
    u32                       voxels_buffer_capacity;
    u32                       voxels_buffer_size;
    u32                       leaf_node_data_buffer_capacity;
    u32                       leaf_node_data_buffer_count;
    u32                       node_buffer_capacity;
    u32                       node_buffer_count;

    u32*                      p_voxels_buffer;         // Axis aligned
    tg_svo_leaf_node_data*    p_leaf_node_data_buffer;
    tg_svo_node*              p_node_buffer;           // First node is always an 'tg_svo_inner_node'
} tg_svo_header;

void tg_svo_create(v3i extent_min, v3i extent_max, u32 instance_count, const tg_instance* p_instances, TG_OUT tg_svo_header* p_header);
void tg_svo_destroy(tg_svo_header* p_header);

#endif
