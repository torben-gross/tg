#version 450

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_atomic_int64                    : require

#include "shaders/common.inc"

layout(location = 0) in v3    v_position;
layout(location = 1) in v3    v_normal;

layout(set = 0, binding = 2) uniform model
{
    v2i    window_size;
};

layout(set = 0, binding = 3) buffer visibility_buffer
{
    uint64_t    u_visibility_buffer[];
};

void main()
{
    u32 x = u32(gl_FragCoord.x * f32(window_size.x));
    u32 y = u32(gl_FragCoord.y * f32(window_size.y));
    u32 i = window_size.x * y + x;
    uint64_t data = uint64_t(3);
    atomicMin(u_visibility_buffer[i], data);
}
