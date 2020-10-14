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
    
    //vec3 color_grass = vec3(204.0/255.0, 255.0/255.0, 0.0/255.0);
    vec3 color_grass = vec3(255.0/255.0, 192.0/255.0, 0.0/255.0);
    //vec3 color_stone = vec3(0.5, 0.2, 0.2);
    vec3 color_stone = vec3(129.0/255.0, 110.0/255.0, 100.0/255.0);

    float t = d < 0.5 ? 0.0 : 1.0;
    out_albedo = vec4(mix(color_stone, color_grass, t), 1.0);

    float roughness_grass = 1.0;
    float roughness_stone = 0.5;
    float roughness = mix(roughness_stone, roughness_grass, t);
    out_metallic_roughness_ao = vec4(0.0, roughness, 1.0, 0.0);
}
