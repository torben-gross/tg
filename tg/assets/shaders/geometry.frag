#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4    v_position;
layout(location = 1) in vec3    v_normal;
layout(location = 2) in vec2    v_uv;
layout(location = 3) in mat3    v_tbn; // TODO: this will be needed for normal mapping

layout(location = 0) out vec4    out_position;
layout(location = 1) out vec4    out_normal;

void main()
{
    out_position    = v_position;
    out_normal      = vec4(normalize(normalize(v_normal)), 1.0);
}
