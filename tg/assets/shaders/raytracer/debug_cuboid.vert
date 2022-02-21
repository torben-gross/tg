#version 450

#include "shaders/common.inc"

TG_IN(0, u32    in_instance_id); #instance
TG_IN(1, v3     in_position);

TG_SSBO(0,
    m4    transformation_matrices[];
);

TG_UBO(1,
    m4    v_mat;
    m4    p_mat;
);

TG_OUT_FLAT(0, u32 v_instance_id);

void main()
{
    v_instance_id = in_instance_id;
    gl_Position = p_mat * v_mat * transformation_matrices[in_instance_id] * v4(in_position, 1.0);
}
