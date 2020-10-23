#version 450

//#define TG_TONE_MAP_ACES_FILM
//#define TG_TONE_MAP_UNCHARTED_2

layout(location = 0) in vec2 v_uv;

layout(set = 0, binding = 0) uniform sampler2D u_present_texture;

readonly layout(set = 0, binding = 1) buffer exposure
{
	float u_exposure;
};

layout(location = 0) out vec4 out_color;



vec3 tg_aces_film(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 tg_uncharted_2(vec3 x)
{
    const float a = 0.15;
    const float b = 0.50;
    const float c = 0.10;
    const float d = 0.20;
    const float e = 0.02;
    const float f = 0.30;
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

void main()
{
	vec3 hdr = texture(u_present_texture, v_uv).xyz;
    vec3 adapted = hdr / vec3(u_exposure);

#if defined(TG_TONE_MAP_ACES_FILM)
	vec3 tone_mapped = tg_aces_film(adapted);
	
#elif defined(TG_TONE_MAP_UNCHARTED_2)
    const float exposure_bias = 2.0;
    vec3 tone_mapped = tg_uncharted_2(exposure_bias * adapted);
    const float w = 11.2;
    vec3 white_scale = 1.0 / tg_uncharted_2(vec3(w));
    tone_mapped.x = pow(tone_mapped.x, 1 / 2.2);
    tone_mapped.y = pow(tone_mapped.y, 1 / 2.2);
    tone_mapped.z = pow(tone_mapped.z, 1 / 2.2);
#else
    vec3 tone_mapped = adapted;
#endif

    out_color = vec4(tone_mapped, 1.0);
}
