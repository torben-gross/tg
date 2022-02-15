#include "graphics/tg_sparse_voxel_octree.h"

#include "memory/tg_memory.h"
#include "physics/tg_physics.h"



static void tg__construct_leaf_node(
    tg_svo_header*        p_header,
    tg_svo_leaf_node*     p_parent,
    v3i                   parent_offset,
    v3i                   parent_extent,
    u32                   instance_count,
    const tg_instance*    p_instances)
{
    TG_ASSERT(parent_extent.x * parent_extent.y * parent_extent.z <= 512); // Otherwise, this should be an inner node!
    TG_ASSERT(parent_extent.x >= 1);
    TG_ASSERT(parent_extent.y >= 1);
    TG_ASSERT(parent_extent.z >= 1);

    const u32 voxel_count = parent_extent.x * parent_extent.y * parent_extent.z;
    const u32 voxels_size = ((voxel_count + 31) / 32);
    TG_ASSERT(p_header->voxels_buffer_size + voxels_size <= p_header->voxels_buffer_capacity);
    u32* p_voxels = &p_header->p_voxels_buffer[p_header->voxels_buffer_size];
    p_header->voxels_buffer_size += voxels_size;

    const v3i block_min = tgm_v3i_add(p_header->min, parent_offset);
    const v3i block_max = tgm_v3i_add(block_min, parent_extent);

    u32 icnt = 0;

    for (u32 instance_idx = 0; instance_idx < instance_count; instance_idx++)
    {
        const tg_instance* p_instance = &p_instances[instance_idx];
        const v3i instance_max = tgm_v3u_to_v3i(p_instance->half_extent);
        const v3i instance_min = tgm_v3i_neg(instance_max);

        // Instead of transforming the instance, we transform the block with the inverse of the
        // instance's model matrix, such that we can simply determine in which instance cell a
        // block's voxel center lies.
        const m4 m = tgm_m4_mul(p_instance->translation, p_instance->rotation);
        const m4 invm = tgm_m4_inverse(m);

        v4 p_block_corners[8] = { 0 };
        p_block_corners[0] = (v4){ (f32)block_min.x, (f32)block_min.y, (f32)block_min.z, 1.0f };
        p_block_corners[1] = (v4){ (f32)block_max.x, (f32)block_min.y, (f32)block_min.z, 1.0f };
        p_block_corners[2] = (v4){ (f32)block_min.x, (f32)block_max.y, (f32)block_min.z, 1.0f };
        p_block_corners[3] = (v4){ (f32)block_max.x, (f32)block_max.y, (f32)block_min.z, 1.0f };
        p_block_corners[4] = (v4){ (f32)block_min.x, (f32)block_min.y, (f32)block_max.z, 1.0f };
        p_block_corners[5] = (v4){ (f32)block_max.x, (f32)block_min.y, (f32)block_max.z, 1.0f };
        p_block_corners[6] = (v4){ (f32)block_min.x, (f32)block_max.y, (f32)block_max.z, 1.0f };
        p_block_corners[7] = (v4){ (f32)block_max.x, (f32)block_max.y, (f32)block_max.z, 1.0f };

        p_block_corners[0] = tgm_m4_mulv4(invm, p_block_corners[0]);
        p_block_corners[1] = tgm_m4_mulv4(invm, p_block_corners[1]);
        p_block_corners[2] = tgm_m4_mulv4(invm, p_block_corners[2]);
        p_block_corners[3] = tgm_m4_mulv4(invm, p_block_corners[3]);
        p_block_corners[4] = tgm_m4_mulv4(invm, p_block_corners[4]);
        p_block_corners[5] = tgm_m4_mulv4(invm, p_block_corners[5]);
        p_block_corners[6] = tgm_m4_mulv4(invm, p_block_corners[6]);
        p_block_corners[7] = tgm_m4_mulv4(invm, p_block_corners[7]);

        for (i32 block_z = 0; block_z < parent_extent.z; block_z++)
        {
            const f32 tz = ((f32)block_z + 0.5f) / (f32)parent_extent.z;
            const v3 pz0 = tgm_v3_lerp(p_block_corners[0].xyz, p_block_corners[4].xyz, tz);
            const v3 pz1 = tgm_v3_lerp(p_block_corners[1].xyz, p_block_corners[5].xyz, tz);
            const v3 pz2 = tgm_v3_lerp(p_block_corners[2].xyz, p_block_corners[6].xyz, tz);
            const v3 pz3 = tgm_v3_lerp(p_block_corners[3].xyz, p_block_corners[7].xyz, tz);

            for (i32 block_y = 0; block_y < parent_extent.y; block_y++)
            {
                const f32 ty = ((f32)block_y + 0.5f) / (f32)parent_extent.y;
                const v3 py0 = tgm_v3_lerp(pz0, pz2, ty);
                const v3 py1 = tgm_v3_lerp(pz1, pz3, ty);

                for (i32 block_x = 0; block_x < parent_extent.x; block_x++)
                {
                    const f32 tx = ((f32)block_x + 0.5f) / (f32)parent_extent.x;
                    const v3 block_voxel_center = tgm_v3_lerp(py0, py1, tx);
                    const v3 instance_cell_idx = tgm_v3_floor(block_voxel_center);

                    for (f32 dz = -1.0f; dz < 1.5f; dz += 1.0f)
                    {
                        if ((i32)(instance_cell_idx.z + dz) < instance_min.z || (i32)(instance_cell_idx.z + dz) >= instance_max.z)
                        {
                            continue;
                        }

                        const f32 minz = instance_cell_idx.z + dz - 0.5f;
                        const f32 maxz = instance_cell_idx.z + dz + 1.5f;

                        for (f32 dy = -1.0f; dy < 1.5f; dy += 1.0f)
                        {
                            if ((i32)(instance_cell_idx.y + dy) < instance_min.y || (i32)(instance_cell_idx.y + dy) >= instance_max.y)
                            {
                                continue;
                            }

                            const f32 miny = instance_cell_idx.y + dy - 0.5f;
                            const f32 maxy = instance_cell_idx.y + dy + 1.5f;

                            for (f32 dx = -1.0f; dx < 1.5f; dx += 1.0f)
                            {
                                if ((i32)(instance_cell_idx.x + dx) < instance_min.x || (i32)(instance_cell_idx.x + dx) >= instance_max.x)
                                {
                                    continue;
                                }

                                const f32 minx = instance_cell_idx.x + dx - 0.5f;
                                const f32 maxx = instance_cell_idx.x + dx + 1.5f;

                                int bh = 0;
                                icnt++;
                            }
                        }
                    }
                }
            }
        }
    }
    TG_ASSERT(icnt > 0);
}

