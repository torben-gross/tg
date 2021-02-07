#version 450

#include "shaders/common.inc"

layout(location = 0) in vec3    v_position;
layout(location = 1) in vec3    v_normal;

layout(location = 0) out v4u    out_position_3xu32_normal_3xu8_metallic_1xu8;
layout(location = 1) out v4     out_albedo_3xf8_roughness_1xf8;

void tg_pack(v3 p, v3 n, v3 a, f32 m, f32 r)
{
    v4u v0;
    v0.xyz = floatBitsToUint(p);
    v0.w = uint(n.x * 127.5 + 127.5) | (uint(n.y * 127.5 + 127.5) << 8) | (uint(n.z * 127.5 + 127.5) << 16) | (uint(m * 255.0) << 24);

    v4 v1 = v4(a, r);

    out_position_3xu32_normal_3xu8_metallic_1xu8 = v0;
    out_albedo_3xf8_roughness_1xf8 = v1;
}

void main()
{
    vec3 p = v_position.xyz;
    vec3 n = normalize(v_normal);
    float d = dot(n.xyz, vec3(0.0, 1.0, 0.0));
    
    vec3 color_grass = vec3(160.0/255.0, 150.0/255.0, 40.0/255.0);
    vec3 color_stone = vec3(247.0/255.0, 226.0/255.0, 164.0/255.0);

    float t = d < 0.5 ? 0.0 : 1.0;
    vec3 a = mix(color_stone, color_grass, t);

    float m = 0.0;

    float roughness_grass = 1.0;
    float roughness_stone = 0.5;
    float r = mix(roughness_stone, roughness_grass, t);

    tg_pack(p, n, a, m, r);
}
