#version 450

#include "shaders/common.inc"

layout(location = 0) in v2 in_position;
layout(location = 1) in v2 in_uv;

layout(location = 0) out v2 v_uv;

void main()
{
    gl_Position = v4(in_position, 0.0, 1.0);
    v_uv = in_uv;
}
