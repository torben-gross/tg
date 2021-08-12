#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;

layout(set = 0, binding = 9) uniform atmosphere_ubo
{
    mat4 model_from_view;
    mat4 view_from_clip;
};

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec3 v_view_ray;

void main()
{
    gl_Position = vec4(in_position, 0.0, 1.0);
    v_uv = in_uv;
	v_view_ray = (model_from_view * vec4((view_from_clip * vec4(in_position, 0.0, 1.0)).xyz, 0.0)).xyz;
}
