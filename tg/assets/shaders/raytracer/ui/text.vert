#version 450

#include "shaders/common.inc"

struct tg_char_ssbo_entry
{
    f32    translation_x;
    f32    translation_y;
    f32    half_scale_x;
    f32    half_scale_y;
    f32    uv_center_x;
    f32    uv_center_y;
    f32    uv_half_scale_x;
    f32    uv_half_scale_y;
};

TG_IN(0, u32    in_instance_id); #instance
TG_IN(1, v2     in_position);

TG_SSBO(0, tg_char_ssbo_entry    char_ssbo[];);

TG_OUT(0, v2 v_uv);

void main()
{
	tg_char_ssbo_entry e = char_ssbo[in_instance_id];
	v2 half_scale = v2(e.half_scale_x, e.half_scale_y);
	v2 translation = v2(e.translation_x, e.translation_y);
	
	gl_Position = v4(in_position * half_scale + translation, 0.0, 1.0);
	
	v2 uv_center = v2(e.uv_center_x, e.uv_center_y);
	v2 uv_half_scale = v2(e.uv_half_scale_x, e.uv_half_scale_y);
	
	v_uv = uv_center + uv_half_scale * in_position + v2(0.0, 0.0);
}
