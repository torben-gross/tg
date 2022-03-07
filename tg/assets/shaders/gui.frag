#version 450

#include "shaders/common.inc"
#include "shaders/util/color.inc"

TG_IN(     0, v2     v_uv);
TG_IN_FLAT(1, u32    v_type);

TG_SAMPLER2D(1, tex);
TG_SSBO(     2, u32 colors[];);

TG_OUT(0, v4 out_color);

void main()
{
	f32 value = texture(tex, v_uv).x;
	v4 color = tg_color_unpack(colors[v_type]);
	out_color = value * color;
}
