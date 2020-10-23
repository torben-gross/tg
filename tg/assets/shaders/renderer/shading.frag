#version 450

#define TG_CASCADED_SHADOW_MAPS          4
#define TG_CASCADED_SHADOW_MAP_SIZE      1024
#define TG_CASCADED_SHADOW_MAP_SIZE_F    1024.0
#define TG_SHADOW_BIAS                   0.001
#define TG_MAX_DIRECTIONAL_LIGHTS        512
#define TG_MAX_POINT_LIGHTS              512
#define TG_SHADOW_SAMPLE_COUNT           2
#define TG_PI                            3.14159265358979323846



layout(location = 0) in vec2 v_uv;



layout(set = 0, binding = 0) uniform sampler2D u_position;
layout(set = 0, binding = 1) uniform sampler2D u_normal;
layout(set = 0, binding = 2) uniform sampler2D u_albedo;
layout(set = 0, binding = 3) uniform sampler2D u_metallic_roughness_ao;

layout(set = 0, binding = 4) uniform render_info
{
    vec3                                       u_camera_position;
    uint                                       u_directional_light_count;
    uint                                       u_point_light_count;
    vec4[TG_MAX_DIRECTIONAL_LIGHTS]            u_directional_light_directions;
    vec4[TG_MAX_DIRECTIONAL_LIGHTS]            u_directional_light_colors;
    vec4[TG_MAX_POINT_LIGHTS]                  u_point_light_positions;
    vec4[TG_MAX_POINT_LIGHTS]                  u_point_light_colors;
    mat4[TG_CASCADED_SHADOW_MAPS]              u_lightspace_matrices;
    vec4[(TG_CASCADED_SHADOW_MAPS + 3) / 4]    u_shadow_distances;
};

layout(set = 0, binding = 5) uniform sampler2D[TG_CASCADED_SHADOW_MAPS] u_shadow_maps;
layout(set = 0, binding = 6) uniform sampler2D u_ssao;



layout(location = 0) out vec4 out_color;



const vec2 shadow_poisson_disk[16] = vec2[](
    vec2(-0.94201624,  -0.39906216),
    vec2( 0.94558609,  -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2( 0.34495938,   0.29387760),
    vec2(-0.91588581,   0.45771432),
    vec2(-0.81544232,  -0.87912464),
    vec2(-0.38277543,   0.27676845),
    vec2( 0.97484398,   0.75648379),
    vec2( 0.44323325,  -0.97511554),
    vec2( 0.53742981,  -0.47373420),
    vec2(-0.26496911,  -0.41893023),
    vec2( 0.79197514,   0.19090188),
    vec2(-0.24188840,   0.99706507),
    vec2(-0.81409955,   0.91437590),
    vec2( 0.19984126,   0.78641367),
    vec2( 0.14383161,  -0.14100790)
);



float tg_distribution_ggx(float clamped_n_dot_h, float roughness)
{
    float a = roughness * roughness;
    float a_sqr = a * a;
    float denom = (clamped_n_dot_h * clamped_n_dot_h * (a_sqr - 1.0) + 1.0);
    denom = TG_PI * denom * denom;
	
    return a_sqr / denom;
}

float tg_geometry_schlick_ggf(float n_dot_v, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float denom = n_dot_v * (1.0 - k) + k;
	
    return n_dot_v / denom;
}

float tg_geometry_smith(float clamped_n_dot_v, float clamped_n_dot_l, float roughness)
{
    float ggx2 = tg_geometry_schlick_ggf(clamped_n_dot_v, roughness);
    float ggx1 = tg_geometry_schlick_ggf(clamped_n_dot_l, roughness);
	
    return ggx1 * ggx2;
}

float tg_fresnel_schlick(float u, float roughness)
{
    float f_lambda = 1.0 - roughness;
    return f_lambda + (1.0 - f_lambda) * pow(1.0 - u, 5.0);
}

float tg_random(vec3 seed, int i)
{
    vec4 seed4 = vec4(seed, float(i));
    float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(dot_product) * 43758.5453);
}

