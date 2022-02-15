#version 450

#include "shaders/common.inc"

struct tg_instance_data
{
    m4     s_mat;
    m4     r_mat;
    m4     t_mat;
    u32    grid_w;
    u32    grid_h;
    u32    grid_d;
    u32    first_voxel_id;
};

layout(location = 0) in u32    in_instance_id; #instance
layout(location = 1) in v3     in_position;

layout(set = 0, binding = 0) buffer tg_instance_data_ssbo
{
    tg_instance_data    instance_data[];
};

layout(set = 0, binding = 1) uniform tg_view_projection_ubo
{
    m4    v_mat;
    m4    p_mat;
};

void main()
{
    tg_instance_data i = instance_data[in_instance_id];
    m4 m_mat = i.t_mat * i.r_mat * i.s_mat;
    gl_Position = p_mat * v_mat * m_mat * v4(in_position, 1.0);
}