#version 450

#include "shaders/common.inc"

TG_IN(0, v2 v_uv);

layout(set = 0, binding = 1) uniform sampler2D texture_atlas;

TG_OUT(0, v4 out_color);

void main()
{
	f32 value = texture(texture_atlas, v_uv).x;
	out_color = value == 0 ? v4(0.2, 0.1, 0.5, 0.4) : v4(v3(0.4, 0.2, 1.0), 0.8);
}
