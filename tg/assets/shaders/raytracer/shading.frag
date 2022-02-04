#version 450

#include "shaders/common.inc"
#include "shaders/commoni64.inc"

layout(location = 0) in v2 v_uv;

layout(set = 0, binding = 4) buffer tg_visibility_buffer_ssbo
{
    u32    visibility_buffer_w;
    u32    visibility_buffer_h;
    u64    visibility_buffer_data[];
};

layout(set = 0, binding = 5) uniform ubo
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

u32 tg_hash_u32(u32 v) // Knuth's multiplicative method
{
    return v * 2654435761;
}

void main()
{
    u32 x = u32(gl_FragCoord.x);
    u32 y = u32(gl_FragCoord.y);
    u32 i = visibility_buffer_w * y + x;
    u64 data = visibility_buffer_data[i];
    f32 d = uintBitsToFloat(u32(data >> u64(32)));
    u32 id = u32(data & u64(TG_U32_MAX));

    u32 rui = tg_hash_u32(id);
    u32 gui = tg_hash_u32(rui);
    u32 bui = tg_hash_u32(gui);
    f32 g = f32(gui) / 4294967295.0;
    f32 r = f32(rui) / 4294967295.0;
    f32 b = f32(bui) / 4294967295.0;

    out_color = vec4(r, g, b, 1.0);
    //out_color = vec4(d, d, d, 1.0);
}
