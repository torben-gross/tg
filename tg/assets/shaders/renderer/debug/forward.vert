#version 450

layout(location = 0) in vec3    in_position;

layout(set = 0, binding = 0) uniform model
{
    mat4    u_model;
    vec4    u_color;
};

layout(set = 0, binding = 1) uniform view_projection
{
    mat4    u_view;
    mat4    u_projection;
};

void main()
{
    gl_Position = u_projection * u_view * u_model * vec4(in_position, 1.0);
}
