#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2    v_uv;

layout(set = 0, binding = 0) uniform    sampler2D position;
layout(set = 0, binding = 1) uniform    sampler2D normal;
layout(set = 0, binding = 2) uniform    sampler2D albedo;
layout(set = 0, binding = 3) uniform    sampler2D metallic_roughness_ao;

layout(set = 0, binding = 4) uniform camera
{
    vec3    camera_position;
};

layout(set = 0, binding = 5) uniform light_setup
{
    uint         directional_light_count;
    uint         point_light_count;
    vec4[512]    p_directional_light_directions;
    vec4[512]    p_directional_light_colors;
    vec4[512]    p_point_light_positions;
    vec4[512]    p_point_light_colors;
};

layout(location = 0) out vec4 out_color;



float pi = 3.14159265358979323846;



float distribution_ggx(vec3 n, vec3 h, float roughness)
{
    float a       = roughness * roughness;
    float a_sqr   = a * a;
    float ndh     = max(dot(n, h), 0.0);
    float denom = (ndh * ndh * (a_sqr - 1.0) + 1.0);
    denom       = pi * denom * denom;
	
    return a_sqr / denom;
}

float geometry_schlick_ggf(float n_dot_v, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float denom = n_dot_v * (1.0 - k) + k;
	
    return n_dot_v / denom;
}

float geometry_smith(vec3 n, vec3 v, vec3 l, float roughness)
{
    float ndv = max(dot(n, v), 0.0);
    float ndl = max(dot(n, l), 0.0);
    float ggx2  = geometry_schlick_ggf(ndv, roughness);
    float ggx1  = geometry_schlick_ggf(ndl, roughness);
	
    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cos_theta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(1.0 - cos_theta, 5.0);
}

void main()
{
    vec3  position  = texture(position, v_uv).xyz;
    vec3  normal    = texture(normal, v_uv).xyz;
    vec3  albedo    = texture(albedo, v_uv).xyz;
    float metallic  = texture(metallic_roughness_ao, v_uv).x;
    float roughness = texture(metallic_roughness_ao, v_uv).y;
    float ao        = texture(metallic_roughness_ao, v_uv).z;

    vec4 sky_color = 0.7 * p_directional_light_colors[0];
    if (dot(normal, normal) < 0.5) // sky
    {
        out_color = sky_color;
    }
    else
    {
        vec3 n = normalize(normal);
        vec3 v = normalize(camera_position - position);

        vec3 lo = vec3(0.0);

        for (int i = 0; i < directional_light_count; i++)
        {
            vec3 l = normalize(-p_directional_light_directions[i].xyz);
            vec3 h = normalize(v + l);
            
            float attenuation = 1.0;
            vec3  radiance    = p_directional_light_colors[i].xyz * attenuation;

            vec3 f0 = mix(vec3(0.04), albedo, metallic);

            float ndf = distribution_ggx(n, h, roughness);
            float g   = geometry_smith(n, v, l, roughness);
            vec3  f   = fresnel_schlick(max(dot(h, v), 0.0), f0);

            vec3  numerator   = ndf * g * f;
            float denominator = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0);
            vec3  specular    = numerator / max(denominator, 0.001);

            vec3 k_specular = f;
            vec3 k_diffuse  = (vec3(1.0) - k_specular) * (1.0 - metallic);
        
            float ndl = max(dot(n, l), 0.0);
            lo += (k_diffuse * albedo / pi + specular) * radiance * ndl;
        }

        for (int i = 0; i < point_light_count; i++)
        {
            vec3 l = normalize(p_point_light_positions[i].xyz - position);
            vec3 h = normalize(v + l);
            
            float distance    = length(p_point_light_positions[i].xyz - position);
            float attenuation = 1.0 / (distance * distance);
            vec3  radiance    = p_point_light_colors[i].xyz * attenuation;

            vec3 f0 = mix(vec3(0.04), albedo, metallic);

            float ndf = distribution_ggx(n, h, roughness);
            float g   = geometry_smith(n, v, l, roughness);
            vec3  f   = fresnel_schlick(max(dot(h, v), 0.0), f0);

            vec3  numerator   = ndf * g * f;
            float denominator = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0);
            vec3  specular    = numerator / max(denominator, 0.001);

            vec3 k_specular = f;
            vec3 k_diffuse  = (vec3(1.0) - k_specular) * (1.0 - metallic);
        
            float ndl = max(dot(n, l), 0.0);
            lo += (k_diffuse * albedo / pi + specular) * radiance * ndl;
        }

        vec3 ambient = vec3(0.01) * albedo * ao;
        vec3 color   = ambient + lo;

        color = color / (color + vec3(1.0));
        color = pow(color, vec3(1.0 / 2.2));

        vec3  d = camera_position - position;
        float t = clamp(sqrt(dot(d, d)) / 256.0f, 0.0, 1.0);
        color   = mix(color, sky_color.xyz, t);

        out_color = vec4(color, 1.0);
    }
}
