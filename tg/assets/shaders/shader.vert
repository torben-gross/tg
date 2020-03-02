#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_uv;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 projection;
} u_ubo;

layout(location = 0) out vec3 v_color;
layout(location = 1) out vec2 v_uv;

void main()
{
    gl_Position = u_ubo.projection * u_ubo.view  * u_ubo.model * vec4(in_position, 1.0);
    v_color = in_color;
    v_uv = in_uv;
}
