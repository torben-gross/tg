#version 450

layout(location = 0) in vec3    in_position;
layout(location = 1) in vec4    in_color;

layout(set = 0, binding = 0) uniform view_projection
{
    mat4    u_view;
    mat4    u_projection;
};

layout(location = 0) out vec3    v_position;
layout(location = 1) out vec4    v_color;

void main()
{
    gl_Position    = u_projection * u_view * vec4(in_position, 1.0);
	v_position     = in_position;
	v_color        = in_color;
}
