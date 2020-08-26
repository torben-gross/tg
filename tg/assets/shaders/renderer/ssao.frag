#version 450

#define TG_KERNEL_SIZE    8
#define TG_RADIUS         0.5 // TODO: uniform
#define TG_BIAS           0.001

layout(location = 0) in vec2 v_uv;

layout(set = 0, binding = 0) uniform sampler2D u_position;
layout(set = 0, binding = 1) uniform sampler2D u_normal;
layout(set = 0, binding = 2) uniform sampler2D u_noise;
layout(set = 0, binding = 3) uniform ssao_info
{
    vec4 u_kernel[64];
    vec2 u_noise_scale;
    mat4 u_view;
    mat4 u_projection;
};

layout(location = 0) out vec4 out_color;

void main()
{
    vec3 position_viewspace = (u_view * vec4(texture(u_position, v_uv).xyz, 1.0)).xyz;
    vec3 normal_viewspace = normalize((u_view * vec4(texture(u_normal, v_uv).xyz, 0.0)).xyz);
    vec3 random = normalize(vec3(texture(u_noise, v_uv * u_noise_scale).xy, 0.0));
    vec3 tangent = normalize(random - normal_viewspace * dot(random, normal_viewspace));
    vec3 bitangent = cross(normal_viewspace, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal_viewspace);

    float occlusion = 0.0;
    for (int i = 0; i < TG_KERNEL_SIZE; i++)
    {
        vec3 kernel_sample = tbn * u_kernel[i].xyz;
        kernel_sample = position_viewspace + kernel_sample * TG_RADIUS;

        vec4 offset = vec4(kernel_sample, 1.0);
        offset = u_projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float depth = (u_view * vec4(texture(u_position, clamp(offset.xy, 0.0, 1.0)).xyz, 1.0)).z;

        float range_check = smoothstep(0.0, 1.0, TG_RADIUS / abs(position_viewspace.z - depth));
        occlusion += (depth >= kernel_sample.z + TG_BIAS ? 1.0 : 0.0) * range_check;
    }
    occlusion = clamp(1.0 - (occlusion / TG_KERNEL_SIZE), 0.0, 1.0);

    out_color.x = occlusion;
}
