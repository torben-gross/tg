#include "graphics/tg_sparse_voxel_octree.h"

#include "memory/tg_memory.h"
#include "physics/tg_physics.h"
#include "util/tg_amanatides_woo.h"



#define TG_SVO_SIDE_LENGTH                1024
#define TG_SVO_BLOCK_SIDE_LENGTH          32
#define TG_SVO_BLOCK_VOXEL_COUNT          (TG_SVO_BLOCK_SIDE_LENGTH * TG_SVO_BLOCK_SIDE_LENGTH * TG_SVO_BLOCK_SIDE_LENGTH)

TG_STATIC_ASSERT(TG_SVO_SIDE_LENGTH / TG_SVO_BLOCK_SIDE_LENGTH == 32); // Otherwise, the stack capacity below is too small
#define TG_SVO_TRAVERSE_STACK_CAPACITY    5                           // tg_u32_log2(TG_SVO_SIDE_LENGTH / TG_SVO_BLOCK_SIDE_LENGTH)



static void tg__construct_leaf_node(
    tg_svo*               p_svo,
    tg_svo_leaf_node*     p_parent,
    v3                    parent_min,
    v3                    parent_max,
    const tg_instance*    p_instances,
    u16                   instance_id_count,
    u16*                  p_instance_ids,
    const u32*            p_voxel_buffer)
{
    TG_ASSERT((u32)(parent_max.x - parent_min.x) * (u32)(parent_max.y - parent_min.y) * (u32)(parent_max.z - parent_min.z) == TG_SVO_BLOCK_VOXEL_COUNT); // Otherwise, this should be an inner node!
    TG_ASSERT(parent_max.x - parent_min.x == (f32)TG_SVO_BLOCK_SIDE_LENGTH);
    TG_ASSERT(parent_max.y - parent_min.y == (f32)TG_SVO_BLOCK_SIDE_LENGTH);
    TG_ASSERT(parent_max.z - parent_min.z == (f32)TG_SVO_BLOCK_SIDE_LENGTH);
    
    TG_ASSERT(instance_id_count < sizeof(((tg_svo_leaf_node_data*)0)->p_instance_ids) / sizeof(*((tg_svo_leaf_node_data*)0)->p_instance_ids));

    TG_ASSERT(p_svo->leaf_node_data_buffer_count < p_svo->leaf_node_data_buffer_capacity);
    tg_svo_leaf_node_data* p_data = &p_svo->p_leaf_node_data_buffer[p_svo->leaf_node_data_buffer_count];
    TG_ASSERT(p_data->instance_count == 0);
    p_parent->data_pointer = p_svo->leaf_node_data_buffer_count;
    p_svo->leaf_node_data_buffer_count++;

    const v3 parent_extent = tgm_v3_sub(parent_max, parent_min);

    const u32 voxel_count = (u32)parent_extent.x * (u32)parent_extent.y * (u32)parent_extent.z;
    const u32 voxels_in_u32 = ((voxel_count + 31) / 32);
    TG_ASSERT(p_svo->voxel_buffer_count_in_u32 + voxels_in_u32 <= p_svo->voxel_buffer_capacity_in_u32);
    u32* p_block_voxels = &p_svo->p_voxels_buffer[p_svo->voxel_buffer_count_in_u32];
    p_svo->voxel_buffer_count_in_u32 += voxels_in_u32;

    v4 p_block_corners_v4[8] = { 0 };
    p_block_corners_v4[0] = (v4){ parent_min.x, parent_min.y, parent_min.z, 1.0f };
    p_block_corners_v4[1] = (v4){ parent_max.x, parent_min.y, parent_min.z, 1.0f };
    p_block_corners_v4[2] = (v4){ parent_min.x, parent_max.y, parent_min.z, 1.0f };
    p_block_corners_v4[3] = (v4){ parent_max.x, parent_max.y, parent_min.z, 1.0f };
    p_block_corners_v4[4] = (v4){ parent_min.x, parent_min.y, parent_max.z, 1.0f };
    p_block_corners_v4[5] = (v4){ parent_max.x, parent_min.y, parent_max.z, 1.0f };
    p_block_corners_v4[6] = (v4){ parent_min.x, parent_max.y, parent_max.z, 1.0f };
    p_block_corners_v4[7] = (v4){ parent_max.x, parent_max.y, parent_max.z, 1.0f };

    for (u32 instance_id_idx = 0; instance_id_idx < instance_id_count; instance_id_idx++)
    {
        const u16 instance_id = p_instance_ids[instance_id_idx];
        const tg_instance* p_instance = &p_instances[instance_id];
        const v3 instance_half_extent = p_instance->half_extent;

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

        TG_ASSERT(tg_intersect_aabb_obb_ignore_contact(tgm_v3_neg(instance_half_extent), instance_half_extent, p_block_corners_v3));

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

                    const b32 in_x = instance_voxel_id_x >= 0.0f && instance_voxel_id_x < instance_extent.x;
                    const b32 in_y = instance_voxel_id_y >= 0.0f && instance_voxel_id_y < instance_extent.y;
                    const b32 in_z = instance_voxel_id_z >= 0.0f && instance_voxel_id_z < instance_extent.z;

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
    v3                    parent_min,
    v3                    parent_max,
    const tg_instance*    p_instances,
    u16                   instance_id_count,
    u16*                  p_instance_ids,
    const u32*            p_voxel_buffer)
{
    TG_ASSERT((u32)(parent_max.x - parent_min.x) * (u32)(parent_max.y - parent_min.y) * (u32)(parent_max.z - parent_min.z) > TG_SVO_BLOCK_VOXEL_COUNT); // Otherwise, this should be a leaf!
    TG_ASSERT(parent_max.x - parent_min.x > 8.0f);
    TG_ASSERT(parent_max.y - parent_min.y > 8.0f);
    TG_ASSERT(parent_max.z - parent_min.z > 8.0f);

    const v3 child_extent = tgm_v3_mulf(tgm_v3_sub(parent_max, parent_min), 0.5f);

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
        const v3 instance_half_extent = p_instance->half_extent;

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
            const v3 dv3 = { dx, dy, dz };

            const v3 child_min = tgm_v3_add(parent_min, dv3);
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

            if (tg_intersect_aabb_obb_ignore_contact(tgm_v3_neg(instance_half_extent), instance_half_extent, p_child_corners_v3))
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
        const u32 first_child_idx = p_svo->node_buffer_count;
        tg_svo_node* p_child_nodes = &p_svo->p_node_buffer[first_child_idx];

        TG_ASSERT((tg_size)p_child_nodes > (tg_size)p_parent);
        TG_ASSERT(p_child_nodes - (tg_svo_node*)p_parent < TG_U16_MAX);

        p_parent->child_pointer = (u16)(p_child_nodes - (tg_svo_node*)p_parent);
        const u32 child_count = tgm_u16_count_set_bits(valid_mask);
        
        TG_ASSERT(p_svo->node_buffer_count + child_count <= p_svo->node_buffer_capacity);
        
        p_svo->node_buffer_count += child_count;

        const u32 child_voxel_count = (u32)child_extent.x * (u32)child_extent.y * (u32)child_extent.z;
        TG_ASSERT(child_voxel_count >= TG_SVO_BLOCK_VOXEL_COUNT); // A child has exactly this amount of voxels, so the result should never be smaller
        const b32 construct_children_as_leaf_nodes = child_voxel_count == TG_SVO_BLOCK_VOXEL_COUNT;

        u32 child_node_offset = 0;
        for (u32 child_idx = 0; child_idx < 8; child_idx++)
        {
            if ((valid_mask & (1 << child_idx)) != 0)
            {
                const f32 dx = (f32)( child_idx      % 2) * child_extent.x;
                const f32 dy = (f32)((child_idx / 2) % 2) * child_extent.y;
                const f32 dz = (f32)((child_idx / 4) % 2) * child_extent.z;
                const v3 dv3 = { dx, dy, dz };

                const v3 child_min = tgm_v3_add(parent_min, dv3);
                const v3 child_max = tgm_v3_add(child_min, child_extent);

                if (!construct_children_as_leaf_nodes)
                {
                    tg__construct_inner_node(
                        p_svo,
                        (tg_svo_inner_node*)&p_child_nodes[child_node_offset],
                        child_min,
                        child_max,
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
                        child_min,
                        child_max,
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

void tg_svo_create(v3 svo_min, v3 svo_max, u32 instance_count, const tg_instance* p_instances, const u32* p_voxel_buffer, TG_OUT tg_svo* p_svo)
{
    TG_ASSERT(svo_max.x - svo_min.x == (f32)TG_SVO_SIDE_LENGTH);
    TG_ASSERT(svo_max.y - svo_min.y == (f32)TG_SVO_SIDE_LENGTH);
    TG_ASSERT(svo_max.z - svo_min.z == (f32)TG_SVO_SIDE_LENGTH);

    // We store the instance count as an 16-bit unsigned integer and we use a for-loop below, which is why we have to subtract one
    TG_ASSERT(instance_count <= TG_U16_MAX - 1);

    // TODO: chunked svo construction for infinite worlds
    p_svo->min = svo_min;
    p_svo->max = svo_max;
    
    p_svo->voxel_buffer_capacity_in_u32   = (1 << 21);
    p_svo->voxel_buffer_count_in_u32      = 0;
    p_svo->leaf_node_data_buffer_capacity = (1 << 13);
    p_svo->leaf_node_data_buffer_count    = 0;
    p_svo->node_buffer_capacity           = (1 << 14); // Note: We store it in 15 bits, so never more than 2^15-1
    p_svo->node_buffer_count              = 0;

    p_svo->p_voxels_buffer         = TG_MALLOC(p_svo->voxel_buffer_capacity_in_u32   * sizeof(*p_svo->p_voxels_buffer));
    p_svo->p_leaf_node_data_buffer = TG_MALLOC(p_svo->leaf_node_data_buffer_capacity * sizeof(*p_svo->p_leaf_node_data_buffer));
    p_svo->p_node_buffer           = TG_MALLOC(p_svo->node_buffer_capacity           * sizeof(*p_svo->p_node_buffer));
    
    p_svo->p_node_buffer[0].inner.child_pointer = 0;
    p_svo->p_node_buffer[0].inner.valid_mask    = 0;
    p_svo->p_node_buffer[0].inner.leaf_mask     = 0;
    p_svo->node_buffer_count++;

    tg_svo_inner_node* p_parent = (tg_svo_inner_node*)p_svo->p_node_buffer;
    u16* p_instance_ids = TG_MALLOC_STACK(instance_count * sizeof(*p_instance_ids));
    for (u16 i = 0; i < instance_count; i++)
    {
        p_instance_ids[i] = i;
    }
    tg__construct_inner_node(p_svo, p_parent, svo_min, svo_max, p_instances, (u16)instance_count, p_instance_ids, p_voxel_buffer);
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

b32 tg_svo_traverse(const tg_svo* p_svo, v3 ray_origin, v3 ray_direction, TG_OUT f32* p_distance, TG_OUT u32* p_node_idx, TG_OUT u32* p_voxel_idx)
{
    TG_ASSERT(p_svo->max.x - p_svo->min.x == 1024.0f);
    TG_ASSERT(p_svo->max.y - p_svo->min.y == 1024.0f);
    TG_ASSERT(p_svo->max.z - p_svo->min.z == 1024.0f);

    b32 result = TG_FALSE;

    *p_distance = TG_F32_MAX;
    *p_node_idx = TG_U32_MAX;
    *p_voxel_idx = TG_U32_MAX;

    const v3 extent = tgm_v3_sub(p_svo->max, p_svo->min);
    const v3 center = tgm_v3_add(p_svo->min, tgm_v3_mulf(extent, 0.5f));

    // Instead of transforming the box, the ray is transformed with its inverse
    ray_origin = tgm_v3_sub(ray_origin, center);

    u32 stack_size;
    u32 p_parent_idx_stack[TG_SVO_TRAVERSE_STACK_CAPACITY] = { 0 };
    v3  p_parent_min_stack[TG_SVO_TRAVERSE_STACK_CAPACITY] = { 0 };
    v3  p_parent_max_stack[TG_SVO_TRAVERSE_STACK_CAPACITY] = { 0 };

    f32 enter, exit;
    if (tg_intersect_ray_aabb(ray_origin, ray_direction, p_svo->min, p_svo->max, &enter, &exit))
    {
        const v3 v3_min = { TG_F32_MIN, TG_F32_MIN, TG_F32_MIN };
        const v3 v3_max = { TG_F32_MAX, TG_F32_MAX, TG_F32_MAX };

        v3 position = enter > 0.0f ? tgm_v3_add(ray_origin, tgm_v3_mulf(ray_direction, enter)) : ray_origin;

        stack_size = 1;
        p_parent_idx_stack[0] = 0;
        p_parent_min_stack[0] = p_svo->min;
        p_parent_max_stack[0] = p_svo->max;

        while (stack_size > 0)
        {
            const u32 parent_idx = p_parent_idx_stack[stack_size - 1];
            TG_ASSERT(parent_idx < p_svo->node_buffer_count);
            const v3 parent_min = p_parent_min_stack[stack_size - 1];
            const v3 parent_max = p_parent_max_stack[stack_size - 1];

            const tg_svo_inner_node* p_parent_node = (const tg_svo_inner_node*)&p_svo->p_node_buffer[parent_idx];
            const u32 child_pointer = (u32)p_parent_node->child_pointer;
            const u32 valid_mask    = (u32)p_parent_node->valid_mask;
            const u32 leaf_mask     = (u32)p_parent_node->leaf_mask;

            const v3 child_extent = tgm_v3_mulf(tgm_v3_sub(parent_max, parent_min), 0.5f);

            u32 relative_child_idx = 0;
            v3 child_min = parent_min;
            v3 child_max = tgm_v3_add(child_min, child_extent);

            if (child_max.x < position.x || position.x == child_max.x && ray_direction.x > 0.0f)
            {
                relative_child_idx += 1;
                child_min.x += child_extent.x;
                child_max.x += child_extent.x;
            }
            if (child_max.y < position.y || position.y == child_max.y && ray_direction.y > 0.0f)
            {
                relative_child_idx += 2;
                child_min.y += child_extent.y;
                child_max.y += child_extent.y;
            }
            if (child_max.z < position.z || position.z == child_max.z && ray_direction.z > 0.0f)
            {
                relative_child_idx += 4;
                child_min.z += child_extent.z;
                child_max.z += child_extent.z;
            }

            b32 advance_to_border = TG_TRUE;
            if ((valid_mask & (1 << relative_child_idx)) != 0)
            {
                // Note: The SVO may not have all children set, so the child may be stored at a smaller offset
                u32 relative_child_offset = 0;
                for (u32 i = 0; i < relative_child_idx; i++)
                {
                    relative_child_offset += (valid_mask >> i) & 1;
                }
                const u32 child_idx = parent_idx + child_pointer + relative_child_offset;

                // IF LEAF: TRAVERSE VOXELS
                // ELSE: PUSH INNER CHILD NODE ONTO STACK

                if ((leaf_mask & (1 << relative_child_idx)) != 0)
                {
                    TG_ASSERT(child_extent.x * child_extent.y * child_extent.z == TG_SVO_BLOCK_VOXEL_COUNT);

                    const tg_svo_node* p_child_node = &p_svo->p_node_buffer[child_idx];
                    const tg_svo_leaf_node_data* p_data = &p_svo->p_leaf_node_data_buffer[p_child_node->leaf.data_pointer];
                    if (p_data->instance_count != 0)
                    {
                        // TRAVERESE BLOCK

                        const u32 first_voxel_id = p_child_node->leaf.data_pointer * TG_SVO_BLOCK_VOXEL_COUNT;
                        TG_ASSERT(first_voxel_id % 32 == 0);
                        
                        const v3 ray_hit_on_grid = tgm_v3_sub(position, child_min);
                        const u32* p_voxel_grid = p_svo->p_voxels_buffer + first_voxel_id / 32;
                        v3i voxel_id;
                        if (tg_amanatides_woo(ray_hit_on_grid, ray_direction, child_extent, p_voxel_grid, &voxel_id))
                        {
                            const v3 voxel_min = tgm_v3_add(child_min, (v3) { (f32)voxel_id.x, (f32)voxel_id.y, (f32)voxel_id.z });
                            const v3 voxel_max = tgm_v3_add(voxel_min, (v3) { 1.0f, 1.0f, 1.0f });
                            const b32 intersect_ray_aabb_result = tg_intersect_ray_aabb(position, ray_direction, voxel_min, voxel_max, &enter, &exit); // TODO: We should in this case adjust the function to potentially return enter AND exit, because if we are inside of the voxel, we receive a negative 't_voxel', which results in wrong depth. Might be irrelevant...
#ifdef TG_DEBUG
                            if (!intersect_ray_aabb_result)
                            {
                                const f32 diff = enter - exit;
                                TG_ASSERT(diff < 0.001f);
                            }
#endif
                            const u32 relative_voxel_idx = (u32)child_extent.x * (u32)child_extent.y * voxel_id.z + (u32)child_extent.x * voxel_id.y + voxel_id.x;
                            const u32 voxel_idx = first_voxel_id + relative_voxel_idx;

                            result = TG_TRUE;
                            *p_distance = enter;
                            *p_node_idx = child_idx;
                            *p_voxel_idx = voxel_idx;
                            return result;
                        }
                    }
                }
                else
                {
                    TG_ASSERT(child_extent.x * child_extent.y * child_extent.z > TG_SVO_BLOCK_VOXEL_COUNT);
                    TG_ASSERT(stack_size < TG_SVO_TRAVERSE_STACK_CAPACITY);

                    advance_to_border = TG_FALSE;

                    p_parent_idx_stack[stack_size] = child_idx;
                    p_parent_min_stack[stack_size] = child_min;
                    p_parent_max_stack[stack_size] = child_max;
                    stack_size++;
                }
            }

            if (advance_to_border)
            {
                // ADVANCE POSITION TO BORDER

                v3 a = tgm_v3_div_zero_check(tgm_v3_sub(child_min, position), ray_direction, v3_min);
                v3 b = tgm_v3_div_zero_check(tgm_v3_sub(child_max, position), ray_direction, v3_max);
                v3 f = tgm_v3_max(a, b);
                exit = tgm_v3_min_elem(f);
                TG_ASSERT(exit >= 0.0f);
                const v3 advance = tgm_v3_mulf(ray_direction, exit + TG_F32_EPSILON); // TODO: epsilon required?
                position = tgm_v3_add(position, advance);

                // POP NODES WITH EXIT <= 0

                while (stack_size > 0)
                {
                    const v3 node_min = p_parent_min_stack[stack_size - 1];
                    const v3 node_max = p_parent_max_stack[stack_size - 1];
                    a = tgm_v3_div_zero_check(tgm_v3_sub(node_min, position), ray_direction, v3_min);
                    b = tgm_v3_div_zero_check(tgm_v3_sub(node_max, position), ray_direction, v3_max);
                    f = tgm_v3_max(a, b);
                    exit = tgm_v3_min_elem(f);
                    if (exit > TG_F32_EPSILON)
                    {
                        break;
                    }
                    stack_size--;
#ifdef TG_DEBUG
                    p_parent_idx_stack[stack_size] = 0;
                    p_parent_min_stack[stack_size] = (v3){ 0 };
                    p_parent_max_stack[stack_size] = (v3){ 0 };
#endif
                }
            }
        }
    }

    return result;
}
