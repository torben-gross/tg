#version 450

layout(location = 0) in vec2 v_uv;

layout(set = 0, binding = 0) uniform sampler2D u_present_texture;

readonly layout(set = 0, binding = 1) buffer exposure
{
	float u_exposure;
};

layout(location = 0) out vec4 out_color;

vec3 aces_film(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
	vec3 hdr = texture(u_present_texture, v_uv).xyz;
    vec3 adapted = hdr / vec3(u_exposure);
	//vec3 tone_mapped = aces_film(adapted);
    vec3 tone_mapped = adapted;
    out_color = vec4(tone_mapped, 1.0);
}
