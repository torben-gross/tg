#version 450

#include "shaders/common.inc"

TG_IN(0, v2 in_position);
TG_IN(1, v2 in_uv);

TG_OUT(0, v2 v_uv);

void main()
{
    gl_Position = v4(in_position, 0.0, 1.0);
    v_uv = in_uv;
}
