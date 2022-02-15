#version 450

#include "shaders/common.inc"

layout(location = 0) out v4 out_color;

void main()
{
	out_color = v4(1.0, 1.0, 0.0, 1.0);
}
