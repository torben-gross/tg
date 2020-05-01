#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4    v_position;
layout(location = 1) in vec3    v_normal;
layout(location = 2) in vec2    v_uv;
layout(location = 3) in mat3    v_tbn; // TODO: this will be needed for normal mapping

layout(location = 0) out vec4    out_color;

layout(set = 1, binding = 0) uniform color
{
	vec3    u_color;
};

layout(set = 1, binding = 1) uniform sampler2D test_icon;

void main()
{
    out_color = vec4(u_color, 1.0) * 0.1 + 0.9 * texture(test_icon, v_uv);
}