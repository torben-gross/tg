#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2    v_uv;



layout(set = 0, binding = 0) uniform    sampler2D u_position;
layout(set = 0, binding = 1) uniform    sampler2D u_normal;
layout(set = 0, binding = 2) uniform    sampler2D u_albedo;
layout(set = 0, binding = 3) uniform    sampler2D u_metallic_roughness_ao;

layout(set = 0, binding = 4) uniform camera
{
    vec3    u_camera_position;
};

layout(set = 0, binding = 5) uniform light_setup
{
    uint         u_directional_light_count;
    uint         u_point_light_count;
    vec4[512]    u_directional_light_directions;
    vec4[512]    u_directional_light_colors;
    vec4[512]    u_point_light_positions;
    vec4[512]    u_point_light_colors;
};

layout(set = 0, binding = 6) uniform lightspace_matrix
{
    mat4 u_lightspace_matrix;
};

layout(set = 0, binding = 7) uniform    sampler2D u_shadow_map;



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

float shadow_mapping(vec4 position_lightspace)
{
    vec3 proj = position_lightspace.xyz / position_lightspace.w;
    float shadow = 1.0;
    if (proj.z > 0.0 && proj.z < 1.0)
    {
        float bias = 0.01;
        if (proj.x > -1.0 && proj.x < 1.0 - bias && proj.y > -1.0 + bias && proj.y < 1.0)
        {
            vec2 proj2 = proj.xy * 0.5 + 0.5;
            float current = proj.z;
            for (int x = -1; x <= 1; x++)
            {
                for (int y = -1; y <= 1; y++)
                {
                    float pcf_depth = texture(u_shadow_map, proj2 + vec2(float(x), float(y)) * 0.0009765625).x;
                    shadow += current - bias < pcf_depth ? 1.0 : 0.0;
                }
            }
            shadow /= 9;
        }
    }
    return shadow;
}

void main()
{
    vec3  position            = texture(u_position, v_uv).xyz;
    vec3  normal              = texture(u_normal, v_uv).xyz;
    vec3  albedo              = texture(u_albedo, v_uv).xyz;
    float metallic            = texture(u_metallic_roughness_ao, v_uv).x;
    float roughness           = texture(u_metallic_roughness_ao, v_uv).y;
    float ao                  = texture(u_metallic_roughness_ao, v_uv).z;
    vec4  position_lightspace = u_lightspace_matrix * texture(u_position, v_uv);

    vec4 sky_color = 0.7 * u_directional_light_colors[0];
    if (dot(normal, normal) < 0.5) // sky
    {
        out_color = sky_color;
    }
    else
    {
        vec3 n = normalize(normal);
        vec3 v = normalize(u_camera_position - position);

        vec3 lo = vec3(0.0);

        for (int i = 0; i < u_directional_light_count; i++)
        {
            vec3 l = normalize(-u_directional_light_directions[i].xyz);
            vec3 h = normalize(v + l);
            
            float attenuation = 1.0;
            vec3  radiance    = u_directional_light_colors[i].xyz * attenuation;

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

        for (int i = 0; i < u_point_light_count; i++)
        {
            vec3 l = normalize(u_point_light_positions[i].xyz - position);
            vec3 h = normalize(v + l);
            
            float distance    = length(u_point_light_positions[i].xyz - position);
            float attenuation = 1.0 / (distance * distance);
            vec3  radiance    = u_point_light_colors[i].xyz * attenuation;

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
        vec3 color   = ambient + shadow_mapping(position_lightspace) * lo;

        color = color / (color + vec3(1.0));
        color = pow(color, vec3(1.0 / 2.2));

        vec3  d = u_camera_position - position;
        float t = clamp(sqrt(dot(d, d)) / 256.0f, 0.0, 1.0);
        color   = mix(color, sky_color.xyz, t);

        out_color = vec4(color, 1.0);
    }
}
