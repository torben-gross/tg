#version 450

layout(location = 0) in vec2 v_uv;

layout(set = 0, binding = 0) uniform sampler2D present_texture;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(present_texture, v_uv);
}
