#version 450

#define TG_PACK(position_3xf32, normal_3xf32, albedo_3xf32, metallic_1xf32, roughness_1xf32) \
    out_position_3xf32_normal_3xu8_metallic_1xu8 = vec4(position_3xf32, tg_pack_4xf32_into_1xf32(vec4(normal_3xf32 * vec3(0.5) + vec3(0.5), metallic_1xf32))); \
    out_albedo_3xu8_roughness_1xu8 = vec4(albedo_3xf32, roughness_1xf32)

layout(location = 0) in vec3    v_position;
layout(location = 1) in vec3    v_normal;

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
    vec3 p = v_position.xyz;
    vec3 n = normalize(v_normal);
    float d = dot(n.xyz, vec3(0.0, 1.0, 0.0));
    
    vec3 color_grass = vec3(160.0/255.0, 150.0/255.0, 40.0/255.0);
    vec3 color_stone = vec3(247.0/255.0, 226.0/255.0, 164.0/255.0);

    float t = d < 0.5 ? 0.0 : 1.0;
    vec3 a = mix(color_stone, color_grass, t);

    float m = 0.0;

    float roughness_grass = 1.0;
    float roughness_stone = 0.5;
    float r = mix(roughness_stone, roughness_grass, t);

    TG_PACK(p, n, a, m, r);
}
