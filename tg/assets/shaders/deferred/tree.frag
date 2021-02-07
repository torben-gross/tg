#version 450

#define f32    float
#define i32    int
#define u32    uint

#define v2     vec2
#define v2i    ivec2
#define v2u    uvec2
#define v3     vec3
#define v3i    ivec3
#define v3u    uvec3
#define v4     vec4
#define v4i    ivec4
#define v4u    uvec4

#define m2     mat2
#define m3     mat3
#define m4     mat4

layout(location = 0) in vec3    v_position;
layout(location = 1) in vec3    v_normal;
layout(location = 2) in vec2    v_uv;
//layout(location = 3) in mat3    v_tbn; // TODO: this will be needed for normal mapping

layout(set = 0, binding = 2) uniform sampler2D tex;

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
    vec3 p = v_position;
    vec3 n = normalize(v_normal);
    vec3 a = texture(tex, v_uv).xyz;
    float m = 0.0f;
    float r = 1.0f;
    tg_pack(p, n, a, m, r);
}
