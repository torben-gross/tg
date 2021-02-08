#version 450

#include "shaders/common.inc"

layout(location = 0) in v3    v_position;
layout(location = 1) in v3    v_normal;
layout(location = 2) in v2    v_uv;
//layout(location = 3) in m3    v_tbn; // TODO: this will be needed for normal mapping

layout(set = 0, binding = 2) uniform material
{
    v4     albedo;
    f32    metallic;
    f32    roughness;
};

#include "shaders/fs_out.inc"

void main()
{
    v3 n = normalize(v_normal);
    v3 a = albedo.xyz;
    f32 m = metallic;
    f32 r = roughness;
    tg_out(n, a, m, r);
}
