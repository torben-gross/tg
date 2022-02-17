#include "graphics/tg_sparse_voxel_octree.h"

#include "memory/tg_memory.h"
#include "physics/tg_physics.h"



#define TG_SVO_BLOCK_MAX_VOXELS      512



static void tg__construct_leaf_node(
    tg_svo*               p_svo,
    tg_svo_leaf_node*     p_parent,
    v3                    parent_offset,
    v3                    parent_extent,
    const tg_instance*    p_instances,
    u16                   instance_id_count,
    u16*                  p_instance_ids,
    const u32*            p_voxel_buffer)
{
    TG_ASSERT((u32)parent_extent.x * (u32)parent_extent.y * (u32)parent_extent.z <= TG_SVO_BLOCK_MAX_VOXELS); // Otherwise, this should be an inner node!
    TG_ASSERT(parent_extent.x >= 1.0f);
    TG_ASSERT(parent_extent.y >= 1.0f);
    TG_ASSERT(parent_extent.z >= 1.0f);

    TG_ASSERT(instance_id_count <= sizeof(((tg_svo_leaf_node_data*)0)->p_instance_ids) / sizeof(*((tg_svo_leaf_node_data*)0)->p_instance_ids));

    TG_ASSERT(p_svo->leaf_node_data_buffer_count < p_svo->leaf_node_data_buffer_capacity);
    tg_svo_leaf_node_data* p_data = &p_svo->p_leaf_node_data_buffer[p_svo->leaf_node_data_buffer_count];
    TG_ASSERT(p_data->instance_count == 0);
    p_parent->data_pointer = p_svo->leaf_node_data_buffer_count;
    p_svo->leaf_node_data_buffer_count++;

    const u32 voxel_count = (u32)parent_extent.x * (u32)parent_extent.y * (u32)parent_extent.z;
    const u32 voxels_in_u32 = ((voxel_count + 31) / 32);
    TG_ASSERT(p_svo->voxel_buffer_count_in_u32 + voxels_in_u32 <= p_svo->voxel_buffer_capacity_in_u32);
    u32* p_block_voxels = &p_svo->p_voxels_buffer[p_svo->voxel_buffer_count_in_u32];
    p_svo->voxel_buffer_count_in_u32 += voxels_in_u32;

    const v3 block_min = tgm_v3_add(p_svo->min, parent_offset);
    const v3 block_max = tgm_v3_add(block_min, parent_extent);

    v4 p_block_corners_v4[8] = { 0 };
    p_block_corners_v4[0] = (v4){ block_min.x, block_min.y, block_min.z, 1.0f };
    p_block_corners_v4[1] = (v4){ block_max.x, block_min.y, block_min.z, 1.0f };
    p_block_corners_v4[2] = (v4){ block_min.x, block_max.y, block_min.z, 1.0f };
    p_block_corners_v4[3] = (v4){ block_max.x, block_max.y, block_min.z, 1.0f };
    p_block_corners_v4[4] = (v4){ block_min.x, block_min.y, block_max.z, 1.0f };
    p_block_corners_v4[5] = (v4){ block_max.x, block_min.y, block_max.z, 1.0f };
    p_block_corners_v4[6] = (v4){ block_min.x, block_max.y, block_max.z, 1.0f };
    p_block_corners_v4[7] = (v4){ block_max.x, block_max.y, block_max.z, 1.0f };

    for (u32 instance_id_idx = 0; instance_id_idx < instance_id_count; instance_id_idx++)
    {
        const u16 instance_id = p_instance_ids[instance_id_idx];
        const tg_instance* p_instance = &p_instances[instance_id];
        const v3 instance_half_extent = tgm_v3u_to_v3(p_instance->half_extent);

        // Instead of transforming the instance, we transform the block with the inverse of the
        // instance's model matrix, such that we can simply determine in which instance cell a
        // block's voxel center lies.
        const m4 m = tgm_m4_mul(p_instance->translation, p_instance->rotation);
        const m4 invm = tgm_m4_inverse(m);

        v3 p_block_corners_v3[8] = { 0 };
        p_block_corners_v3[0] = tgm_m4_mulv4(invm, p_block_corners_v4[0]).xyz;
        p_block_corners_v3[1] = tgm_m4_mulv4(invm, p_block_corners_v4[1]).xyz;
        p_block_corners_v3[2] = tgm_m4_mulv4(invm, p_block_corners_v4[2]).xyz;
        p_block_corners_v3[3] = tgm_m4_mulv4(invm, p_block_corners_v4[3]).xyz;
        p_block_corners_v3[4] = tgm_m4_mulv4(invm, p_block_corners_v4[4]).xyz;
        p_block_corners_v3[5] = tgm_m4_mulv4(invm, p_block_corners_v4[5]).xyz;
        p_block_corners_v3[6] = tgm_m4_mulv4(invm, p_block_corners_v4[6]).xyz;
        p_block_corners_v3[7] = tgm_m4_mulv4(invm, p_block_corners_v4[7]).xyz;

        TG_ASSERT(tg_intersect_aabb_obb(tgm_v3_neg(instance_half_extent), instance_half_extent, p_block_corners_v3));

        const v3 instance_extent = tgm_v3_mulf(instance_half_extent, 2.0f);
        const u32 instance_first_voxel_id = p_instance->first_voxel_id;

        for (i32 block_z = 0; block_z < parent_extent.z; block_z++)
        {
            const f32 tz = ((f32)block_z + 0.5f) / (f32)parent_extent.z;
            const f32 omtz = 1.0f - tz;

            const f32 pz0_z = omtz * p_block_corners_v3[0].z + tz * p_block_corners_v3[4].z;
            const f32 pz1_z = omtz * p_block_corners_v3[1].z + tz * p_block_corners_v3[5].z;

            const f32 pz_max_z = tgm_f32_ceil(TG_MAX(pz0_z, pz1_z));
            if (pz_max_z < -instance_half_extent.z)
            {
                continue;
            }
            const f32 pz_min_z = tgm_f32_floor(TG_MIN(pz0_z, pz1_z));
            if (pz_min_z > instance_half_extent.z)
            {
                continue;
            }

            const f32 pz0_x = omtz * p_block_corners_v3[0].x + tz * p_block_corners_v3[4].x;
            const f32 pz0_y = omtz * p_block_corners_v3[0].y + tz * p_block_corners_v3[4].y;

            const f32 pz1_x = omtz * p_block_corners_v3[1].x + tz * p_block_corners_v3[5].x;
            const f32 pz1_y = omtz * p_block_corners_v3[1].y + tz * p_block_corners_v3[5].y;

            const f32 pz2_x = omtz * p_block_corners_v3[2].x + tz * p_block_corners_v3[6].x;
            const f32 pz2_y = omtz * p_block_corners_v3[2].y + tz * p_block_corners_v3[6].y;
            const f32 pz2_z = omtz * p_block_corners_v3[2].z + tz * p_block_corners_v3[6].z;

            const f32 pz3_x = omtz * p_block_corners_v3[3].x + tz * p_block_corners_v3[7].x;
            const f32 pz3_y = omtz * p_block_corners_v3[3].y + tz * p_block_corners_v3[7].y;
            const f32 pz3_z = omtz * p_block_corners_v3[3].z + tz * p_block_corners_v3[7].z;

            for (i32 block_y = 0; block_y < parent_extent.y; block_y++)
            {
                const f32 ty = ((f32)block_y + 0.5f) / (f32)parent_extent.y;
                const f32 omty = 1.0f - ty;

                const f32 py0_y = omty * pz0_y + ty * pz2_y;
                const f32 py1_y = omty * pz1_y + ty * pz3_y;

                const f32 py_max_y = tgm_f32_ceil(TG_MAX(py0_y, py1_y));
                if (py_max_y < -instance_half_extent.y)
                {
                    continue;
                }
                const f32 py_min_y = tgm_f32_floor(TG_MIN(py0_y, py1_y));
                if (py_min_y > instance_half_extent.y)
                {
                    continue;
                }

                const f32 py0_x = omty * pz0_x + ty * pz2_x;
                const f32 py0_z = omty * pz0_z + ty * pz2_z;

                const f32 py1_x = omty * pz1_x + ty * pz3_x;
                const f32 py1_z = omty * pz1_z + ty * pz3_z;

                for (i32 block_x = 0; block_x < parent_extent.x; block_x++)
                {
                    const f32 tx = ((f32)block_x + 0.5f) / (f32)parent_extent.x;
                    const f32 omtx = 1.0f - tx;
                    
                    const f32 block_voxel_center_x = omtx * py0_x + tx * py1_x;
                    const f32 block_voxel_center_y = omtx * py0_y + tx * py1_y;
                    const f32 block_voxel_center_z = omtx * py0_z + tx * py1_z;

                    const f32 sign_x = (f32)((*(u32*)&block_voxel_center_x) >> 31);
                    const f32 sign_y = (f32)((*(u32*)&block_voxel_center_y) >> 31);
                    const f32 sign_z = (f32)((*(u32*)&block_voxel_center_z) >> 31);

                    const f32 block_voxel_center_floor_x = (f32)((i32)block_voxel_center_x) - sign_x;
                    const f32 block_voxel_center_floor_y = (f32)((i32)block_voxel_center_y) - sign_y;
                    const f32 block_voxel_center_floor_z = (f32)((i32)block_voxel_center_z) - sign_z;

                    const f32 instance_voxel_id_x = instance_half_extent.x + block_voxel_center_floor_x;
                    const f32 instance_voxel_id_y = instance_half_extent.y + block_voxel_center_floor_y;
                    const f32 instance_voxel_id_z = instance_half_extent.z + block_voxel_center_floor_z;

                    const b32 in_x = instance_voxel_id_x > 0.0f && instance_voxel_id_x < instance_extent.x;
                    const b32 in_y = instance_voxel_id_y > 0.0f && instance_voxel_id_y < instance_extent.y;
                    const b32 in_z = instance_voxel_id_z > 0.0f && instance_voxel_id_z < instance_extent.z;

                    if (in_x && in_y && in_z)
                    {
                        const u32 instance_voxel_id = (u32)instance_voxel_id_z * (u32)instance_extent.x * (u32)instance_extent.y + (u32)instance_voxel_id_y * (u32)instance_extent.x + (u32)instance_voxel_id_x;
                        const u32 instance_voxel_idx = instance_first_voxel_id + instance_voxel_id;
                        const u32 instance_slot_idx = instance_voxel_idx / 32;
                        const u32 instance_bit_idx = instance_voxel_idx % 32;
                        const u32 instance_slot = p_voxel_buffer[instance_slot_idx];
                        const u32 instance_bit = instance_slot & (1 << instance_bit_idx);
                        if (instance_bit != 0)
                        {
                            const u32 block_voxel_idx = (u32)(block_z * parent_extent.x * parent_extent.y + block_y * parent_extent.x + block_x);
                            const u32 block_slot_idx = block_voxel_idx / 32;
                            const u32 block_bit_idx = block_voxel_idx % 32;
                            const u32 block_bit = 1 << (block_bit_idx);
                            p_block_voxels[block_slot_idx] |= block_bit;
                            if (p_data->instance_count == 0 || p_data->p_instance_ids[p_data->instance_count - 1] != instance_id)
                            {
                                TG_ASSERT(p_data->instance_count < sizeof(p_data->p_instance_ids) / sizeof(*p_data->p_instance_ids));
                                p_data->p_instance_ids[p_data->instance_count++] = instance_id;
                            }
                        }
                    }

                    // TODO: The current method only checks, whether the voxel corresponding to the
                    // blocks voxels position is set. This is incorrect. We need to check all
                    // surrounding voxels and determine, whether the overlap/tough the current
                    // voxel and only then set the voxel in the block. The current approximation
                    // may be good enough, but if not, this needs to be added!

                    //// TODO: Depending on the rotation, this value can be something between 0.5 (0°) or 0.707... (45°)
                    //const f32 max_span = 0.70710678118f; // A voxel may span at most this half diagonal length if it 
                    //const v3 span_v = { max_span, max_span, max_span };
                    //
                    //const v3 block_voxel_min = tgm_v3_sub(block_voxel_center, span_v);
                    //const v3 block_voxel_max = tgm_v3_add(block_voxel_center, span_v);
                    //const v3 instance_cell_first = tgm_v3_floor(block_voxel_min);
                    //const v3 instance_cell_last = tgm_v3_floor(block_voxel_max);
                    //
                    //for (f32 instance_z = instance_cell_first.z; instance_z <= instance_cell_last.z; instance_z++)
                    //{
                    //    for (f32 instance_y = instance_cell_first.y; instance_y <= instance_cell_last.y; instance_y++)
                    //    {
                    //        for (f32 instance_z = instance_cell_first.z; instance_z <= instance_cell_last.z; instance_z++)
                    //        {
                    //
                    //        }
                    //    }
                    //}
                }
            }
        }
    }
}

