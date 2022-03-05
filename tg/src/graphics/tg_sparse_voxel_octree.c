#include "graphics/tg_sparse_voxel_octree.h"

#include "memory/tg_memory.h"
#include "physics/tg_physics.h"
#include "util/tg_amanatides_woo.h"

#include "graphics/vulkan/tgvk_raytracer.h" // TODO: this is awful!



#define TG_SVO_SIDE_LENGTH                1024
#define TG_SVO_BLOCK_SIDE_LENGTH          32
#define TG_SVO_BLOCK_VOXEL_COUNT          (TG_SVO_BLOCK_SIDE_LENGTH * TG_SVO_BLOCK_SIDE_LENGTH * TG_SVO_BLOCK_SIDE_LENGTH)

TG_STATIC_ASSERT(TG_SVO_SIDE_LENGTH / TG_SVO_BLOCK_SIDE_LENGTH == 32); // Otherwise, the stack capacity below is too small
#define TG_SVO_TRAVERSE_STACK_CAPACITY    5                            // tg_u32_log2(TG_SVO_SIDE_LENGTH / TG_SVO_BLOCK_SIDE_LENGTH)

#define TG_CLUSTER_MIN                    (v3){ 0.0f, 0.0f, 0.0f }
#define TG_CLUSTER_MAX                    (v3){ (f32)TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT, (f32)TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT, (f32)TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT }



