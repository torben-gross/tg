#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2    v_uv;

layout(set = 0, binding = 0) uniform    sampler2D positions;
layout(set = 0, binding = 1) uniform    sampler2D normals;
layout(set = 0, binding = 2) uniform    sampler2D albedos;

layout(location = 0) out vec4 out_color;

void main()
{
    // temporary constant values
    vec3 light_color       = vec3(1.0);
    vec3 light_position    = vec3(0.0, 0.0, 0.5);



    // geometry
    vec3 position             = texture(positions, v_uv).xyz;
    vec3 normal               = normalize(texture(normals, v_uv).xyz);
    vec4 albedo               = texture(albedos, v_uv);

    // ambient
    float ambient_strength    = 0.16;
    vec3  ambient             = ambient_strength * light_color;

    // diffuse
    vec3  light_direction     = normalize(light_position - position);
    float diffuse_strength    = max(dot(normal, light_direction), 0.0);
    vec3  diffuse             = diffuse_strength * light_color;

    // final color assembly
    out_color                 = vec4((ambient + diffuse), 1.0) * albedo;
}
