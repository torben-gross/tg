#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 v_uv;

layout(set = 0, binding = 0) uniform sampler2D u_texture;

layout(set = 0, binding = 1) uniform uniform_with_block
{
	int u_int;
};

layout(set = 0, binding = 2) buffer buffer_with_block
{
	float b_float;
};

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(1.0);
}