static m4 tg__world_space_to_model_space(const tg_voxel_object* p_object, u32 relative_cluster_idx)
{
    const m4 rotation = tgm_m4_angle_axis(p_object->angle_in_radians, p_object->axis);

    const u32 relative_cluster_idx_x = relative_cluster_idx % p_object->n_clusters_per_dim.x;
    const u32 relative_cluster_idx_y = (relative_cluster_idx / p_object->n_clusters_per_dim.x) % p_object->n_clusters_per_dim.y;
    const u32 relative_cluster_idx_z = relative_cluster_idx / (p_object->n_clusters_per_dim.x * p_object->n_clusters_per_dim.y); // Note: We don't need modulo here, because z doesn't loop

    const f32 relative_cluster_offset_x = (f32)(relative_cluster_idx_x * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
    const f32 relative_cluster_offset_y = (f32)(relative_cluster_idx_y * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
    const f32 relative_cluster_offset_z = (f32)(relative_cluster_idx_z * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
    const v3 relative_cluster_offset = { relative_cluster_offset_x, relative_cluster_offset_y, relative_cluster_offset_z };

    const f32 h = (f32)(TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT / 2);
    const v3 cluster_half_extent = { h, h, h };
    const v3 n_clusters_per_dim = { (f32)p_object->n_clusters_per_dim.x, (f32)p_object->n_clusters_per_dim.y, (f32)p_object->n_clusters_per_dim.z };
    const v3 object_half_extent = tgm_v3_mul(n_clusters_per_dim, cluster_half_extent);

    // Input:
    //   8 corners of cluster in world space
    // 
    // Output:
    //   Unrotated 8 corners of cluster with min corner at (0,0,0) and max corner at (8,8,8)
    // 
    // Transformation:
    //   1. Translate by objects inverse translation    (=> Center is at origin)
    //   2. Rotate by objects inverse rotation          (=> Object is not rotated)
    //   3. Translate by objects half extent (+)        (=> Objects min corner is at (0,0,0), max corner is at its extent)
    //   4. Translate by clusters inverse offset (-)    (=> Clusters min corner is at (0,0,0), max corner is at (8,8,8))

    const m4 ws2ms1 = tgm_m4_translate(tgm_v3_neg(p_object->translation));               // (1)
    const m4 ws2ms2 = tgm_m4_inverse(rotation);                                          // (2)
    const m4 ws2ms3 = tgm_m4_translate(object_half_extent);                              // (3)
    const m4 ws2ms4 = tgm_m4_translate(tgm_v3_neg(relative_cluster_offset));             // (4)
    const m4 ws2ms = tgm_m4_mul(tgm_m4_mul(tgm_m4_mul(ws2ms4, ws2ms3), ws2ms2), ws2ms1);

    return ws2ms;
}

static m4 tg__model_space_to_world_space(const tg_voxel_object* p_object, u32 relative_cluster_idx)
{
    const u32 relative_cluster_idx_x = relative_cluster_idx % p_object->n_clusters_per_dim.x;
    const u32 relative_cluster_idx_y = (relative_cluster_idx / p_object->n_clusters_per_dim.x) % p_object->n_clusters_per_dim.y;
    const u32 relative_cluster_idx_z = relative_cluster_idx / (p_object->n_clusters_per_dim.x * p_object->n_clusters_per_dim.y); // Note: We don't need modulo here, because z doesn't loop
    
    const f32 relative_cluster_offset_x = (f32)(relative_cluster_idx_x * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
    const f32 relative_cluster_offset_y = (f32)(relative_cluster_idx_y * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
    const f32 relative_cluster_offset_z = (f32)(relative_cluster_idx_z * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
    const v3 relative_cluster_offset = { relative_cluster_offset_x, relative_cluster_offset_y, relative_cluster_offset_z };

    const f32 h = (f32)(TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT / 2);
    const v3 cluster_half_extent = { h, h, h };
    const v3 n_clusters_per_dim = { (f32)p_object->n_clusters_per_dim.x, (f32)p_object->n_clusters_per_dim.y, (f32)p_object->n_clusters_per_dim.z };
    const v3 object_half_extent = tgm_v3_mul(n_clusters_per_dim, cluster_half_extent);

    const m4 rotation = tgm_m4_angle_axis(p_object->angle_in_radians, p_object->axis);

    // Input:
    //   Unrotated 8 corners of cluster with min corner at (0,0,0) and max corner at (8,8,8)
    // 
    // Output:
    //   8 corners of cluster in world space
    // 
    // Transformation:
    //   1. Translate by clusters offset (+)        (=> Cluster is translated relative inside of object. Clusters min corner is at offset, max corner is at offset + cluster extent)
    //   2. Translate by objects half extent (-)    (=> Object is centered at origin)
    //   3. Rotate by objects rotation              (=> Object is rotated)
    //   4. Translate by objects translation        (=> Object is translated in world space)

    const m4 ms2ws1 = tgm_m4_translate(relative_cluster_offset);           // (1)
    const m4 ms2ws2 = tgm_m4_translate(tgm_v3_neg(object_half_extent));    // (2)
    const m4 ms2ws3 = rotation;                                            // (3)
    const m4 ms2ws4 = tgm_m4_translate(p_object->translation);             // (4)

    const m4 ms2ws = tgm_m4_mul(tgm_m4_mul(tgm_m4_mul(ms2ws4, ms2ws3), ms2ws2), ms2ws1);

    return ms2ws;
}

static void tg__construct_leaf_node(
    tg_svo*                   p_svo,
    tg_svo_leaf_node*         p_parent,
    v3                        parent_min,
    v3                        parent_max,
    u32                       n_clusters,
    const u32*                p_cluster_idcs,
    const tg_scene*           p_scene)
{
    TG_ASSERT((u32)(parent_max.x - parent_min.x) * (u32)(parent_max.y - parent_min.y) * (u32)(parent_max.z - parent_min.z) == TG_SVO_BLOCK_VOXEL_COUNT); // Otherwise, this should be an inner node!
    TG_ASSERT(parent_max.x - parent_min.x == (f32)TG_SVO_BLOCK_SIDE_LENGTH);
    TG_ASSERT(parent_max.y - parent_min.y == (f32)TG_SVO_BLOCK_SIDE_LENGTH);
    TG_ASSERT(parent_max.z - parent_min.z == (f32)TG_SVO_BLOCK_SIDE_LENGTH);

    TG_ASSERT(p_svo->leaf_node_data_buffer_count < p_svo->leaf_node_data_buffer_capacity);
    tg_svo_leaf_node_data* p_data = &p_svo->p_leaf_node_data_buffer[p_svo->leaf_node_data_buffer_count];
    TG_ASSERT(p_data->n == 0); // Uninitialized
    p_parent->data_pointer = p_svo->leaf_node_data_buffer_count++;

    const v3 parent_extent = tgm_v3_sub(parent_max, parent_min);
    const u32 voxel_count = (u32)parent_extent.x * (u32)parent_extent.y * (u32)parent_extent.z;
    const u32 voxels_in_u32 = ((voxel_count + 31) / 32);
    TG_ASSERT(p_svo->voxel_buffer_count_in_u32 + voxels_in_u32 <= p_svo->voxel_buffer_capacity_in_u32);
    u32* p_block_voxels = &p_svo->p_voxels_buffer[p_svo->voxel_buffer_count_in_u32];
    p_svo->voxel_buffer_count_in_u32 += voxels_in_u32;

    // Construct 8 cluster corners (cluster space)
    //   Min corner := (0,0,0)
    //   Max corner := (8,8,8)

    const f32 ext_c = (f32)TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT;
    
    v4 p_cluster_corners_cs[8] = { 0 };
    p_cluster_corners_cs[0] = (v4){  0.0f,  0.0f,  0.0f, 1.0f };
    p_cluster_corners_cs[1] = (v4){ ext_c,  0.0f,  0.0f, 1.0f };
    p_cluster_corners_cs[2] = (v4){  0.0f, ext_c,  0.0f, 1.0f };
    p_cluster_corners_cs[3] = (v4){ ext_c, ext_c,  0.0f, 1.0f };
    p_cluster_corners_cs[4] = (v4){  0.0f,  0.0f, ext_c, 1.0f };
    p_cluster_corners_cs[5] = (v4){ ext_c,  0.0f, ext_c, 1.0f };
    p_cluster_corners_cs[6] = (v4){  0.0f, ext_c, ext_c, 1.0f };
    p_cluster_corners_cs[7] = (v4){ ext_c, ext_c, ext_c, 1.0f };

    // Construct 8 block corners (world space)

    v4 p_block_corners_ws[8] = { 0 };
    p_block_corners_ws[0] = (v4){ parent_min.x, parent_min.y, parent_min.z, 1.0f };
    p_block_corners_ws[1] = (v4){ parent_max.x, parent_min.y, parent_min.z, 1.0f };
    p_block_corners_ws[2] = (v4){ parent_min.x, parent_max.y, parent_min.z, 1.0f };
    p_block_corners_ws[3] = (v4){ parent_max.x, parent_max.y, parent_min.z, 1.0f };
    p_block_corners_ws[4] = (v4){ parent_min.x, parent_min.y, parent_max.z, 1.0f };
    p_block_corners_ws[5] = (v4){ parent_max.x, parent_min.y, parent_max.z, 1.0f };
    p_block_corners_ws[6] = (v4){ parent_min.x, parent_max.y, parent_max.z, 1.0f };
    p_block_corners_ws[7] = (v4){ parent_max.x, parent_max.y, parent_max.z, 1.0f };

    for (u32 cluster_idcs_idx = 0; cluster_idcs_idx < n_clusters; cluster_idcs_idx++)
    {
        const u32 cluster_idx = p_cluster_idcs[cluster_idcs_idx];
        const u32 object_idx = p_scene->p_cluster_idx_to_object_idx[cluster_idx];
        const tg_voxel_object* p_object = &p_scene->p_objects[object_idx];
        const u32 relative_cluster_idx = cluster_idx - p_object->first_cluster_idx;

        // Transform 8 cluster corners (cluster space -> world space)

        const m4 cs2ws = tg__model_space_to_world_space(p_object, relative_cluster_idx);

        v3 p_cluster_corners_ws[8] = { 0 };
        p_cluster_corners_ws[0] = tgm_m4_mulv4(cs2ws, p_cluster_corners_cs[0]).xyz;
        p_cluster_corners_ws[1] = tgm_m4_mulv4(cs2ws, p_cluster_corners_cs[1]).xyz;
        p_cluster_corners_ws[2] = tgm_m4_mulv4(cs2ws, p_cluster_corners_cs[2]).xyz;
        p_cluster_corners_ws[3] = tgm_m4_mulv4(cs2ws, p_cluster_corners_cs[3]).xyz;
        p_cluster_corners_ws[4] = tgm_m4_mulv4(cs2ws, p_cluster_corners_cs[4]).xyz;
        p_cluster_corners_ws[5] = tgm_m4_mulv4(cs2ws, p_cluster_corners_cs[5]).xyz;
        p_cluster_corners_ws[6] = tgm_m4_mulv4(cs2ws, p_cluster_corners_cs[6]).xyz;
        p_cluster_corners_ws[7] = tgm_m4_mulv4(cs2ws, p_cluster_corners_cs[7]).xyz;

        // Compute min corner and max corner of 8 world space cluster corners

        const v3 min_c = tgm_v3_min(
            tgm_v3_min(
                tgm_v3_min(p_cluster_corners_ws[0], p_cluster_corners_ws[1]),
                tgm_v3_min(p_cluster_corners_ws[2], p_cluster_corners_ws[2])),
            tgm_v3_min(
                tgm_v3_min(p_cluster_corners_ws[4], p_cluster_corners_ws[6]),
                tgm_v3_min(p_cluster_corners_ws[5], p_cluster_corners_ws[7])));

        const v3 max_c = tgm_v3_max(
            tgm_v3_max(
                tgm_v3_max(p_cluster_corners_ws[0], p_cluster_corners_ws[1]),
                tgm_v3_max(p_cluster_corners_ws[2], p_cluster_corners_ws[2])),
            tgm_v3_max(
                tgm_v3_max(p_cluster_corners_ws[4], p_cluster_corners_ws[6]),
                tgm_v3_max(p_cluster_corners_ws[5], p_cluster_corners_ws[7])));

        const v3 floor_min_c = tgm_v3_floor(min_c);
        const v3 ceil_max_c = tgm_v3_ceil(max_c);

        // Transform 8 trimmed block corners (world space -> cluster space)

        //const m4 ws2cs = tg__world_space_to_model_space(p_object, relative_cluster_idx);
        const m4 ws2cs = tgm_m4_inverse(cs2ws);

        v3 p_block_corners_cs[8] = { 0 };
        p_block_corners_cs[0] = tgm_m4_mulv4(ws2cs, p_block_corners_ws[0]).xyz;
        p_block_corners_cs[1] = tgm_m4_mulv4(ws2cs, p_block_corners_ws[1]).xyz;
        p_block_corners_cs[2] = tgm_m4_mulv4(ws2cs, p_block_corners_ws[2]).xyz;
        p_block_corners_cs[3] = tgm_m4_mulv4(ws2cs, p_block_corners_ws[3]).xyz;
        p_block_corners_cs[4] = tgm_m4_mulv4(ws2cs, p_block_corners_ws[4]).xyz;
        p_block_corners_cs[5] = tgm_m4_mulv4(ws2cs, p_block_corners_ws[5]).xyz;
        p_block_corners_cs[6] = tgm_m4_mulv4(ws2cs, p_block_corners_ws[6]).xyz;
        p_block_corners_cs[7] = tgm_m4_mulv4(ws2cs, p_block_corners_ws[7]).xyz;

        TG_ASSERT(tg_intersect_aabb_obb_ignore_contact(TG_CLUSTER_MIN, TG_CLUSTER_MAX, p_block_corners_cs));

        // Compute trimmed min corner and max corner of world space block corners. They define the
        // extent of the traversal.

        const v3 trimmed_min_b = tgm_v3_max(parent_min, floor_min_c);
        const v3 trimmed_max_b = tgm_v3_min(parent_max, ceil_max_c);

        TG_ASSERT(trimmed_min_b.x - parent_min.x >= 0.0f && trimmed_min_b.x - parent_min.x <= parent_extent.x); // Implementation check: Within block extent?
        TG_ASSERT(trimmed_min_b.y - parent_min.y >= 0.0f && trimmed_min_b.y - parent_min.y <= parent_extent.y);
        TG_ASSERT(trimmed_min_b.z - parent_min.z >= 0.0f && trimmed_min_b.z - parent_min.z <= parent_extent.z);
        TG_ASSERT(trimmed_max_b.x - parent_min.x >= 0.0f && trimmed_max_b.x - parent_min.x <= parent_extent.x);
        TG_ASSERT(trimmed_max_b.y - parent_min.y >= 0.0f && trimmed_max_b.y - parent_min.y <= parent_extent.y);
        TG_ASSERT(trimmed_max_b.z - parent_min.z >= 0.0f && trimmed_max_b.z - parent_min.z <= parent_extent.z);

        const u32 trimmed_min_b_x = (u32)(trimmed_min_b.x - parent_min.x);
        const u32 trimmed_min_b_y = (u32)(trimmed_min_b.y - parent_min.y);
        const u32 trimmed_min_b_z = (u32)(trimmed_min_b.z - parent_min.z);
        const u32 trimmed_max_b_x = (u32)(trimmed_max_b.x - parent_min.x);
        const u32 trimmed_max_b_y = (u32)(trimmed_max_b.y - parent_min.y);
        const u32 trimmed_max_b_z = (u32)(trimmed_max_b.z - parent_min.z);

        TG_ASSERT(trimmed_min_b_x <= trimmed_max_b_x); // Logic check: Min leq max?
        TG_ASSERT(trimmed_min_b_y <= trimmed_max_b_y);
        TG_ASSERT(trimmed_min_b_z <= trimmed_max_b_z);

        TG_ASSERT((u64)cluster_idx * (u64)TG_N_PRIMITIVES_PER_CLUSTER <= TG_U32_MAX);
        const u32 cluster_offset_in_voxels = cluster_idx * TG_N_PRIMITIVES_PER_CLUSTER;

        for (u32 trimmed_b_z = trimmed_min_b_z; trimmed_b_z < trimmed_max_b_z; trimmed_b_z++)
        {
            const f32 tz = ((f32)trimmed_b_z + 0.5f) / parent_extent.z;
            const f32 omtz = 1.0f - tz;

            const f32 pz0_x = omtz * p_block_corners_cs[0].x + tz * p_block_corners_cs[4].x;
            const f32 pz0_y = omtz * p_block_corners_cs[0].y + tz * p_block_corners_cs[4].y;
            const f32 pz0_z = omtz * p_block_corners_cs[0].z + tz * p_block_corners_cs[4].z;

            const f32 pz1_x = omtz * p_block_corners_cs[1].x + tz * p_block_corners_cs[5].x;
            const f32 pz1_y = omtz * p_block_corners_cs[1].y + tz * p_block_corners_cs[5].y;
            const f32 pz1_z = omtz * p_block_corners_cs[1].z + tz * p_block_corners_cs[5].z;

            const f32 pz2_x = omtz * p_block_corners_cs[2].x + tz * p_block_corners_cs[6].x;
            const f32 pz2_y = omtz * p_block_corners_cs[2].y + tz * p_block_corners_cs[6].y;
            const f32 pz2_z = omtz * p_block_corners_cs[2].z + tz * p_block_corners_cs[6].z;

            const f32 pz3_x = omtz * p_block_corners_cs[3].x + tz * p_block_corners_cs[7].x;
            const f32 pz3_y = omtz * p_block_corners_cs[3].y + tz * p_block_corners_cs[7].y;
            const f32 pz3_z = omtz * p_block_corners_cs[3].z + tz * p_block_corners_cs[7].z;

            for (u32 trimmed_b_y = trimmed_min_b_y; trimmed_b_y < trimmed_max_b_y; trimmed_b_y++)
            {
                const f32 ty = ((f32)trimmed_b_y + 0.5f) / parent_extent.y;
                const f32 omty = 1.0f - ty;

                const f32 py0_x = omty * pz0_x + ty * pz2_x;
                const f32 py0_y = omty * pz0_y + ty * pz2_y;
                const f32 py0_z = omty * pz0_z + ty * pz2_z;

                const f32 py1_x = omty * pz1_x + ty * pz3_x;
                const f32 py1_y = omty * pz1_y + ty * pz3_y;
                const f32 py1_z = omty * pz1_z + ty * pz3_z;

                for (u32 trimmed_b_x = trimmed_min_b_x; trimmed_b_x < trimmed_max_b_x; trimmed_b_x++)
                {
                    const f32 tx = ((f32)trimmed_b_x + 0.5f) / parent_extent.x;
                    const f32 omtx = 1.0f - tx;

                    const f32 block_voxel_center_x = omtx * py0_x + tx * py1_x;
                    if (block_voxel_center_x < 0.0f || block_voxel_center_x >= TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT)
                        continue;

                    const f32 block_voxel_center_y = omtx * py0_y + tx * py1_y;
                    if (block_voxel_center_y < 0.0f || block_voxel_center_y >= TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT)
                        continue;

                    const f32 block_voxel_center_z = omtx * py0_z + tx * py1_z;
                    if (block_voxel_center_z < 0.0f || block_voxel_center_z >= TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT)
                        continue;

                    const u32 relative_cluster_voxel_x = (u32)block_voxel_center_x;
                    const u32 relative_cluster_voxel_y = (u32)block_voxel_center_y;
                    const u32 relative_cluster_voxel_z = (u32)block_voxel_center_z;
                    
                    const u32 relative_cluster_voxel_idx
                        = TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT * relative_cluster_voxel_z
                        + TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT * relative_cluster_voxel_y
                        + relative_cluster_voxel_x;
                    TG_ASSERT((u64)cluster_offset_in_voxels + (u64)relative_cluster_voxel_idx <= TG_U32_MAX);
                    const u32 cluster_voxel_idx = cluster_offset_in_voxels + relative_cluster_voxel_idx;
                    if ((p_scene->p_voxel_cluster_data[cluster_voxel_idx / 32] & (1 << (cluster_voxel_idx % 32))) != 0)
                    {
                        const u32 block_voxel_idx = (u32)(parent_extent.x * parent_extent.y * trimmed_b_z + parent_extent.x * trimmed_b_y + trimmed_b_x);
                        p_block_voxels[block_voxel_idx / 32] |= 1 << (block_voxel_idx % 32);
                        if (p_data->n == 0 || p_data->p_cluster_idcs[p_data->n - 1] != cluster_idx)
                        {
                            TG_ASSERT(p_data->n < sizeof(p_data->p_cluster_idcs) / sizeof(*p_data->p_cluster_idcs));
                            p_data->p_cluster_idcs[p_data->n++] = cluster_idx;
                        }
                    }
                }
            }
        }
    }
}

static void tg__construct_inner_node(
    tg_svo*                   p_svo,
    tg_svo_inner_node*        p_parent,
    v3                        parent_min,
    v3                        parent_max,
    u32                       n_clusters,
    const u32*                p_cluster_idcs,
    const tg_scene*           p_scene)
{
    TG_ASSERT((u32)(parent_max.x - parent_min.x) * (u32)(parent_max.y - parent_min.y) * (u32)(parent_max.z - parent_min.z) > TG_SVO_BLOCK_VOXEL_COUNT); // Otherwise, this should be a leaf!
    TG_ASSERT(parent_max.x - parent_min.x > 8.0f);
    TG_ASSERT(parent_max.y - parent_min.y > 8.0f);
    TG_ASSERT(parent_max.z - parent_min.z > 8.0f);

    u8 valid_mask = 0;

    u32 p_n_clusters_per_child[8] = { 0 };

    u32* p_cluster_idcs_per_child = TG_NULL;
    const tg_size cluster_idcs_per_child_size = 8 * (tg_size)n_clusters * sizeof(*p_cluster_idcs_per_child);
    p_cluster_idcs_per_child = TG_MALLOC_STACK(cluster_idcs_per_child_size);
#ifdef TG_DEBUG
    tg_memory_nullify(cluster_idcs_per_child_size, p_cluster_idcs_per_child);
#endif

    const v3 child_extent = tgm_v3_mulf(tgm_v3_sub(parent_max, parent_min), 0.5f);

    for (u32 cluster_idcs_idx = 0; cluster_idcs_idx < n_clusters; cluster_idcs_idx++)
    {
        const u32 cluster_idx = p_cluster_idcs[cluster_idcs_idx];
        const u32 object_idx = p_scene->p_cluster_idx_to_object_idx[cluster_idx];
        const tg_voxel_object* p_object = &p_scene->p_objects[object_idx];
        const u32 relative_cluster_idx = cluster_idx - p_object->first_cluster_idx;

        const m4 ws2ms = tg__world_space_to_model_space(p_object, relative_cluster_idx);

        for (u32 child_idx = 0; child_idx < 8; child_idx++)
        {
            // TODO: this may be optimized with & and >>
            const f32 d_x = (f32)( child_idx      % 2) * child_extent.x;
            const f32 d_y = (f32)((child_idx / 2) % 2) * child_extent.y;
            const f32 d_z = (f32)((child_idx / 4) % 2) * child_extent.z;
            const v3 d_v3 = { d_x, d_y, d_z };

            const v3 child_min = tgm_v3_add(parent_min, d_v3);
            const v3 child_max = tgm_v3_add(child_min, child_extent);

            v4 p_block_corners_ws[8] = { 0 };
            p_block_corners_ws[0] = (v4){ child_min.x, child_min.y, child_min.z, 1.0f };
            p_block_corners_ws[1] = (v4){ child_max.x, child_min.y, child_min.z, 1.0f };
            p_block_corners_ws[2] = (v4){ child_min.x, child_max.y, child_min.z, 1.0f };
            p_block_corners_ws[3] = (v4){ child_max.x, child_max.y, child_min.z, 1.0f };
            p_block_corners_ws[4] = (v4){ child_min.x, child_min.y, child_max.z, 1.0f };
            p_block_corners_ws[5] = (v4){ child_max.x, child_min.y, child_max.z, 1.0f };
            p_block_corners_ws[6] = (v4){ child_min.x, child_max.y, child_max.z, 1.0f };
            p_block_corners_ws[7] = (v4){ child_max.x, child_max.y, child_max.z, 1.0f };

            v3 p_block_corners[8] = { 0 };
            p_block_corners[0] = tgm_m4_mulv4(ws2ms, p_block_corners_ws[0]).xyz;
            p_block_corners[1] = tgm_m4_mulv4(ws2ms, p_block_corners_ws[1]).xyz;
            p_block_corners[2] = tgm_m4_mulv4(ws2ms, p_block_corners_ws[2]).xyz;
            p_block_corners[3] = tgm_m4_mulv4(ws2ms, p_block_corners_ws[3]).xyz;
            p_block_corners[4] = tgm_m4_mulv4(ws2ms, p_block_corners_ws[4]).xyz;
            p_block_corners[5] = tgm_m4_mulv4(ws2ms, p_block_corners_ws[5]).xyz;
            p_block_corners[6] = tgm_m4_mulv4(ws2ms, p_block_corners_ws[6]).xyz;
            p_block_corners[7] = tgm_m4_mulv4(ws2ms, p_block_corners_ws[7]).xyz;

            if (tg_intersect_aabb_obb_ignore_contact(TG_CLUSTER_MIN, TG_CLUSTER_MAX, p_block_corners))
            {
                valid_mask |= 1 << child_idx;
                if (p_n_clusters_per_child[child_idx] == 0
                    || p_cluster_idcs_per_child[child_idx * n_clusters + p_n_clusters_per_child[child_idx] - 1] != cluster_idx)
                {
                    p_cluster_idcs_per_child[child_idx * n_clusters + p_n_clusters_per_child[child_idx]++] = cluster_idx;
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
                        p_n_clusters_per_child[child_idx],
                        &p_cluster_idcs_per_child[child_idx * n_clusters],
                        p_scene);
                }
                else
                {
                    p_parent->leaf_mask |= 1 << child_idx;
                    tg__construct_leaf_node(
                        p_svo,
                        (tg_svo_leaf_node*)&p_child_nodes[child_node_offset],
                        child_min,
                        child_max,
                        p_n_clusters_per_child[child_idx],
                        &p_cluster_idcs_per_child[child_idx * n_clusters],
                        p_scene);
                }
        
                child_node_offset++;
            }
        }
    }

    TG_FREE_STACK(cluster_idcs_per_child_size);
}

void tg_svo_create(v3 extent_min, v3 extent_max, const tg_scene* p_scene, TG_OUT tg_svo* p_svo)
{
    TG_ASSERT(extent_max.x - extent_min.x == (f32)TG_SVO_SIDE_LENGTH);
    TG_ASSERT(extent_max.y - extent_min.y == (f32)TG_SVO_SIDE_LENGTH);
    TG_ASSERT(extent_max.z - extent_min.z == (f32)TG_SVO_SIDE_LENGTH);
    TG_ASSERT(p_scene);
    TG_ASSERT(p_scene->n_clusters > 0);
    TG_ASSERT(p_svo);

    // TODO: chunked svo construction for infinite worlds
    p_svo->min = extent_min;
    p_svo->max = extent_max;
    
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

    // TODO: We can already filter the clusters here (?)! Some are not inside of the initial SVO!
    u32 n_clusters = 0;
    u32* p_cluster_idcs = TG_MALLOC_STACK(p_scene->n_clusters * sizeof(*p_cluster_idcs));
    for (u32 object_idx = 0; object_idx < p_scene->n_objects; object_idx++)
    {
        const tg_voxel_object* p_object = &p_scene->p_objects[object_idx];
        const u32 n_clusters_of_object
            = p_object->n_clusters_per_dim.x
            * p_object->n_clusters_per_dim.y
            * p_object->n_clusters_per_dim.z;
        for (u32 relative_cluster_idx = 0; relative_cluster_idx < n_clusters_of_object; relative_cluster_idx++)
        {
            const u32 cluster_idx = p_object->first_cluster_idx + relative_cluster_idx;



            // Filter empty clusters
            // TODO: If we ever remove empty clusters from the scene, the following section will not be required anymore!
            const tg_size cluster_offset = (tg_size)cluster_idx * TG_CLUSTER_SIZE(1);
            const u32* p_cluster = (u32*)&((u8*)p_scene->p_voxel_cluster_data)[cluster_offset];
            b32 contains_voxels = TG_FALSE;
            for (u32 relative_voxel_idx = 0; relative_voxel_idx < TG_N_PRIMITIVES_PER_CLUSTER; relative_voxel_idx++)
            {
                if (p_cluster[relative_voxel_idx / 32] & (1 << (relative_voxel_idx % 32)))
                {
                    contains_voxels = TG_TRUE;
                    break;
                }
            }
            if (!contains_voxels)
            {
                continue;
            }



            p_cluster_idcs[n_clusters++] = cluster_idx;
        }
    }

    tg_svo_inner_node* p_parent = (tg_svo_inner_node*)p_svo->p_node_buffer;
    tg__construct_inner_node(p_svo, p_parent, extent_min, extent_max, n_clusters, p_cluster_idcs, p_scene);
    TG_FREE_STACK(p_scene->n_clusters * sizeof(*p_cluster_idcs));
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
    f32 distance_ray_origin_2_position = 0.0f;
    if (tg_intersect_ray_aabb(ray_origin, ray_direction, p_svo->min, p_svo->max, &enter, &exit))
    {
        const v3 v3_min = { TG_F32_MIN, TG_F32_MIN, TG_F32_MIN };
        const v3 v3_max = { TG_F32_MAX, TG_F32_MAX, TG_F32_MAX };

        v3 position = enter > 0.0f ? tgm_v3_add(ray_origin, tgm_v3_mulf(ray_direction, enter)) : ray_origin;
        distance_ray_origin_2_position = enter > 0.0f ? enter : 0.0f;

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
                    if (p_data->n != 0)
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
                            *p_distance = distance_ray_origin_2_position + enter;
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
                const f32 t_advance = exit + TG_F32_EPSILON; // TODO: epsilon required?
                const v3 advance = tgm_v3_mulf(ray_direction, t_advance);
                position = tgm_v3_add(position, advance);
                distance_ray_origin_2_position += t_advance;

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
