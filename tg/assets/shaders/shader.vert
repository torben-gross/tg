
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3    in_position;
layout(location = 1) in vec3    in_color;
layout(location = 2) in vec2    in_uv;
layout(location = 3) in int     in_image;

layout(set = 0, binding = 0) uniform matrices
{
    mat4 model;
    mat4 view;
    mat4 projection;
} u_matrices;

layout(location = 0) out vec3        v_color;
layout(location = 1) out vec2        v_uv;
layout(location = 2) out flat int    v_image;

void main()
{
    gl_Position = u_matrices.projection * u_matrices.view  * u_matrices.model * vec4(in_position, 1.0);
    v_color     = in_color;
    v_uv        = in_uv;
    v_image     = in_image;
}
