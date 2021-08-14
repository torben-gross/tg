#version 450

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_atomic_int64                    : require

#include "shaders/common.inc"

layout(location = 0) in v3 v_position;

layout(set = 0, binding = 0) uniform model
{
    m4     u_model;
    u32    u_first_voxel_id;
};

layout(set = 0, binding = 2) buffer visibility_buffer
{
    u32         u_w;
    u32         u_h;
    uint64_t    u_vb[];
};

uint64_t tg_pack(f32 d, u32 id)
{
    return uint64_t(id) | (uint64_t(d * f32(TG_U32_MAX)) << uint64_t(32));
}

void main()
{
    u32 x = u32(gl_FragCoord.x);
    u32 y = u32(gl_FragCoord.y);
    f32 d = gl_FragCoord.z / gl_FragCoord.w;
    u32 id = x*x + y + x*374261; // TODO
    uint64_t data = tg_pack(d, id);
    u32 i = u_w * y + x;
    atomicMin(u_vb[i], data);
}
