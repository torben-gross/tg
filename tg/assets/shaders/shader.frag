#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 v_color;
layout(location = 1) in vec2 v_uv;

layout(set = 0, binding = 1) uniform sampler2D tex;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(tex, v_uv);
}