static void tg__construct_inner_node(
    tg_svo*               p_svo,
    tg_svo_inner_node*    p_parent,
    v3                    parent_offset,
    v3                    parent_extent,
    const tg_instance*    p_instances,
    u16                   instance_id_count,
    u16*                  p_instance_ids,
    const u32*            p_voxel_buffer)
{
    TG_ASSERT((u32)parent_extent.x * (u32)parent_extent.y * (u32)parent_extent.z > TG_SVO_BLOCK_MAX_VOXELS); // Otherwise, this should be a leaf!
    TG_ASSERT(parent_extent.x >= 2.0f);
    TG_ASSERT(parent_extent.y >= 2.0f);
    TG_ASSERT(parent_extent.z >= 2.0f);

    const v3 child_extent = tgm_v3_divf(parent_extent, 2.0f);

    u8 valid_mask = 0;

    u16* p_instance_id_count_per_child = TG_NULL;
    u16* p_instance_ids_per_child      = TG_NULL;

    const tg_size instance_id_count_per_child_size = 8 * (tg_size)instance_id_count * sizeof(*p_instance_id_count_per_child);
    const tg_size instance_ids_per_child_size      = 8 * (tg_size)instance_id_count * sizeof(*p_instance_ids_per_child);

    p_instance_id_count_per_child = TG_MALLOC_STACK(instance_id_count_per_child_size);
    p_instance_ids_per_child      = TG_MALLOC_STACK(instance_ids_per_child_size);
    
    tg_memory_nullify(instance_id_count_per_child_size, p_instance_id_count_per_child);
#ifdef TG_DEBUG
    tg_memory_nullify(instance_ids_per_child_size, p_instance_ids_per_child);
#endif

    for (u32 instance_id_idx = 0; instance_id_idx < instance_id_count; instance_id_idx++)
    {
        const u16 instance_id = p_instance_ids[instance_id_idx];
        const tg_instance* p_instance = &p_instances[instance_id];
        const v3 instance_half_extent = tgm_v3u_to_v3(p_instance->half_extent);

        // Instead of transforming the instance, we transform the block with the inverse of the
        // instance's model matrix, such that we can simply determine in which instance cell a
        // block's voxel center lies.
        const m4 m = tgm_m4_mul(p_instance->translation, p_instance->rotation);
        const m4 invm = tgm_m4_inverse(m);

        for (u32 child_idx = 0; child_idx < 8; child_idx++)
        {
            // TODO: this may be optimized with & and >>
            const f32 dx = (f32)( child_idx      % 2) * child_extent.x;
            const f32 dy = (f32)((child_idx / 2) % 2) * child_extent.y;
            const f32 dz = (f32)((child_idx / 4) % 2) * child_extent.z;

            v3 child_offset = { 0 };
            child_offset.x = parent_offset.x + dx;
            child_offset.y = parent_offset.y + dy;
            child_offset.z = parent_offset.z + dz;

            const v3 child_min = tgm_v3_add(p_svo->min, child_offset);
            const v3 child_max = tgm_v3_add(child_min, child_extent);

            v4 p_child_corners_v4[8] = { 0 };
            p_child_corners_v4[0] = (v4){ child_min.x, child_min.y, child_min.z, 1.0f };
            p_child_corners_v4[1] = (v4){ child_max.x, child_min.y, child_min.z, 1.0f };
            p_child_corners_v4[2] = (v4){ child_min.x, child_max.y, child_min.z, 1.0f };
            p_child_corners_v4[3] = (v4){ child_max.x, child_max.y, child_min.z, 1.0f };
            p_child_corners_v4[4] = (v4){ child_min.x, child_min.y, child_max.z, 1.0f };
            p_child_corners_v4[5] = (v4){ child_max.x, child_min.y, child_max.z, 1.0f };
            p_child_corners_v4[6] = (v4){ child_min.x, child_max.y, child_max.z, 1.0f };
            p_child_corners_v4[7] = (v4){ child_max.x, child_max.y, child_max.z, 1.0f };

            v3 p_child_corners_v3[8] = { 0 };
            p_child_corners_v3[0] = tgm_m4_mulv4(invm, p_child_corners_v4[0]).xyz;
            p_child_corners_v3[1] = tgm_m4_mulv4(invm, p_child_corners_v4[1]).xyz;
            p_child_corners_v3[2] = tgm_m4_mulv4(invm, p_child_corners_v4[2]).xyz;
            p_child_corners_v3[3] = tgm_m4_mulv4(invm, p_child_corners_v4[3]).xyz;
            p_child_corners_v3[4] = tgm_m4_mulv4(invm, p_child_corners_v4[4]).xyz;
            p_child_corners_v3[5] = tgm_m4_mulv4(invm, p_child_corners_v4[5]).xyz;
            p_child_corners_v3[6] = tgm_m4_mulv4(invm, p_child_corners_v4[6]).xyz;
            p_child_corners_v3[7] = tgm_m4_mulv4(invm, p_child_corners_v4[7]).xyz;

            if (tg_intersect_aabb_obb(tgm_v3_neg(instance_half_extent), instance_half_extent, p_child_corners_v3))
            {
                valid_mask |= 1 << child_idx;
                if (p_instance_id_count_per_child[child_idx] == 0
                    || p_instance_ids_per_child[child_idx * instance_id_count + p_instance_id_count_per_child[child_idx] - 1] != instance_id)
                {
                    p_instance_ids_per_child[child_idx * instance_id_count + p_instance_id_count_per_child[child_idx]++] = instance_id;
                }
            }
        }
    }

    if (valid_mask)
    {
        p_parent->valid_mask = valid_mask;

        // TODO: free list and allocator stuff here
        tg_svo_node* p_child_nodes = &p_svo->p_node_buffer[p_svo->node_buffer_count];

        TG_ASSERT((tg_size)p_child_nodes > (tg_size)p_parent);
        TG_ASSERT(p_child_nodes - (tg_svo_node*)p_parent < TG_U16_MAX);

        p_parent->child_pointer = (u16)(p_child_nodes - (tg_svo_node*)p_parent);
        const u32 child_count = tgm_u16_count_set_bits(valid_mask);
        
        TG_ASSERT(p_svo->node_buffer_count + child_count <= p_svo->node_buffer_capacity);
        
        p_svo->node_buffer_count += child_count;

        const u32 child_voxel_count = (u32)child_extent.x * (u32)child_extent.y * (u32)child_extent.z;
        const b32 construct_inner_nodes_as_children = child_voxel_count > TG_SVO_BLOCK_MAX_VOXELS;

        u32 child_node_offset = 0;
        for (u32 child_idx = 0; child_idx < 8; child_idx++)
        {
            if ((valid_mask & (1 << child_idx)) != 0)
            {
                const f32 dx = (f32)( child_idx      % 2) * child_extent.x;
                const f32 dy = (f32)((child_idx / 2) % 2) * child_extent.y;
                const f32 dz = (f32)((child_idx / 4) % 2) * child_extent.z;

                v3 child_offset = { 0 };
                child_offset.x = parent_offset.x + dx;
                child_offset.y = parent_offset.y + dy;
                child_offset.z = parent_offset.z + dz;

                if (construct_inner_nodes_as_children)
                {
                    tg__construct_inner_node(
                        p_svo,
                        (tg_svo_inner_node*)&p_child_nodes[child_node_offset],
                        child_offset,
                        child_extent,
                        p_instances,
                        p_instance_id_count_per_child[child_idx],
                        &p_instance_ids_per_child[child_idx * instance_id_count],
                        p_voxel_buffer);
                }
                else
                {
                    p_parent->leaf_mask |= 1 << child_idx;
                    tg__construct_leaf_node(
                        p_svo,
                        (tg_svo_leaf_node*)&p_child_nodes[child_node_offset],
                        child_offset,
                        child_extent,
                        p_instances,
                        p_instance_id_count_per_child[child_idx],
                        &p_instance_ids_per_child[child_idx * instance_id_count],
                        p_voxel_buffer);
                }

                child_node_offset++;
            }
        }
    }

    TG_FREE_STACK(instance_ids_per_child_size);
    TG_FREE_STACK(instance_id_count_per_child_size);
}

