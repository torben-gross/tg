#version 450

#include "shaders/common.inc"
#include "shaders/raytracer/buffers.inc"

TG_IN(0, u32    in_instance_id); #instance
TG_IN(1, v3     in_position);

TG_SSBO(0, m4                        transformation_matrices[];);
TG_UBO( 1, tg_view_projection_ubo    view_projection_ubo;      );

TG_OUT_FLAT(0, u32 v_instance_id);

void main()
{
    v_instance_id = in_instance_id;
    gl_Position = view_projection_ubo.p * view_projection_ubo.v * transformation_matrices[in_instance_id] * v4(in_position, 1.0);
}
