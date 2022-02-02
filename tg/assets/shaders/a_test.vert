#version 450

#include "shaders/common.inc"

layout(location = 0) in vec3    test_input0;
layout(location = 1) in uint    test_input1; #instance
//layout(location = 1) in mat4    test_input1;
//layout(location = 5) in mat3    test_input2[9];

layout(location = 0) out vec3          test_v0;
layout(location = 1) flat out uvec2    test_v1;
//layout(location = 2) flat out mat4     test_v2;
//layout(location = 6) flat out int      test_v3[2];
layout(location = 8) flat out ivec2    test_v4;

void main()
{
	gl_Position  = vec4(0.0);
	
	test_v0 = vec3(0.0);
	test_v1 = uvec2(0);
	//test_v2 = mat4(vec4(0.0), vec4(0.0), vec4(0.0), vec4(0.0));
	//test_v3[0] = 0;
	//test_v3[1] = 0;
	test_v4 = ivec2(0);
}
