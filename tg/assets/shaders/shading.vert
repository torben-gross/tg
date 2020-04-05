
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2    in_position;
layout(location = 1) in vec2    in_uv;

layout(location = 0) out vec2    v_uv;

void main()
{
    gl_Position = vec4(in_position, 0.0, 1.0);
    v_uv        = in_uv;
}
