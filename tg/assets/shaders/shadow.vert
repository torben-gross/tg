#version 450

layout(location = 0) in vec3 in_position;

layout(set = 0, binding = 0) uniform model
{
    mat4 u_model;
};

layout(set = 0, binding = 1) uniform lightspace_matrix
{
    mat4 u_lightspace_matrix;
};

void main()
{
    gl_Position = u_lightspace_matrix * u_model * vec4(in_position, 1.0);
}
