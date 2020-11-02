/**
 * Copyright (c) 2017 Eric Bruneton
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TG_VULKAN_ATMOSPHERE_DEMO_SHADERS_H
#define TG_VULKAN_ATMOSPHERE_DEMO_SHADERS_H

static const char p_atmosphere_demo_vertex_shader[] =
	"#version 450\r\n"
	"\r\n"
	"layout(location = 0) in vec2 vertex;\r\n"
	"\r\n"
	"layout(set = 0, binding = 4) uniform ubo\r\n"
	"{\r\n"
	"    mat4 model_from_view;\r\n"
	"    mat4 view_from_clip;\r\n"
	"};\r\n"
	"\r\n"
	"layout(location = 0) out vec3 view_ray;\r\n"
	"\r\n"
	"void main()\r\n"
	"{\r\n"
	"    vec4 vert = vec4(vertex.x, vertex.y, 0.0, 1.0);\r\n"
	"    view_ray = (model_from_view * vec4((view_from_clip * vert).xyz, 0.0)).xyz;\r\n"
	"    gl_Position = vert;\r\n"
	"}\r\n";

static const char p_atmosphere_demo_fragment_shader[] =
	"layout(location = 0) in vec3 view_ray;\r\n"
	"\r\n"
	"layout(set = 0, binding = 5) uniform ubo\r\n"
	"{\r\n"
	"    vec4 camera;\r\n"
	"    float exposure;\r\n"
	"    float pad0;\r\n"
	"    float pad1;\r\n"
	"    float pad2;\r\n"
	"    vec4 white_point;\r\n"
	"    vec4 earth_center;\r\n"
	"    vec4 sun_direction;\r\n"
	"    vec2 sun_size;\r\n"
	"};\r\n"
	"\r\n"
	"layout(location = 0) out vec4 color;\r\n"
	"\r\n"
	"#ifdef TG_USE_LUMINANCE\r\n"
	"#define tg_get_solar_radiance tg_get_solar_luminance\r\n"
	"#define tg_get_sky_radiance tg_get_sky_luminance\r\n"
	"#endif\r\n"
	"\r\n"
	"void main()\r\n"
	"{\r\n"
	"    vec3 view_direction = normalize(view_ray);\r\n"
	"    vec3 transmittance;\r\n"
	"    vec3 radiance = tg_get_sky_radiance(\r\n"
	"        camera.xyz - earth_center.xyz,\r\n"
	"        view_direction,\r\n"
	"        0.0,\r\n"
	"        sun_direction.xyz,\r\n"
	"        transmittance\r\n"
	"    );\r\n"
	"    if (dot(view_direction, sun_direction.xyz) > sun_size.y)\r\n"
	"    {\r\n"
	"        radiance = radiance + transmittance * tg_get_solar_radiance();\r\n"
	"    }\r\n"
	"    color.rgb = pow(vec3(1.0) - exp(-radiance / white_point.xyz * exposure), vec3(1.0 / 2.2));\r\n"
	"    color.a = 1.0;\r\n"
	"}\r\n";

#endif
