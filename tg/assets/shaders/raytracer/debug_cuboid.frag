#version 450

#include "shaders/common.inc"

TG_IN_FLAT(0, u32 v_instance_id);

TG_SSBO(2,
	v4    colors[];
);

TG_OUT(0, v4 out_color);

void main()
{
	out_color = colors[v_instance_id];
}
