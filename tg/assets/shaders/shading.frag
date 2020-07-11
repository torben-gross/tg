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

layout(set = 0, binding = 6) uniform shadows
{
    vec4[1] u_shadow_distances;
};

layout(set = 0, binding = 7) uniform lightspace_matrix0
{
    mat4 u_lightspace_matrix0;
};

layout(set = 0, binding = 8) uniform lightspace_matrix1
{
    mat4 u_lightspace_matrix1;
};

layout(set = 0, binding = 9) uniform lightspace_matrix2
{
    mat4 u_lightspace_matrix2;
};

layout(set = 0, binding = 10) uniform    sampler2D u_shadow_map0;
layout(set = 0, binding = 11) uniform    sampler2D u_shadow_map1;
layout(set = 0, binding = 12) uniform    sampler2D u_shadow_map2;



layout(location = 0) out vec4 out_color;



const float pi = 3.14159265358979323846;
const float shadow_sample_count = 4;
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

float random(vec3 seed, int i){
    vec4 seed4 = vec4(seed, float(i));
    float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(dot_product) * 43758.5453);
}

float shadow_mapping(vec3 position, vec4 position_lightspace)
{
    float shadow = 1.0;

    vec3 proj = (position_lightspace.xyz / vec3(position_lightspace.w)) * vec3(0.5, 0.5, 1.0) + vec3(0.5, 0.5, 0.0);
    if (proj.x > 0.0 && proj.x < 1.0 &&
        proj.y > 0.0 && proj.y < 1.0 &&
        proj.z > 0.0 && proj.z < 1.0
    )
    {
        float bias = 0.025;

        //for (int i = 0; i < shadow_sample_count; i++)
        //{
		//    int index = int(16.0 * random(floor(position.xyz * 1024.0), i)) % 16;
        //    vec2 uv = clamp(proj.xy + shadow_poisson_disk[index] / vec2(1024.0), vec2(0.0), vec2(1.0));
        //    float pcf_depth = texture(u_shadow_map0, uv).x;
        //    shadow += proj.z - bias < pcf_depth ? 1.0 : 0.0;
	    //}
        //shadow = clamp(shadow / float(shadow_sample_count), 0.0, 1.0);
        
        float d = distance(position, u_camera_position);
        if (d >= u_shadow_distances[0].x && d < u_shadow_distances[0].y)
        {
            return proj.z - bias < texture(u_shadow_map0, proj.xy).x ? 1.0 : 0.0;
        }
        else if (d >= u_shadow_distances[0].y && d < u_shadow_distances[0].z)
        {
            return proj.z - bias < texture(u_shadow_map1, proj.xy).x ? 1.0 : 0.0;
        }
        else if (d >= u_shadow_distances[0].z)
        {
            if (proj.z - bias < texture(u_shadow_map2, proj.xy).x)
            {
                return 1.0;
            }
            else
            {
                return 0.0f;//(d - u_shadow_distances[0].z) / (u_shadow_distances[0].w - u_shadow_distances[0].z);
            }
        }
        else
        {
            return 1.0;
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
    vec4  position_lightspace;
    
    float d = distance(position, u_camera_position);
    if (d >= u_shadow_distances[0].x && d < u_shadow_distances[0].y)
    {
        position_lightspace = u_lightspace_matrix0 * texture(u_position, v_uv);
    }
    else if (d >= u_shadow_distances[0].y && d < u_shadow_distances[0].z)
    {
        position_lightspace = u_lightspace_matrix1 * texture(u_position, v_uv);
    }
    else
    {
        position_lightspace = u_lightspace_matrix2 * texture(u_position, v_uv);
    }

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
            float shadow = i == 0 ? shadow_mapping(position, position_lightspace) : 1.0;
            lo += (k_diffuse * albedo / pi + specular) * radiance * ndl * shadow;
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
        vec3 color   = ambient + lo;

        color = color / (color + vec3(1.0));
        color = pow(color, vec3(1.0 / 2.2));

        vec3  d = u_camera_position - position;
        float t = clamp(sqrt(dot(d, d)) / 256.0f, 0.0, 1.0);
        color   = mix(color, sky_color.xyz, t);

        out_color = vec4(color, 1.0);
    }
}
