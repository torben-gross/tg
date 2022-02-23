#version 450

#include "shaders/common.inc"

TG_IN(0, u32 in_instance_id); #instance
TG_IN(1, v2  in_position);

TG_OUT_FLAT(0, u32 v_instance_id);

void main()
{
    v_instance_id = in_instance_id;
    gl_Position   = v4(in_position, 0.0, 1.0);
}
