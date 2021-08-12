#version 450

#include "shaders/common.inc"

layout(location = 0) in v2 v_uv;

layout(set = 0, binding = 4) uniform sampler2D out_normal_3xf8_metallic_1xf8;
layout(set = 0, binding = 5) uniform sampler2D u_albedo_3xf8_roughness_1xf8;
layout(set = 0, binding = 6) uniform sampler2D u_depth;

layout(set = 0, binding = 7) uniform ubo
{
    //v4                               u_camera_position;
    //v4                               u_sun_direction;
    //u32                              u_directional_light_count;
    //u32                              u_point_light_count;
    m4                               u_ivp;
    //v4[TG_MAX_DIRECTIONAL_LIGHTS]    u_directional_light_directions;
    //v4[TG_MAX_DIRECTIONAL_LIGHTS]    u_directional_light_colors;
    //v4[TG_MAX_POINT_LIGHTS]          u_point_light_positions;
    //v4[TG_MAX_POINT_LIGHTS]          u_point_light_colors;
};

layout(location = 0) out v4 out_color;

void tg_unpack(out v3 p, out v3 n, out v3 a, out f32 m, out f32 r, out f32 d)
{
    d = texture(u_depth, v_uv).x;
    v4 screen = v4(v_uv.x * 2.0 - 1.0, v_uv.y * 2.0 - 1.0, d, 1.0);
    v4 world = u_ivp * screen;
    p = v3(world.x / world.w, world.y / world.w, world.z / world.w);

    v4 v0 = texture(out_normal_3xf8_metallic_1xf8, v_uv);
    v4 v1 = texture(u_albedo_3xf8_roughness_1xf8, v_uv);

    n = v0.xyz;
    a = v1.xyz;
    m = v0.w;
    r = v1.w;
}

void main()
{
    vec3 position, normal, albedo;
    float metallic, roughness, depth;
    tg_unpack(position, normal, albedo, metallic, roughness, depth);

    out_color = vec4(normal * 0.5 + 0.5, 1.0);
}