static void tg__construct_inner_node(
    tg_svo_header*        p_header,
    tg_svo_inner_node*    p_parent,
    v3i                   parent_offset,
    v3i                   parent_extent,
    u32                   instance_count,
    const tg_instance*    p_instances)
{
    TG_ASSERT(parent_extent.x * parent_extent.y * parent_extent.z > 512); // Otherwise, this should be a leaf!
    TG_ASSERT(parent_extent.x >= 2);
    TG_ASSERT(parent_extent.y >= 2);
    TG_ASSERT(parent_extent.z >= 2);

    const v3i child_extent = tgm_v3i_divi(parent_extent, 2);

    u8 valid_mask = 0;

    // May may already know that we need all children
    if (valid_mask != 0xff)
    {
        for (u32 instance_idx = 0; instance_idx < instance_count; instance_idx++)
        {
            const tg_instance* p_instance = &p_instances[instance_idx];
            const m4 m = tgm_m4_mul(p_instance->translation, p_instance->rotation);

            v4 p_instance_corners_v4[8] = { 0 };
            p_instance_corners_v4[0] = (v4){ -(f32)p_instance->half_extent.x, -(f32)p_instance->half_extent.y, -(f32)p_instance->half_extent.z, 1.0f };
            p_instance_corners_v4[1] = (v4){  (f32)p_instance->half_extent.x, -(f32)p_instance->half_extent.y, -(f32)p_instance->half_extent.z, 1.0f };
            p_instance_corners_v4[2] = (v4){ -(f32)p_instance->half_extent.x,  (f32)p_instance->half_extent.y, -(f32)p_instance->half_extent.z, 1.0f };
            p_instance_corners_v4[3] = (v4){  (f32)p_instance->half_extent.x,  (f32)p_instance->half_extent.y, -(f32)p_instance->half_extent.z, 1.0f };
            p_instance_corners_v4[4] = (v4){ -(f32)p_instance->half_extent.x, -(f32)p_instance->half_extent.y,  (f32)p_instance->half_extent.z, 1.0f };
            p_instance_corners_v4[5] = (v4){  (f32)p_instance->half_extent.x, -(f32)p_instance->half_extent.y,  (f32)p_instance->half_extent.z, 1.0f };
            p_instance_corners_v4[6] = (v4){ -(f32)p_instance->half_extent.x,  (f32)p_instance->half_extent.y,  (f32)p_instance->half_extent.z, 1.0f };
            p_instance_corners_v4[7] = (v4){  (f32)p_instance->half_extent.x,  (f32)p_instance->half_extent.y,  (f32)p_instance->half_extent.z, 1.0f };

            v3 p_instance_corners_v3[8] = { 0 };
            p_instance_corners_v3[0] = tgm_m4_mulv4(m, p_instance_corners_v4[0]).xyz;
            p_instance_corners_v3[1] = tgm_m4_mulv4(m, p_instance_corners_v4[1]).xyz;
            p_instance_corners_v3[2] = tgm_m4_mulv4(m, p_instance_corners_v4[2]).xyz;
            p_instance_corners_v3[3] = tgm_m4_mulv4(m, p_instance_corners_v4[3]).xyz;
            p_instance_corners_v3[4] = tgm_m4_mulv4(m, p_instance_corners_v4[4]).xyz;
            p_instance_corners_v3[5] = tgm_m4_mulv4(m, p_instance_corners_v4[5]).xyz;
            p_instance_corners_v3[6] = tgm_m4_mulv4(m, p_instance_corners_v4[6]).xyz;
            p_instance_corners_v3[7] = tgm_m4_mulv4(m, p_instance_corners_v4[7]).xyz;

            for (u32 child_idx = 0; child_idx < 8; child_idx++)
            {
                // TODO: this may be optimized with & and >>
                const i32 dx = (child_idx % 2) * child_extent.x;
                const i32 dy = ((child_idx / 2) % 2) * child_extent.y;
                const i32 dz = ((child_idx / 4) % 2) * child_extent.z;

                v3i child_offset = { 0 };
                child_offset.x = parent_offset.x + dx;
                child_offset.y = parent_offset.y + dy;
                child_offset.z = parent_offset.z + dz;

                const v3i child_min = tgm_v3i_add(p_header->min, child_offset);
                const v3i child_max = tgm_v3i_add(child_min, child_extent);

                if (tg_intersect_aabb_obb(tgm_v3i_to_v3(child_min), tgm_v3i_to_v3(child_max), p_instance_corners_v3))
                {
                    valid_mask |= 1 << child_idx;

                    // If we require all children to be present, we can stop constructing the valid mask here
                    if (valid_mask == 0xff)
                    {
                        break;
                    }
                }
            }

            // If we require all children to be present, we can stop constructing the valid mask here
            if (valid_mask == 0xff)
            {
                break;
            }
        }
    }

    if (valid_mask)
    {
        p_parent->valid_mask = valid_mask;

        // TODO: free list and allocator stuff here
        tg_svo_node* p_child_nodes = &p_header->p_node_buffer[p_header->node_buffer_count];

        TG_ASSERT((tg_size)p_child_nodes > (tg_size)p_parent);
        TG_ASSERT(p_child_nodes - (tg_svo_node*)p_parent < TG_U16_MAX);

        p_parent->child_pointer = (u16)(p_child_nodes - (tg_svo_node*)p_parent);
        const u32 child_count = tgm_u16_count_set_bits(valid_mask);
        
        TG_ASSERT(p_header->node_buffer_count + child_count <= p_header->node_buffer_capacity);
        
        p_header->node_buffer_count += child_count;

        const i32 child_voxel_count = child_extent.x * child_extent.y * child_extent.z;
        u32 child_idx = 0;
        for (u32 corner_idx = 0; corner_idx < 8; corner_idx++)
        {
            if ((valid_mask & (1 << corner_idx)) != 0)
            {
                const i32 dx = (corner_idx % 2) * child_extent.x;
                const i32 dy = ((corner_idx / 2) % 2) * child_extent.y;
                const i32 dz = ((corner_idx / 4) % 2) * child_extent.z;

                v3i child_offset = { 0 };
                child_offset.x = parent_offset.x + dx;
                child_offset.y = parent_offset.y + dy;
                child_offset.z = parent_offset.z + dz;

                if (child_voxel_count > 512)
                {
                    tg__construct_inner_node(p_header, (tg_svo_inner_node*)&p_child_nodes[child_idx], child_offset, child_extent, instance_count, p_instances);
                }
                else
                {
                    p_parent->leaf_mask |= 1 << corner_idx;
                    tg__construct_leaf_node(p_header, (tg_svo_leaf_node*)&p_child_nodes[child_idx], child_offset, child_extent, instance_count, p_instances);
                }

                child_idx++;
            }
        }
    }
}

