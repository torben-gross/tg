#version 450

#include "shaders/common.inc"

layout(location = 0) in u32    in_instance_id; #instance
layout(location = 1) in v3     in_position;

layout(set = 0, binding = 0) buffer tg_instance_data_ssbo
{
    m4    transformation_matrices[];
};

layout(set = 0, binding = 1) uniform tg_view_projection_ubo
{
    m4    v_mat;
    m4    p_mat;
};

void main()
{
    gl_Position = p_mat * v_mat * transformation_matrices[in_instance_id] * v4(in_position, 1.0);
}
