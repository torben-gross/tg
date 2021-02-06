#version 450

layout(location = 0) in vec3    in_position;
layout(location = 1) in vec3    in_normal;
layout(location = 2) in vec2    in_uv;
//layout(location = 3) in vec3    in_tangent;
//layout(location = 4) in vec3    in_bitangent;

layout(set = 0, binding = 0) uniform model
{
    mat4     u_model;
};

layout(set = 0, binding = 1) uniform view_projection
{
    mat4    u_view;
    mat4    u_projection;
};

layout(location = 0) out vec3    v_position;
layout(location = 1) out vec3    v_normal;
layout(location = 2) out vec2    v_uv;
//layout(location = 3) out mat3    v_tbn;

void main()
{
    gl_Position    = u_projection * u_view * u_model * vec4(in_position, 1.0);
	v_position     = (u_model * vec4(in_position, 1.0)).xyz;
	v_normal       = (u_model * vec4(in_normal, 0.0)).xyz;
	v_uv           = in_uv;

	//vec3 t         = normalize((u_model * vec4(in_tangent,   0.0)).xyz);
	//vec3 b         = normalize((u_model * vec4(in_bitangent, 0.0)).xyz);
	//vec3 n         = normalize((u_model * vec4(in_normal,    0.0)).xyz);
	//v_tbn          = mat3(t, b, n);
}
