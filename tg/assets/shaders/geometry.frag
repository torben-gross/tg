#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4    v_position;
layout(location = 1) in vec3    v_normal;
layout(location = 2) in vec2    v_uv;
layout(location = 3) in mat3    v_tbn; // TODO: this will be needed for normal mapping

layout(location = 0) out vec4    out_position;
layout(location = 1) out vec4    out_normal;
layout(location = 2) out vec4    out_albedo;

void main()
{
    out_position    = v_position;
    out_normal      = vec4(normalize(v_normal), 1.0);
    float t         = v_position.y / 64.0;
    out_albedo      = mix(vec4(1.0, 0.2, 0.4, 1.0), vec4(0.2, 0.0, 1.0, 1.0), t);
}
