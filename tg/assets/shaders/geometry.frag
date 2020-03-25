#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4    v_position;
layout(location = 1) in vec3    v_normal;
layout(location = 2) in vec2    v_uv;

layout(location = 0) out vec4    out_position;

void main()
{
    out_position = vec4(v_uv, 0.0, 1.0);
}
