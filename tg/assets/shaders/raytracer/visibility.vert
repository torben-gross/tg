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

void main()
{
    v_cluster_idx = in_cluster_idx;
	
	u32 object_idx = cluster_idx_to_object_idx_ssbo[in_cluster_idx].object_idx;
	tg_object_data_ssbo_entry object_data = object_data_ssbo[object_idx];

    m4 s_mat = m4(
        v4(f32(object_data.n_clusters_per_dim.x * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT), 0.0, 0.0, 0.0), // col0
        v4(0.0, f32(object_data.n_clusters_per_dim.y * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT), 0.0, 0.0), // col1
        v4(0.0, 0.0, f32(object_data.n_clusters_per_dim.z * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT), 0.0), // col2
        v4(0.0, 0.0, 0.0, 1.0)                                                                            // col3
    );
    m4 r_mat = object_data.rotation;
    m4 t_mat = m4(
        v4(1.0, 0.0, 0.0, 0.0),                               // col0
        v4(0.0, 1.0, 0.0, 0.0),                               // col1
        v4(0.0, 0.0, 1.0, 0.0),                               // col2
        v4(object_data.translation, 1.0)); // col3
    // TODO: can we just set the translation col in the end? saves some multiplications
    m4 m_mat = t_mat * r_mat * s_mat;

    gl_Position = view_projection_ssbo.p * view_projection_ssbo.v * m_mat * v4(in_position, 1.0);
}
