#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3        v_color;
layout(location = 1) in vec2        v_uv;
layout(location = 2) in flat int    v_image;

layout(set = 0, binding = 1) uniform    sampler samp;
layout(set = 0, binding = 2) uniform    texture2D textures[8];

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(sampler2D(textures[v_image], samp), v_uv);
}
