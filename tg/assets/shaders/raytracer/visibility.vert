#version 450

#include "shaders/common.inc"
#include "shaders/raytracer/buffers.inc"
#include "shaders/raytracer/cluster.inc"

TG_IN(0, u32    in_cluster_idx); #instance
TG_IN(1, v3     in_position);

TG_SSBO(0, tg_cluster_idx_to_object_idx_ssbo_entry    cluster_idx_to_object_idx_ssbo[];);
TG_SSBO(1, tg_object_data_ssbo_entry                  object_data_ssbo[];              );
TG_UBO( 4, tg_view_projection_ubo                     view_projection_ssbo;            );

TG_OUT_FLAT(0, u32    v_cluster_idx);

m4 tg_cluster_model_matrix(u32 object_idx, u32 cluster_idx)
{
	tg_object_data_ssbo_entry object_data = object_data_ssbo[object_idx];
	
	u32 relative_cluster_idx = cluster_idx - object_data.first_cluster_idx;
	u32 relative_cluster_idx_x = relative_cluster_idx % object_data.n_clusters_per_dim.x;
	u32 relative_cluster_idx_y = (relative_cluster_idx / object_data.n_clusters_per_dim.x) % object_data.n_clusters_per_dim.y;
	u32 relative_cluster_idx_z = relative_cluster_idx / (object_data.n_clusters_per_dim.x * object_data.n_clusters_per_dim.y); // Note: We don't need modulo here, because z doesn't loop
	
	f32 relative_cluster_offset_x = f32(relative_cluster_idx_x * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
	f32 relative_cluster_offset_y = f32(relative_cluster_idx_y * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
	f32 relative_cluster_offset_z = f32(relative_cluster_idx_z * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
	v3 relative_cluster_offset = v3(relative_cluster_offset_x, relative_cluster_offset_y, relative_cluster_offset_z);
	
	v3 cluster_half_extent = v3(f32(TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT / 2));
	v3 n_clusters_per_dim = v3(f32(object_data.n_clusters_per_dim.x), f32(object_data.n_clusters_per_dim.y), f32(object_data.n_clusters_per_dim.z));
	v3 object_half_extent = n_clusters_per_dim * cluster_half_extent;
	
	v3 relative_cluster_translation = -object_half_extent + cluster_half_extent + relative_cluster_offset;
	
	// Transformation order:
	//   1. Scale the cluster to its final size
	//   2. Translate the cluster relative inside of the object
	//   3. Rotate the cluster
	//   4. Translate the object, such that the relatively moved cluster is in the world location of the object
	// (1) and (2) are composed in m0. (3) and (4) are composed in m1.
	
	m4 m0 = m4(
        v4(f32(TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT), 0.0, 0.0, 0.0), // col0
        v4(0.0, f32(TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT), 0.0, 0.0), // col1
        v4(0.0, 0.0, f32(TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT), 0.0), // col2
        v4(relative_cluster_translation, 1.0)                          // col3
	);
	m4 m1 = m4(object_data.rotation[0], object_data.rotation[1], object_data.rotation[2], v4(object_data.translation, 1.0));
	m4 m_mat = m1 * m0;
	
	return m_mat;
}

void main()
{
    v_cluster_idx = in_cluster_idx;
	u32 object_idx = cluster_idx_to_object_idx_ssbo[in_cluster_idx].object_idx;
	m4 m_mat = tg_cluster_model_matrix(object_idx, in_cluster_idx);
    gl_Position = view_projection_ssbo.p * view_projection_ssbo.v * m_mat * v4(in_position, 1.0);
}
