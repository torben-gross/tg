#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2    v_uv;

layout(set = 0, binding = 0) uniform    sampler2D positions;
layout(set = 0, binding = 1) uniform    sampler2D normals;

layout(location = 0) out vec4 out_color;

void main()
{
    // temporary constant values
    vec3 albedo = vec3(1.0, 1.0, 1.0);
    vec3 light_color = vec3(1.0);
    vec3 light_position = vec3(-10.0, 2.0, 5.0);



    // geometry
    vec3 position = texture(positions, v_uv).xyz;
    vec3 normal = texture(normals, v_uv).xyz;

    // ambient
    float ambient_strength = 0.16;
    vec3 ambient = ambient_strength * light_color;

    // diffuse
    vec3 light_direction = normalize(light_position - position);
    float diffuse_strength = max(dot(normal, light_direction), 0.0);
    vec3 diffuse = diffuse_strength * light_color;

    if (length(normal) != 0)
    {
        vec3 color = (ambient + diffuse) * albedo;
        out_color = vec4(color, 1.0);
    }
    else
    {
        out_color = vec4(0.0);
    }
}
