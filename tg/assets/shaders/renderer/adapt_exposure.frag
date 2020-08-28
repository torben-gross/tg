#version 450

layout(location = 0) in vec2 v_uv;

layout(set = 0, binding = 0) uniform sampler2D u_present_texture;

readonly layout(set = 0, binding = 1) buffer exposure
{
	float u_exposure;
};

layout(location = 0) out vec4 out_color;

void main()
{
	vec4 hdr = texture(u_present_texture, v_uv);
    out_color = hdr / vec4(vec3(u_exposure), 1.0);
}
