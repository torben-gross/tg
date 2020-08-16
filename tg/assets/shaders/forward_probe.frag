#version 450

layout(location = 0) in vec4    v_position;
layout(location = 1) in vec3    v_normal;
layout(location = 2) in vec2    v_uv;
//layout(location = 3) in mat3    v_tbn; // TODO: this will be needed for normal mapping

layout(location = 0) out vec4    out_color;

layout(set = 0, binding = 2) uniform samplerCube cubemap;

void main()
{
    float brightness = texture(cubemap, normalize(v_normal)).x;
    out_color = vec4(vec3(brightness), 1.0);
}
