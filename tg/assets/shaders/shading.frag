#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2    v_uv;

layout(set = 0, binding = 0) uniform    sampler2D positions;
layout(set = 0, binding = 1) uniform    sampler2D normals;
layout(set = 0, binding = 2) uniform    sampler2D albedos;

layout(set = 0, binding = 3) uniform light_setup
{
    uint          directional_light_count;
    uint          point_light_count;
    vec4[512]    directional_light_positions_radii;
    vec3[512]    directional_light_colors;
    vec4[512]    point_light_positions_radii;
    vec3[512]    point_light_colors;
};

layout(location = 0) out vec4 out_color;

void main()
{
    // geometry
    vec3 position = texture(positions, v_uv).xyz;
    vec3 normal = normalize(texture(normals, v_uv).xyz);
    vec4 albedo = texture(albedos, v_uv);

    // ambient
    float ambient_strength = 0.16;
    vec3 ambient = ambient_strength * vec3(1.0);

    // diffuse
    vec3 diffuse = vec3(0.0);
    for (uint i = 0; i < point_light_count; i++)
    {
        vec3 point_light_position = point_light_positions_radii[i].xyz;
        float point_light_radius = point_light_positions_radii[i].w;

        float distance_from_light_center = distance(point_light_position, position);
        if (distance_from_light_center <= point_light_radius)
        {
            vec3 light_direction = normalize(point_light_position - position);
            float t = max(0.0, (point_light_radius - distance_from_light_center)) / point_light_radius;
            float diffuse_strength = max(dot(normal, light_direction), 0.0) * t;
            diffuse += diffuse_strength * point_light_colors[i];
        }
    }

    // final color assembly
    out_color = vec4((ambient + diffuse), 1.0) * albedo;
}
