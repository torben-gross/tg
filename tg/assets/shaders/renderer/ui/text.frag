#version 450

layout(location = 0) in vec2    v_uv;

layout(set = 0, binding = 0) uniform sampler2D texture_atlas;

layout(location = 0) out vec4    out_color;

void main()
{
	float value = texture(texture_atlas, v_uv).x;
	out_color = vec4(value);
}