void tg_svo_create(v3i extent_min, v3i extent_max, u32 instance_count, const tg_instance* p_instances, TG_OUT tg_svo_header* p_header)
{
    // The total extent per axis must be 2^k for some natural k
    TG_ASSERT(tgm_u32_count_set_bits(extent_max.x - extent_min.x) == 1);
    TG_ASSERT(tgm_u32_count_set_bits(extent_max.y - extent_min.y) == 1);
    TG_ASSERT(tgm_u32_count_set_bits(extent_max.z - extent_min.z) == 1);

    // We need to be able to split at least once
    TG_ASSERT(extent_max.x - extent_min.x >= 16);
    TG_ASSERT(extent_max.y - extent_min.y >= 16);
    TG_ASSERT(extent_max.z - extent_min.z >= 16);

    // TODO: chunked svo construction for infinite worlds
    p_header->min = extent_min;
    p_header->max = extent_max;
    
    p_header->voxels_buffer_capacity         = (1 << 16);
    p_header->voxels_buffer_size            = 0;
    p_header->leaf_node_data_buffer_capacity = 16;
    p_header->leaf_node_data_buffer_count    = 0;
    p_header->node_buffer_capacity           = 128;
    p_header->node_buffer_count              = 0;

    p_header->p_voxels_buffer         = TG_MALLOC(p_header->voxels_buffer_capacity         * sizeof(*p_header->p_voxels_buffer));
    p_header->p_leaf_node_data_buffer = TG_MALLOC(p_header->leaf_node_data_buffer_capacity * sizeof(*p_header->p_leaf_node_data_buffer));
    p_header->p_node_buffer           = TG_MALLOC(p_header->node_buffer_capacity           * sizeof(*p_header->p_node_buffer));
    
    p_header->p_node_buffer[0].inner.child_pointer = 0;
    p_header->p_node_buffer[0].inner.valid_mask    = 0;
    p_header->p_node_buffer[0].inner.leaf_mask     = 0;
    p_header->node_buffer_count++;

    tg_svo_inner_node* p_parent = (tg_svo_inner_node*)p_header->p_node_buffer;
    const v3i parent_offset = { 0, 0, 0 };
    const v3i parent_extent = tgm_v3i_sub(p_header->max, p_header->min);
    tg__construct_inner_node(p_header, p_parent, parent_offset, parent_extent, instance_count, p_instances);
}

void tg_svo_destroy(tg_svo_header* p_header)
{
    TG_ASSERT(p_header);
    TG_ASSERT(p_header->min.x < p_header->max.x && p_header->min.y < p_header->max.y && p_header->min.z < p_header->max.z);
    TG_ASSERT(p_header->p_voxels_buffer);
    TG_ASSERT(p_header->p_leaf_node_data_buffer);
    TG_ASSERT(p_header->p_node_buffer);

    TG_FREE(p_header->p_voxels_buffer);
    TG_FREE(p_header->p_leaf_node_data_buffer);
    TG_FREE(p_header->p_node_buffer);
    *p_header = (tg_svo_header){ 0 };
}
