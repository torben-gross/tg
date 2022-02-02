#version 450

#include "shaders/common.inc"

layout(location = 0) in u32    in_instance_id; #instance
layout(location = 1) in v3     in_position;
layout(location = 2) in v3     in_normal;

layout(set = 0, binding = 0) uniform unique_ubo
{
    m4     u_translation;
    m4     u_rotation;
    m4     u_scale;
    u32    u_first_voxel_id;
};

layout(set = 0, binding = 1) uniform view_projection_ubo
{
    m4    u_view;
    m4    u_projection;
};

layout(location = 0) flat out uint    v_instance_id;
layout(location = 1)      out v3      v_position;

void main()
{
    v_instance_id    = in_instance_id;
    m4 model         = u_translation * u_rotation * u_scale;
    gl_Position      = u_projection * u_view * model * v4(in_position, 1.0);
	v_position       = (model * v4(in_position, 1.0)).xyz;
}
