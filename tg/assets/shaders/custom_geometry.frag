#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4    v_position;
layout(location = 1) in vec3    v_normal;
layout(location = 2) in vec2    v_uv;
layout(location = 3) in mat3    v_tbn; // TODO: this will be needed for normal mapping

layout(location = 0) out vec4    out_position;
layout(location = 1) out vec4    out_normal;
layout(location = 2) out vec4    out_albedo;

layout(set = 1, binding = 0) uniform color
{
	vec3    u_color;
};

void main()
{
    out_position    = v_position;
    out_normal      = vec4(normalize(v_normal), 1.0);
    out_albedo      = vec4(u_color, 1.0);
}
