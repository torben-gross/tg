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
} tg_svo_leaf_node_data;

typedef struct tg_svo_leaf_node
{
    u32    data_pointer; // Offset into the 'tg_svo_leaf_node_data' array inside of the header. '(data_pointer * header.leaf_block_size + 31) / 32' is index into voxel array of svo
} tg_svo_leaf_node;

typedef union tg_svo_node
{
    tg_svo_inner_node    inner;
    tg_svo_leaf_node     leaf;
} tg_svo_node;

typedef struct tg_svo
{
    // Note: These are floating-point vectors, so we don't need as many conversions during SVO-construction
    v3                        min;
    v3                        max;
    
    u32                       voxel_buffer_capacity_in_u32;
    u32                       voxel_buffer_count_in_u32;
    u32                       leaf_node_data_buffer_capacity;
    u32                       leaf_node_data_buffer_count;
    u32                       node_buffer_capacity;
    u32                       node_buffer_count;

    u32*                      p_voxels_buffer;         // Axis aligned
    tg_svo_leaf_node_data*    p_leaf_node_data_buffer;
    tg_svo_node*              p_node_buffer;           // First node is always an 'tg_svo_inner_node'
} tg_svo;

void tg_svo_create(v3 extent_min, v3 extent_max, u32 instance_count, const tg_instance* p_instances, const u32* p_voxel_buffer, TG_OUT tg_svo* p_svo);
void tg_svo_destroy(tg_svo* p_svo);
void tg_svo_traverse(const tg_svo* p_svo, v3 ray_origin, v3 ray_direction);

#endif
