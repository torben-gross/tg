#version 450

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;

layout(set = 0, binding = 2, r32ui) uniform uimage3D u_albedo_r;
layout(set = 0, binding = 3, r32ui) uniform uimage3D u_albedo_g;
layout(set = 0, binding = 4, r32ui) uniform uimage3D u_albedo_b;
layout(set = 0, binding = 5, r32ui) uniform uimage3D u_albedo_a;
layout(set = 0, binding = 6, r32ui) uniform uimage3D u_metallic;
layout(set = 0, binding = 7, r32ui) uniform uimage3D u_roughness;
layout(set = 0, binding = 8, r32ui) uniform uimage3D u_ao;
layout(set = 0, binding = 9, r32ui) uniform uimage3D u_count;

void main()
{
    vec3 image_size = vec3(imageSize(u_albedo_r));
    
    float x = floor(image_size.x * ((v_position.x + 1.0) / 2.0));
    float y = floor(image_size.y * (1.0 - ((v_position.y + 1.0) / 2.0)));
    float z = floor(image_size.z * (1.0 - v_position.z));
    ivec3 index = ivec3(x, y, z);
    
    vec4 albedo = vec4(normalize(v_normal) * vec3(0.5) + vec3(0.5), 1.0);
    float metallic = 0.1;
    float roughness = 0.9;
    float ao = 1.0;

    imageAtomicAdd(u_albedo_r,  index, uint(albedo.r  * 255.0));
    imageAtomicAdd(u_albedo_g,  index, uint(albedo.g  * 255.0));
    imageAtomicAdd(u_albedo_b,  index, uint(albedo.b  * 255.0));
    imageAtomicAdd(u_albedo_a,  index, uint(albedo.a  * 255.0));
    imageAtomicAdd(u_metallic,  index, uint(metallic  * 255.0));
    imageAtomicAdd(u_roughness, index, uint(roughness * 255.0));
    imageAtomicAdd(u_ao,        index, uint(ao        * 255.0));
    imageAtomicAdd(u_count,     index, 1);
}
