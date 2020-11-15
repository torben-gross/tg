#version 450

layout(location = 0) in vec4    v_position;
layout(location = 1) in vec3    v_normal;
layout(location = 2) in vec2    v_uv;
//layout(location = 3) in mat3    v_tbn; // TODO: this will be needed for normal mapping

layout(location = 0) out vec4    out_color;

layout(set = 0, binding = 2) uniform color
{
	vec3    u_color;
};

layout(set = 0, binding = 3) uniform sampler2D tex;

void main()
{
    out_color = vec4(u_color, 1.0) * 0.1 + 0.9 * texture(tex, v_uv);
	const float start = 0.1;
	float d = (v_uv.x - 0.5) * (v_uv.x - 0.5) + (v_uv.y - 0.5) * (v_uv.y - 0.5);
	if (d > start)
	{
		out_color.a = smoothstep(1.0, 0.0, (d - start) / (0.25 - start));
	}
	if (dot(texture(tex, v_uv).rgb, texture(tex, v_uv).rgb) == 0.0)
	{
		out_color.a *= 0.5;
	}
}
