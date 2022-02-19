#version 450

#include "shaders/common.inc"

struct tg_svo
{
    v4    min;
    v4    max;
};

TG_IN(0, u32 in_instance_id); #instance
TG_IN(1, v2  in_position);

TG_UBO(0,
    m4    v_mat;
    m4    p_mat;
);

TG_OUT_FLAT(0, u32 v_instance_id);

void main()
{
    v_instance_id    = in_instance_id;
    gl_Position = v4(in_position, 0.0, 1.0);
}
