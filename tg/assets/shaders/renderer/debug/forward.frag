#version 450

layout(location = 0) out vec4    out_color;

layout(set = 0, binding = 0) uniform model
{
    mat4    u_model;
    vec4    u_color;
};

void main()
{
    out_color = u_color;
}
