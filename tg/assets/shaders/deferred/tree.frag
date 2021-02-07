#version 450

#include "shaders/common.inc"

layout(location = 0) in vec3    v_position;
layout(location = 1) in vec3    v_normal;
layout(location = 2) in vec2    v_uv;
//layout(location = 3) in mat3    v_tbn; // TODO: this will be needed for normal mapping

layout(set = 0, binding = 2) uniform sampler2D tex;

#include "shaders/fs_out.inc"

void main()
{
    vec3 p = v_position;
    vec3 n = normalize(v_normal);
    vec3 a = texture(tex, v_uv).xyz;
    float m = 0.0f;
    float r = 1.0f;
    tg_out(p, n, a, m, r);
}
