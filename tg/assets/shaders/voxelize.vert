#version 450

layout(location = 0) in vec3 in_position;

layout(set = 0, binding = 0) uniform model
{
    mat4 u_model;
};

layout(set = 0, binding = 1) uniform view_projection
{
    mat4 u_view_translation;
    mat4 u_view_rotation;
    mat4 u_projection;
};

layout(location = 0) out vec4 v_position;

void main()
{
    gl_Position = u_projection * (u_view_rotation * u_view_translation) * u_model * vec4(in_position, 1.0);
    v_position = u_projection * (u_view_translation) * u_model * vec4(in_position, 1.0);
}