float tg_shadow_factor()
{
    float shadow = 1.0;

    vec3  position = texture(u_position, v_uv).xyz;
    float d = distance(position, u_camera_position);

    int shadow_map_index = -1;
    int secondary = -1;
    float t = 0.0;
    for (int i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
    {
        int major0 = i / 4;
        int minor0 = i % 4;
        int major1 = (i + 1) / 4;
        int minor1 = (i + 1) % 4;
        
        float f0 = u_shadow_distances[major0][minor0];
        float f1 = u_shadow_distances[major1][minor1];

        if (f0 < d && d < f1)
        {
            shadow_map_index = i;
            if (d - f0 <= 0.4) // TODO: the 0.4 is quite hard coded!
            {
                secondary = i - 1;
                t = (1.0 - (d - f0) / 0.4) * 0.5;
            }
            else if (f1 - d <= 0.4)
            {
                secondary = i + 1;
                t = (1.0 - (f1 - d) / 0.4) * 0.5;
            }
            break;
        }
    }

    if (shadow_map_index != -1)
    {
        vec4 position_lightspace = u_lightspace_matrices[shadow_map_index] * texture(u_position, v_uv);

        vec3 proj = (position_lightspace.xyz / vec3(position_lightspace.w)) * vec3(0.5, 0.5, 1.0) + vec3(0.5, 0.5, 0.0);
        if (proj.x > 0.0 && proj.x < 1.0 &&
            proj.y > 0.0 && proj.y < 1.0 &&
            proj.z > 0.0 && proj.z < 1.0
        )
        {
            //const int sample_count = 4;
            //float shadow_sum = 0.0;
            //for (int y = 0; y < sample_count; y++)
            //{
            //    for (int x = 0; x < sample_count; x++)
            //    {
            //        vec2 offset = (vec2(x, y) - 0.5 * float(sample_count)) / TG_CASCADED_SHADOW_MAP_SIZE_F;
            //        vec2 uv = clamp(vec2(proj.xy + offset), 0.0, 1.0);
            //        float depth = texture(u_shadow_maps[shadow_map_index], uv).x;
            //        if (proj.z - TG_SHADOW_BIAS >= depth)
            //        {
            //            shadow_sum += 1.0;
            //        }
            //    }
            //}
            //shadow = 1.0 - clamp(shadow_sum / float(sample_count * sample_count), 0.0, 1.0);
            
            //float depth = 0.0;
            //for (int y = -1; y <= 1; y++)
            //{
            //    for (int x = -1; x <= 1; x++)
            //    {
            //        vec2 offset = vec2(x, y) / TG_CASCADED_SHADOW_MAP_SIZE_F;
            //        vec2 uv = clamp(vec2(proj.xy + offset), 0.0, 1.0);
            //        depth += texture(u_shadow_maps[shadow_map_index], uv).x;
            //    }
            //}
            //depth = clamp(depth / 9.0, 0.0, 1.0);
            //shadow = proj.z - TG_SHADOW_BIAS < depth ? 1.0 : 0.0;



            //float shadow_sum = 0.0;
            //for (int i = 0; i < TG_SHADOW_SAMPLE_COUNT; i++)
            //{
		    //    int index = int(16.0 * tg_random(floor(position * TG_CASCADED_SHADOW_MAP_SIZE_F), i)) % 16;
            //    vec2 uv = clamp(proj.xy + (shadow_poisson_disk[index] / vec2(TG_CASCADED_SHADOW_MAP_SIZE_F)), vec2(0.0), vec2(1.0));
            //    float depth = texture(u_shadow_maps[shadow_map_index], uv).x;
            //    if (proj.z - TG_SHADOW_BIAS >= depth)
            //    {
            //        shadow_sum += 1.0;
            //    }
	        //}
            //shadow = 1.0 - clamp(shadow_sum / float(TG_SHADOW_SAMPLE_COUNT), 0.0, 1.0);
            
            //if (secondary != -1)
            //{
            //    vec4 secondary_position_lightspace = u_lightspace_matrices[secondary] * texture(u_position, v_uv);
            //    vec3 secondary_proj = (secondary_position_lightspace.xyz / vec3(secondary_position_lightspace.w)) * vec3(0.5, 0.5, 1.0) + vec3(0.5, 0.5, 0.0);
            //    float secondary_shadow = 1.0;
            //    for (int i = 0; i < TG_SHADOW_SAMPLE_COUNT; i++)
            //    {
		    //        int index = int(16.0 * tg_random(floor(position * TG_CASCADED_SHADOW_MAP_SIZE_F), i)) % 16;
            //        vec2 uv = clamp(secondary_proj.xy + shadow_poisson_disk[index] / vec2(TG_CASCADED_SHADOW_MAP_SIZE_F), vec2(0.0), vec2(1.0));
            //        float depth = texture(u_shadow_maps[secondary], uv).x;
            //        secondary_shadow += secondary_proj.z - TG_SHADOW_BIAS < depth ? 1.0 : 0.0;
	        //    }
            //    secondary_shadow = clamp(secondary_shadow / float(TG_SHADOW_SAMPLE_COUNT), 0.0, 1.0);
            //    shadow = (1.0 - t) * shadow + t * secondary_shadow;
            //}



            shadow = proj.z - TG_SHADOW_BIAS < texture(u_shadow_maps[shadow_map_index], proj.xy).x ? 1.0 : 0.0;
            //if (secondary != -1)
            //{
            //    vec4 secondary_position_lightspace = u_lightspace_matrices[secondary] * texture(u_position, v_uv);
            //    vec3 secondary_proj = (secondary_position_lightspace.xyz / vec3(secondary_position_lightspace.w)) * vec3(0.5, 0.5, 1.0) + vec3(0.5, 0.5, 0.0);
            //    float secondary_shadow = secondary_proj.z - TG_SHADOW_BIAS < texture(u_shadow_maps[secondary], secondary_proj.xy).x ? 1.0 : 0.0;
            //    shadow = (1.0 - t) * shadow + t * secondary_shadow;
            //}
        }
    }

    return shadow;
}

vec3 tg_shade(vec3 n, vec3 v, vec3 l, vec3 diffuse_albedo, vec3 specular_albedo, float metallic, float roughness, vec3 radiance)
{
    vec3 h = normalize(v + l);

    float h_dot_n = dot(h, n);
    float h_dot_v = dot(h, v);
    float l_dot_n = dot(n, l);
    float n_dot_v = dot(n, v);

    float clamped_h_dot_n = clamp(h_dot_n, 0.0, 1.0);
    float clamped_h_dot_v = clamp(h_dot_v, 0.0, 1.0);
    float clamped_l_dot_n = clamp(l_dot_n, 0.0, 1.0);
    float clamped_n_dot_v = clamp(n_dot_v, 0.0, 1.0);

    float d = tg_distribution_ggx(clamped_h_dot_n, roughness);
    float f = tg_fresnel_schlick(clamped_n_dot_v, roughness);
    float g = tg_geometry_smith(clamped_n_dot_v, clamped_l_dot_n, roughness);
    float dfg = d * f * g;
    float denominator = 4.0 * clamped_n_dot_v * clamped_l_dot_n;
    vec3 specular = specular_albedo * (dfg / max(denominator, 0.001));

    vec3 diffuse = (1.0 - f) * (1.0 - metallic) * diffuse_albedo / TG_PI;

    float shadow = tg_shadow_factor();
    return (diffuse + specular) * radiance * clamped_l_dot_n * shadow;
}

void main()
{
    vec3  position     = texture(u_position, v_uv).xyz;
    vec3  normal       = texture(u_normal, v_uv).xyz;
    vec3  albedo       = texture(u_albedo, v_uv).xyz;
    float metallic     = texture(u_metallic_roughness_ao, v_uv).x;
    float roughness    = texture(u_metallic_roughness_ao, v_uv).y;
    float ao           = texture(u_ssao, v_uv).x * texture(u_metallic_roughness_ao, v_uv).z;

    vec4 sky_color = 0.7 * u_directional_light_colors[0];
    if (dot(normal, normal) < 0.5) // TODO: sky
    {
        out_color = sky_color;
    }
    else
    {
        vec3 n = normalize(normal);
        vec3 v = normalize(u_camera_position - position);
        vec3 specular_albedo = mix(vec3(0.04), albedo, metallic);

        vec3 lo = vec3(0.0);

        for (int i = 0; i < u_directional_light_count; i++)
        {
            vec3 l = normalize(-u_directional_light_directions[i].xyz);

            lo += tg_shade(n, v, l, albedo, specular_albedo, metallic, roughness, u_directional_light_colors[i].xyz);
        }

        for (int i = 0; i < u_point_light_count; i++)
        {
            vec3 l = normalize(u_point_light_positions[i].xyz - position);

            float distance = length(u_point_light_positions[i].xyz - position);
            float attenuation = 1.0 / (distance * distance);
            if (attenuation > 0.0)
            {
                vec3 radiance = u_point_light_colors[i].xyz * attenuation;
                lo += tg_shade(n, v, l, albedo, specular_albedo, metallic, roughness, radiance);
            }
        }

        vec3 ambient = vec3(0.1) * albedo * ao;
        //ambient = vec3(0.0);
        vec3 color = ambient + lo;

        //vec3  d = u_camera_position - position;
        //float t = clamp(sqrt(dot(d, d)) / 1024.0f, 0.0, 1.0);
        //t = pow(t, 1.5);
        //color = mix(color, sky_color.xyz, t);

        out_color = vec4(color, 1.0);
    }
}
