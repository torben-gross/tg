#version 450

layout(location = 0) in vec4    v_position;
layout(location = 1) in vec3    v_normal;
layout(location = 2) in vec2    v_uv;
//layout(location = 3) in mat3    v_tbn; // TODO: this will be needed for normal mapping

layout(set = 0, binding = 2) uniform sampler2D tex;

layout(location = 0) out vec4    out_position;
layout(location = 1) out vec4    out_normal;
layout(location = 2) out vec4    out_albedo;
layout(location = 3) out vec4    out_metallic_roughness_ao;

void main()
{
    out_position                   = v_position;
    out_normal                     = vec4(normalize(v_normal), 1.0);
    out_albedo                     = texture(tex, v_uv);
    out_metallic_roughness_ao.x    = 0.0f;
    out_metallic_roughness_ao.y    = 1.0f;
    out_metallic_roughness_ao.z    = 1.0f;
}
