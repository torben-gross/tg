#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2    v_uv;

layout(set = 0, binding = 0) uniform    sampler2D present_texture;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(present_texture, v_uv);
}
