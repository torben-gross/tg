#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3    in_position;
layout(location = 1) in vec3    in_normal;

layout(set = 0, binding = 0) uniform view_projection
{
    mat4    u_view;
    mat4    u_projection;
};

layout(set = 1, binding = 0) uniform transition_mask
{
	int    u_transition_mask;
};

layout(location = 0) out vec4    v_position;
layout(location = 1) out vec3    v_normal;

void main()
{
    gl_Position    = u_projection * u_view * vec4(in_position, 1.0);
	v_position     = vec4(in_position, 1.0);
	v_normal       = vec4(in_normal, 0.0).xyz;
}
