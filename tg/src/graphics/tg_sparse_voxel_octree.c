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



static v4 p_cube_corners[8] = {
    { -0.5f, -0.5f, -0.5f,  1.0f },
    {  0.5f, -0.5f, -0.5f,  1.0f },
    { -0.5f,  0.5f, -0.5f,  1.0f },
    {  0.5f,  0.5f, -0.5f,  1.0f },
    { -0.5f, -0.5f,  0.5f,  1.0f },
    {  0.5f, -0.5f,  0.5f,  1.0f },
    { -0.5f,  0.5f,  0.5f,  1.0f },
    {  0.5f,  0.5f,  0.5f,  1.0f }
};



static m4 tg__cluster_model_matrix(const tg_voxel_object* p_object, u32 relative_cluster_idx)
{
    const u32 relative_cluster_idx_x = relative_cluster_idx % p_object->n_clusters_per_dim.x;
    const u32 relative_cluster_idx_y = (relative_cluster_idx / p_object->n_clusters_per_dim.x) % p_object->n_clusters_per_dim.y;
    const u32 relative_cluster_idx_z = relative_cluster_idx / (p_object->n_clusters_per_dim.x * p_object->n_clusters_per_dim.y); // Note: We don't need modulo here, because z doesn't loop

    const f32 relative_cluster_offset_x = (f32)(relative_cluster_idx_x * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
    const f32 relative_cluster_offset_y = (f32)(relative_cluster_idx_y * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
    const f32 relative_cluster_offset_z = (f32)(relative_cluster_idx_z * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
    const v3 relative_cluster_offset = { relative_cluster_offset_x, relative_cluster_offset_y, relative_cluster_offset_z };

    const f32 n_prims_per_cluster_cube_root_half_f = (f32)(TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT / 2);
    const v3 cluster_half_extent = { n_prims_per_cluster_cube_root_half_f, n_prims_per_cluster_cube_root_half_f, n_prims_per_cluster_cube_root_half_f };
    const v3 n_clusters_per_dim = { (f32)p_object->n_clusters_per_dim.x, (f32)p_object->n_clusters_per_dim.y, (f32)p_object->n_clusters_per_dim.z };
    const v3 object_half_extent = tgm_v3_mul(n_clusters_per_dim, cluster_half_extent);

    const v3 relative_cluster_translation = tgm_v3_add(tgm_v3_add(tgm_v3_neg(object_half_extent), cluster_half_extent), relative_cluster_offset);

    // Transformation order:
    //   1. Scale the cluster to its final size
    //   2. Translate the cluster relative inside of the object
    //   3. Rotate the cluster
    //   4. Translate the object, such that the relatively moved cluster is in the world location of the object
    // (1) and (2) are composed in m0. (3) and (4) are composed in m1.

    m4 m_mat0 = tgm_m4_scale1((f32)TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
    m_mat0.col3.xyz = relative_cluster_translation;
    
    m4 m_mat1 = tgm_m4_angle_axis(p_object->angle_in_radians, p_object->axis);
    m_mat1.col3.xyz = p_object->translation;
    
    const m4 m_mat = tgm_m4_mul(m_mat1, m_mat0);
    return m_mat;
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
    // TODO: The current method only checks, whether the voxel corresponding to the
    // blocks voxels position is set. This is incorrect. We need to check all
    // surrounding voxels and determine, whether the overlap/tough the current
    // voxel and only then set the voxel in the block. The current approximation
    // may be good enough, but if not, this needs to be added!

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

    for (u32 cluster_idcs_idx = 0; cluster_idcs_idx < n_clusters; cluster_idcs_idx++)
    {
        const u32 cluster_idx = p_cluster_idcs[cluster_idcs_idx];
        
#ifdef TG_DEBUG
        // TODO: If we ever remove empty clusters, this will not be required anymore!
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
#endif

        const u32 object_idx = p_scene->p_cluster_idx_to_object_idx[cluster_idx];
        const tg_voxel_object* p_object = &p_scene->p_objects[object_idx];
        const u32 relative_cluster_idx = cluster_idx - p_object->first_cluster_idx;

        const m4 m_mat = tg__cluster_model_matrix(p_object, relative_cluster_idx);

        v3 p_cluster_corners[8] = { 0 };
        p_cluster_corners[0] = tgm_m4_mulv4(m_mat, p_cube_corners[0]).xyz;
        p_cluster_corners[1] = tgm_m4_mulv4(m_mat, p_cube_corners[1]).xyz;
        p_cluster_corners[2] = tgm_m4_mulv4(m_mat, p_cube_corners[2]).xyz;
        p_cluster_corners[3] = tgm_m4_mulv4(m_mat, p_cube_corners[3]).xyz;
        p_cluster_corners[4] = tgm_m4_mulv4(m_mat, p_cube_corners[4]).xyz;
        p_cluster_corners[5] = tgm_m4_mulv4(m_mat, p_cube_corners[5]).xyz;
        p_cluster_corners[6] = tgm_m4_mulv4(m_mat, p_cube_corners[6]).xyz;
        p_cluster_corners[7] = tgm_m4_mulv4(m_mat, p_cube_corners[7]).xyz;

        TG_ASSERT(tg_intersect_aabb_obb_ignore_contact(parent_min, parent_max, p_cluster_corners));

        TG_ASSERT((u64)cluster_idx * (u64)TG_N_PRIMITIVES_PER_CLUSTER <= TG_U32_MAX);
        const u32 cluster_offset_in_voxels = cluster_idx * TG_N_PRIMITIVES_PER_CLUSTER;

        for (u32 cluster_voxel_z = 0; cluster_voxel_z < TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT; cluster_voxel_z++)
        {
            const f32 tz = ((f32)cluster_voxel_z + 0.5f) / (f32)TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT;
            const f32 omtz = 1.0f - tz;

            const f32 pz0_x = omtz * p_cluster_corners[0].x + tz * p_cluster_corners[4].x;
            const f32 pz0_y = omtz * p_cluster_corners[0].y + tz * p_cluster_corners[4].y;
            const f32 pz0_z = omtz * p_cluster_corners[0].z + tz * p_cluster_corners[4].z;

            const f32 pz1_x = omtz * p_cluster_corners[1].x + tz * p_cluster_corners[5].x;
            const f32 pz1_y = omtz * p_cluster_corners[1].y + tz * p_cluster_corners[5].y;
            const f32 pz1_z = omtz * p_cluster_corners[1].z + tz * p_cluster_corners[5].z;

            const f32 pz2_x = omtz * p_cluster_corners[2].x + tz * p_cluster_corners[6].x;
            const f32 pz2_y = omtz * p_cluster_corners[2].y + tz * p_cluster_corners[6].y;
            const f32 pz2_z = omtz * p_cluster_corners[2].z + tz * p_cluster_corners[6].z;

            const f32 pz3_x = omtz * p_cluster_corners[3].x + tz * p_cluster_corners[7].x;
            const f32 pz3_y = omtz * p_cluster_corners[3].y + tz * p_cluster_corners[7].y;
            const f32 pz3_z = omtz * p_cluster_corners[3].z + tz * p_cluster_corners[7].z;

            for (u32 cluster_voxel_y = 0; cluster_voxel_y < TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT; cluster_voxel_y++)
            {
                const f32 ty = ((f32)cluster_voxel_y + 0.5f) / (f32)TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT;
                const f32 omty = 1.0f - ty;

                const f32 py0_x = omty * pz0_x + ty * pz2_x;
                const f32 py0_y = omty * pz0_y + ty * pz2_y;
                const f32 py0_z = omty * pz0_z + ty * pz2_z;

                const f32 py1_x = omty * pz1_x + ty * pz3_x;
                const f32 py1_y = omty * pz1_y + ty * pz3_y;
                const f32 py1_z = omty * pz1_z + ty * pz3_z;

                for (u32 cluster_voxel_x = 0; cluster_voxel_x < TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT; cluster_voxel_x++)
                {
                    const u32 relative_voxel_idx
                        = TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT * cluster_voxel_z
                        + TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT * cluster_voxel_y
                        + cluster_voxel_x;
                    TG_ASSERT((u64)cluster_offset_in_voxels + (u64)relative_voxel_idx <= TG_U32_MAX);
                    const u32 voxel_idx = cluster_offset_in_voxels + relative_voxel_idx;
                    const u32 voxel = p_scene->p_voxel_cluster_data[voxel_idx / 32] & (1 << (voxel_idx % 32));
                    if (voxel == 0)
                    {
                        continue;
                    }

                    const f32 tx = ((f32)cluster_voxel_x + 0.5f) / (f32)TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT;
                    const f32 omtx = 1.0f - tx;

                    const f32 cluster_voxel_center_x = omtx * py0_x + tx * py1_x;
                    const f32 cluster_voxel_center_y = omtx * py0_y + tx * py1_y;
                    const f32 cluster_voxel_center_z = omtx * py0_z + tx * py1_z;

                    const f32 cvc2bv_x = tgm_f32_floor(cluster_voxel_center_x);
                    const f32 cvc2bv_y = tgm_f32_floor(cluster_voxel_center_y);
                    const f32 cvc2bv_z = tgm_f32_floor(cluster_voxel_center_z);

                    const b32 in_x = cvc2bv_x >= parent_min.x && cvc2bv_x < parent_max.x;
                    const b32 in_y = cvc2bv_y >= parent_min.y && cvc2bv_y < parent_max.y;
                    const b32 in_z = cvc2bv_z >= parent_min.z && cvc2bv_z < parent_max.z;

                    if (in_x && in_y && in_z)
                    {
                        TG_ASSERT(cvc2bv_x - parent_min.x >= 0.0f);
                        TG_ASSERT(cvc2bv_y - parent_min.y >= 0.0f);
                        TG_ASSERT(cvc2bv_z - parent_min.z >= 0.0f);
                        TG_ASSERT(cvc2bv_x - parent_min.x < parent_extent.x);
                        TG_ASSERT(cvc2bv_y - parent_min.y < parent_extent.y);
                        TG_ASSERT(cvc2bv_z - parent_min.z < parent_extent.z);

                        const u32 block_voxel_x = (u32)(cvc2bv_x - parent_min.x);
                        const u32 block_voxel_y = (u32)(cvc2bv_y - parent_min.y);
                        const u32 block_voxel_z = (u32)(cvc2bv_z - parent_min.z);

                        const u32 block_voxel_idx = (u32)(parent_extent.x * parent_extent.y * block_voxel_z + parent_extent.x * block_voxel_y + block_voxel_x);
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
    tg_svo* p_svo,
    tg_svo_inner_node* p_parent,
    v3                        parent_min,
    v3                        parent_max,
    u32                       n_clusters,
    const u32* p_cluster_idcs,
    const tg_scene* p_scene)
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

        const m4 m_mat = tg__cluster_model_matrix(p_object, relative_cluster_idx);

        v3 p_cluster_corners[8] = { 0 };
        p_cluster_corners[0] = tgm_m4_mulv4(m_mat, p_cube_corners[0]).xyz;
        p_cluster_corners[1] = tgm_m4_mulv4(m_mat, p_cube_corners[1]).xyz;
        p_cluster_corners[2] = tgm_m4_mulv4(m_mat, p_cube_corners[2]).xyz;
        p_cluster_corners[3] = tgm_m4_mulv4(m_mat, p_cube_corners[3]).xyz;
        p_cluster_corners[4] = tgm_m4_mulv4(m_mat, p_cube_corners[4]).xyz;
        p_cluster_corners[5] = tgm_m4_mulv4(m_mat, p_cube_corners[5]).xyz;
        p_cluster_corners[6] = tgm_m4_mulv4(m_mat, p_cube_corners[6]).xyz;
        p_cluster_corners[7] = tgm_m4_mulv4(m_mat, p_cube_corners[7]).xyz;

        for (u32 child_idx = 0; child_idx < 8; child_idx++)
        {
            // TODO: this may be optimized with & and >>
            const f32 d_x = (f32)( child_idx      % 2) * child_extent.x;
            const f32 d_y = (f32)((child_idx / 2) % 2) * child_extent.y;
            const f32 d_z = (f32)((child_idx / 4) % 2) * child_extent.z;
            const v3 d_v3 = { d_x, d_y, d_z };

            const v3 child_min = tgm_v3_add(parent_min, d_v3);
            const v3 child_max = tgm_v3_add(child_min, child_extent);

            if (tg_intersect_aabb_obb_ignore_contact(child_min, child_max, p_cluster_corners))
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
