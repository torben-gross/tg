#version 450

layout(location = 0) in vec4    v_position;
layout(location = 1) in vec3    v_normal;

layout(location = 0) out vec4    out_position;
layout(location = 1) out vec4    out_normal;
layout(location = 2) out vec4    out_albedo;
layout(location = 3) out vec4    out_metallic_roughness_ao;

void main()
{
    out_position = v_position;
    out_normal = vec4(normalize(v_normal), 1.0);

    float d = dot(out_normal.xyz, vec3(0.0, 1.0, 0.0));
    
    vec3 color_grass0 = vec3(204.0/255.0, 255.0/255.0, 0.0/255.0) * 0.1;
    vec3 color_grass1 = vec3(204.0/255.0, 255.0/255.0, 0.0/255.0) * 0.7;
    vec3 color_grass = mix(color_grass0, color_grass1, clamp((v_position.y - 100.0) / (130.0 - 100.0), 0.0, 1.0));
    vec3 color_stone = vec3(0.5, 0.2, 0.2);

    float t0 = max(0.0, (1.0 - pow(d, 4)));
    float t1 = clamp(0.2 * (v_position.y - 130.0), 1.0, 10.0);
    float t = clamp(t0 * t1, 0.0, 1.0);
    out_albedo = vec4(mix(color_grass, color_stone, t), 1.0);

    float roughness_grass = 1.0;
    float roughness_stone = 0.5;
    float roughness = mix(roughness_grass, roughness_stone, t);
    out_metallic_roughness_ao = vec4(0.0, roughness, 1.0, 0.0);
}
