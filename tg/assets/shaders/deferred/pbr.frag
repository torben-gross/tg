#version 450

#define TG_PACK(position_3xf32, normal_3xf32, albedo_3xf32, metallic_1xf32, roughness_1xf32) \
    out_position_3xf32_normal_3xu8_metallic_1xu8 = vec4(position_3xf32, tg_pack_4xf32_into_1xf32(vec4(normal_3xf32 * vec3(0.5) + vec3(0.5), metallic_1xf32))); \
    out_albedo_3xu8_roughness_1xu8 = vec4(albedo_3xf32, roughness_1xf32)

layout(location = 0) in vec3    v_position;
layout(location = 1) in vec3    v_normal;
layout(location = 2) in vec2    v_uv;
//layout(location = 3) in mat3    v_tbn; // TODO: this will be needed for normal mapping

layout(set = 0, binding = 2) uniform material
{
    vec4     albedo;
    float    metallic;
    float    roughness;
};

layout(location = 0) out vec4    out_position_3xf32_normal_3xu8_metallic_1xu8;
layout(location = 1) out vec4    out_albedo_3xu8_roughness_1xu8;

vec4 tg_unpack_1xf32_into_4xf32(float v)
{
    return vec4(float(uint(v) & 0x3f) / 64.0, float((uint(v) >> 6) & 0x3f) / 64.0, float((uint(v) >> 12) & 0x3f) / 64.0, float((uint(v) >> 18) & 0x3f) / 64.0);
}

float tg_pack_4xf32_into_1xf32(vec4 v)
{
    return float(uint(v.x * 64.0) | (uint(v.y * 64.0) << 6) | (uint(v.z * 64.0) << 12) | (uint(v.w * 64.0) << 18));
}

void main()
{
    vec3 p = v_position;
    vec3 n = normalize(v_normal);
    vec3 a = albedo.xyz;
    float m = metallic;
    float r = roughness;
    TG_PACK(p, n, a, m, r);
}
