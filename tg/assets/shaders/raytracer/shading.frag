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
    
u64 tg_hash_u64(u64 v) // MurmurHash3 algorithm by Austin Appleby
{
    u64 h = v;
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53L;
    h ^= h >> 33;
    return h;
}

void main()
{
    u32 x = u32(gl_FragCoord.x);
    u32 y = u32(gl_FragCoord.y);
    u32 i = visibility_buffer_w * y + x;
    u64 data = visibility_buffer_data[i];
    //f32 d32 = uintBitsToFloat(u32(data >> u64(32)));
    f32 d24 = f32(data >> u64(40)) / 16777215.0;
    u64 id40 = data & ((u64(1) << u64(40)) - u64(1));

    u64 rui = tg_hash_u64(id40);
    u64 gui = tg_hash_u64(rui);
    u64 bui = tg_hash_u64(gui);
    f32 r = f32(rui) / f32(18446744073709551615.0);
    f32 g = f32(gui) / f32(18446744073709551615.0);
    f32 b = f32(bui) / f32(18446744073709551615.0);

    out_color = vec4(r, g, b, 1.0);
    //out_color = vec4(d24, d24, d24, 1.0);
}
