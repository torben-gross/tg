#version 450

#include "shaders/common.inc"

layout(location = 0) in v3    v_position;
layout(location = 1) in v3    v_normal;

#include "shaders/fs_out.inc"

void main()
{
    v3 p = v_position.xyz;
    v3 n = normalize(v_normal);
    f32 d = dot(n.xyz, vec3(0.0, 1.0, 0.0));
    
    v3 color_grass = vec3(160.0/255.0, 150.0/255.0, 40.0/255.0);
    v3 color_stone = vec3(247.0/255.0, 226.0/255.0, 164.0/255.0);

    f32 t = d < 0.5 ? 0.0 : 1.0;
    v3 a = mix(color_stone, color_grass, t);

    f32 m = 0.0;

    f32 roughness_grass = 1.0;
    f32 roughness_stone = 0.5;
    f32 r = mix(roughness_stone, roughness_grass, t);

    tg_out(n, a, m, r);
}
