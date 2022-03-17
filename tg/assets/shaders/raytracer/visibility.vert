#version 450

#include "shaders/common.inc"
#include "shaders/raytracer/buffers.inc"
#include "shaders/raytracer/cluster.inc"

TG_IN(0, u32    in_cluster_pointer); #instance
TG_IN(1, v3     in_position);

TG_SSBO(0, u32                                        cluster_pointer_ssbo[];          );
TG_SSBO(1, tg_cluster_idx_to_object_idx_ssbo_entry    cluster_idx_to_object_idx_ssbo[];);
TG_SSBO(2, tg_object_data_ssbo_entry                  object_data_ssbo[];              );
TG_UBO( 5, tg_view_projection_ubo                     view_projection_ssbo;            );

TG_OUT_FLAT(0, u32    v_cluster_pointer);

#include "shaders/raytracer/cluster_functions.inc"

void main()
{
	v_cluster_pointer = in_cluster_pointer;
	u32 cluster_idx = cluster_pointer_ssbo[in_cluster_pointer];
	u32 object_idx = cluster_idx_to_object_idx_ssbo[cluster_idx].object_idx;
	m4 m_mat = tg_cluster_model_matrix(object_idx, in_cluster_pointer);
    gl_Position = view_projection_ssbo.p * view_projection_ssbo.v * m_mat * v4(in_position, 1.0);
}
