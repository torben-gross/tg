#version 450

#include "shaders/common.inc"

layout(location = 0) in vec3          test_v0;
layout(location = 1) flat in uvec2    test_v1;
//layout(location = 2) flat in mat4     test_v2;
//layout(location = 6) flat in int      test_v3[2];
layout(location = 8) flat in ivec2    test_v4;

layout(location = 9) in vec3          test_v5; #instance

layout(set = 1, binding = 2) uniform test_uniform_block
{
	bool      test_scaler0;
	int       test_scaler1;
	uint      test_scaler2;
	float     test_scaler3;
	double    test_scaler4;
	bvec3     test_vector0;
	ivec3     test_vector1;
	uvec3     test_vector2;
	vec3      test_vector3;
	dvec3     test_vector4;
	mat4      test_mat0;
	uint      test_array0[3];
};

layout(set = 4, binding = 5) uniform sampler2D    test_sampler_2d_array[6];
layout(set = 7, binding = 8) uniform sampler3D    test_sampler_3d[9];

layout(location = 0) out vec4    test_output0;

void main()
{
	test_output0 = vec4(0.0);
}
