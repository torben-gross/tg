#define f32    float
#define i32    int
#define u32    uint

#define v2     vec2
#define v2i    ivec2
#define v2u    uvec2
#define v3     vec3
#define v3i    ivec3
#define v3u    uvec3
#define v4     vec4
#define v4i    ivec4
#define v4u    uvec4

#define m2     mat2
#define m3     mat3
#define m4     mat4

#define TG_MAX_DIRECTIONAL_LIGHTS            512
#define TG_MAX_POINT_LIGHTS                  512
#define TG_PI                                3.14159265358979323846

#define TG_TONEMAP_SUN_RADIANCE(radiance)    (2.0 * pow(vec3(1.0) - exp(-radiance * exposure), vec3(1.0 / 2.2)))



layout(location = 0) in v2 v_uv;
layout(location = 1) in v3 v_view_ray;



layout(set = 0, binding = 4) uniform sampler2D out_normal_3xf8_metallic_1xf8;
layout(set = 0, binding = 5) uniform sampler2D u_albedo_3xf8_roughness_1xf8;
layout(set = 0, binding = 6) uniform sampler2D u_depth;
    
layout(set = 0, binding = 7) uniform ubo
{
    v4                               u_camera_position;
    v4                               u_sun_direction;
    u32                              u_directional_light_count;
    u32                              u_point_light_count;
    m4                               u_ivp;
    v4[TG_MAX_DIRECTIONAL_LIGHTS]    u_directional_light_directions;
    v4[TG_MAX_DIRECTIONAL_LIGHTS]    u_directional_light_colors;
    v4[TG_MAX_POINT_LIGHTS]          u_point_light_positions;
    v4[TG_MAX_POINT_LIGHTS]          u_point_light_colors;
};

layout(set = 0, binding = 8) uniform atmosphere_ubo
{
    float    exposure;
    float    pad0;
    float    pad1;
    float    pad2;
    vec4     earth_center;
    vec2     sun_size;
};



layout(location = 0) out vec4 out_color;



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

    return (diffuse + specular) * radiance * clamped_l_dot_n;
}

vec3 tg_shade_with_sun(vec3 position, vec3 n, vec3 v, vec3 l, vec3 diffuse_albedo, vec3 specular_albedo, float metallic, float roughness)
{
    vec3 view_direction = normalize(v_view_ray);

    vec3 pos_y = vec3(0.0, position.y, 0.0);
    vec3 earth_y = vec3(0.0, earth_center.y, 0.0);
    vec3 cam_y = vec3(0.0, u_camera_position.y, 0.0);

    tg_length r = length(pos_y - earth_y);
    tg_number mu_s = dot(pos_y - earth_y, -u_sun_direction.xyz) / r;

    vec3 sky_irradiance = tg_get_irradiance(ATMOSPHERE, irradiance_texture, r, mu_s);
    vec3 sun_irradiance = ATMOSPHERE.solar_irradiance * tg_get_transmittance_to_sun(ATMOSPHERE, transmittance_texture, r, mu_s);

    #ifdef TG_USE_LUMINANCE
    sky_irradiance *= SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
    sun_irradiance *= SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;
    #endif

    vec3 transmittance;
    vec3 in_scatter = tg_get_sky_radiance_to_point(
        ATMOSPHERE,
        transmittance_texture,
        scattering_texture,
        single_mie_scattering_texture,
        cam_y - earth_y,
        pos_y - earth_y,
        0.0,
        -u_sun_direction.xyz,
        transmittance
    );
    vec3 radiance = transmittance * (sun_irradiance + sky_irradiance);
    radiance = TG_TONEMAP_SUN_RADIANCE(radiance);

    return tg_shade(n, v, l, diffuse_albedo, specular_albedo, metallic, roughness, radiance) + in_scatter;
}

void tg_unpack(out v3 p, out v3 n, out v3 a, out f32 m, out f32 r, out f32 d)
{
    d = texture(u_depth, v_uv).x;
    v4 screen = v4(v_uv.x * 2.0 - 1.0, v_uv.y * 2.0 - 1.0, d, 1.0);
    v4 world = u_ivp * screen;
    p = v3(world.x / world.w, world.y / world.w, world.z / world.w);

    v4 v0 = texture(out_normal_3xf8_metallic_1xf8, v_uv);
    v4 v1 = texture(u_albedo_3xf8_roughness_1xf8, v_uv);

    n = v0.xyz;
    a = v1.xyz;
    m = v0.w;
    r = v1.w;
}

void main()
{
    vec3 position, normal, albedo;
    float metallic, roughness, depth;
    tg_unpack(position, normal, albedo, metallic, roughness, depth);

    if (depth.x == 1.0)
    {
	    vec3 view_direction = normalize(v_view_ray);
	    vec3 transmittance;
	    vec3 radiance = tg_get_sky_radiance(
            ATMOSPHERE,
            transmittance_texture,
            scattering_texture,
            single_mie_scattering_texture,
	        vec3(0.0, u_camera_position.y, 0.0) - vec3(0.0, earth_center.y, 0.0), // Note: This keeps the atmosphere always xz aligned
	        view_direction,
	        0.0,
	        -u_sun_direction.xyz,
	        transmittance
	    );
	    if (dot(view_direction, -u_sun_direction.xyz) > sun_size.y)
	    {
	        radiance = radiance + transmittance * tg_get_solar_radiance();
	    }
        out_color = vec4(TG_TONEMAP_SUN_RADIANCE(radiance), 1.0);
    }
    else
    {
        vec3 n = normalize(normal);
        vec3 v = normalize(u_camera_position.xyz - position);
        vec3 specular_albedo = mix(vec3(0.04), albedo, metallic);

        vec3 lo = vec3(0.0);

        lo += tg_shade_with_sun(position, n, v, -u_sun_direction.xyz, albedo, specular_albedo, metallic, roughness);

        for (int i = 0; i < u_directional_light_count; i++)
        {
            vec3 l = normalize(-u_directional_light_directions[i].xyz);
            vec3 radiance = u_directional_light_colors[i].xyz;

            lo += tg_shade(n, v, l, albedo, specular_albedo, metallic, roughness, radiance);
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

        vec3 ambient = vec3(0.1) * albedo;// * ao;
        //ambient = vec3(0.0);
        vec3 color = ambient + lo;

        //vec3  d = u_camera_position.xyz - position;
        //float t = clamp(sqrt(dot(d, d)) / 1024.0f, 0.0, 1.0);
        //t = pow(t, 1.5);
        //color = mix(color, sky_color.xyz, t);

        out_color = vec4(color, 1.0);
    }
}
