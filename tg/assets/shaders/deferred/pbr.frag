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
    v3 p = v_position;
    v3 n = normalize(v_normal);
    v3 a = albedo.xyz;
    f32 m = metallic;
    f32 r = roughness;
    tg_pack(p, n, a, m, r);
}
