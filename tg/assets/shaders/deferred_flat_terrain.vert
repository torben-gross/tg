#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3    in_primary_position;
layout(location = 1) in vec3    in_normal;
layout(location = 2) in vec3    in_secondary_position;
layout(location = 3) in int     in_border_mask;

layout(set = 0, binding = 0) uniform model
{
    mat4    u_model;
};

layout(set = 0, binding = 1) uniform view_projection
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

vec3 get_transvoxel_position()
{
	int cell_border_mask = in_border_mask & 63;
	int vertex_border_mask = (in_border_mask >> 6) & 63;
	int m = u_transition_mask & (cell_border_mask & 63);
	float t = float(m != 0);
	t *= float((vertex_border_mask & ~u_transition_mask) == 0);
	return mix(in_primary_position, in_secondary_position, t);
}

void main()
{
    gl_Position    = u_projection * u_view * u_model * vec4(get_transvoxel_position(), 1.0);
	v_position     = (u_model * vec4(in_primary_position, 1.0));
	v_normal       = (u_model * vec4(in_normal, 0.0)).xyz;
}