void tg_svo_create(v3 extent_min, v3 extent_max, u32 instance_count, const tg_instance* p_instances, const u32* p_voxel_buffer, TG_OUT tg_svo* p_svo)
{
    // We expect the extent as floating-point vectors for faster SVO-construction
    TG_ASSERT(tgm_v3_eq(tgm_v3i_to_v3(tgm_v3_to_v3i(extent_min)), extent_min));
    TG_ASSERT(tgm_v3_eq(tgm_v3i_to_v3(tgm_v3_to_v3i(extent_max)), extent_max));

    // The total extent per axis must be 2^k for some natural k
    TG_ASSERT(tgm_u32_count_set_bits((u32)(extent_max.x - extent_min.x)) == 1);
    TG_ASSERT(tgm_u32_count_set_bits((u32)(extent_max.y - extent_min.y)) == 1);
    TG_ASSERT(tgm_u32_count_set_bits((u32)(extent_max.z - extent_min.z)) == 1);

    // We need to be able to split at least once
    TG_ASSERT(extent_max.x - extent_min.x >= 16.0f);
    TG_ASSERT(extent_max.y - extent_min.y >= 16.0f);
    TG_ASSERT(extent_max.z - extent_min.z >= 16.0f);

    // We store the instance count as an 16-bit unsigned integer and we use a for-loop below, which is why we have to subtract one
    TG_ASSERT(instance_count <= TG_U16_MAX - 1);

    // TODO: chunked svo construction for infinite worlds
    p_svo->min = extent_min;
    p_svo->max = extent_max;

    const v3 parent_extent = tgm_v3_sub(p_svo->max, p_svo->min);

    v3 block_extent = parent_extent;
    while ((u32)block_extent.x * (u32)block_extent.y * (u32)block_extent.z > TG_SVO_BLOCK_MAX_VOXELS)
    {
        block_extent = tgm_v3_divf(block_extent, 2.0f);
    }
    p_svo->block_extent = block_extent;
    
    p_svo->voxel_buffer_capacity_in_u32   = (1 << 17);
    p_svo->voxel_buffer_count_in_u32      = 0;
    p_svo->leaf_node_data_buffer_capacity = (1 << 13);
    p_svo->leaf_node_data_buffer_count    = 0;
    p_svo->node_buffer_capacity           = (1 << 14);
    p_svo->node_buffer_count              = 0;

    p_svo->p_voxels_buffer         = TG_MALLOC(p_svo->voxel_buffer_capacity_in_u32   * sizeof(*p_svo->p_voxels_buffer));
    p_svo->p_leaf_node_data_buffer = TG_MALLOC(p_svo->leaf_node_data_buffer_capacity * sizeof(*p_svo->p_leaf_node_data_buffer));
    p_svo->p_node_buffer           = TG_MALLOC(p_svo->node_buffer_capacity           * sizeof(*p_svo->p_node_buffer));
    
    p_svo->p_node_buffer[0].inner.child_pointer = 0;
    p_svo->p_node_buffer[0].inner.valid_mask    = 0;
    p_svo->p_node_buffer[0].inner.leaf_mask     = 0;
    p_svo->node_buffer_count++;

    tg_svo_inner_node* p_parent = (tg_svo_inner_node*)p_svo->p_node_buffer;
    const v3 parent_offset = { 0 };
    u16* p_instance_ids = TG_MALLOC_STACK(instance_count * sizeof(*p_instance_ids));
    for (u16 i = 0; i < instance_count; i++)
    {
        p_instance_ids[i] = i;
    }
    tg__construct_inner_node(p_svo, p_parent, parent_offset, parent_extent, p_instances, (u16)instance_count, p_instance_ids, p_voxel_buffer);
    TG_FREE_STACK(instance_count * sizeof(*p_instance_ids));
}

void tg_svo_destroy(tg_svo* p_svo)
{
    TG_ASSERT(p_svo);
    TG_ASSERT(p_svo->min.x < p_svo->max.x && p_svo->min.y < p_svo->max.y && p_svo->min.z < p_svo->max.z);
    TG_ASSERT(p_svo->p_voxels_buffer);
    TG_ASSERT(p_svo->p_leaf_node_data_buffer);
    TG_ASSERT(p_svo->p_node_buffer);

    TG_FREE(p_svo->p_voxels_buffer);
    TG_FREE(p_svo->p_leaf_node_data_buffer);
    TG_FREE(p_svo->p_node_buffer);
    *p_svo = (tg_svo){ 0 };
}
