#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3    in_position;
layout(location = 1) in vec3    in_normal;
layout(location = 2) in vec2    in_uv;

layout(set = 0, binding = 0) uniform matrices
{
    mat4    u_model;
    mat4    u_view;
    mat4    u_projection;
};

layout(location = 0) out vec4    v_position;
layout(location = 1) out vec3    v_normal;
layout(location = 2) out vec2    v_uv;

void main()
{
    gl_Position = u_projection * u_view * u_model * vec4(in_position, 1.0);
	v_position  = (u_model * vec4(in_position, 1.0));
	v_normal    = (u_model * vec4(in_normal, 0.0)).xyz;
	v_uv        = in_uv;
}
