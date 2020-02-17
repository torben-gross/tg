#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 projection;
} u_ubo;

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;

layout(location = 0) out vec3 v_color;

void main()
{
    gl_Position = vec4(in_position, -1.0, 1.0) * u_ubo.model * u_ubo.view * u_ubo.projection;
    v_color = in_color;
}
