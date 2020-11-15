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

#ifndef TG_VULKAN_ATMOSPHERE_SHADERS_H
#define TG_VULKAN_ATMOSPHERE_SHADERS_H

const char p_atmosphere_vertex_shader[] =
    "#version 450\r\n"
	"\r\n"
    "layout(location = 0) in vec2 vertex;\r\n"
	"\r\n"
    "void main()\r\n"
	"{\r\n"
	"    gl_Position = vec4(vertex, 0.0, 1.0);\r\n"
    "}\r\n";

const char p_atmosphere_geometry_shader[] =
    "#version 450\r\n"
	"\r\n"
    "layout(triangles) in;\r\n"
	"\r\n"
    "layout(set = 0, binding = 0) uniform ubo\r\n"
	"{\r\n"
	"    int layer;\r\n"
	"};\r\n"
	"\r\n"
    "layout(triangle_strip, max_vertices = 3) out;\r\n"
    "\r\n"
	"void main()\r\n"
	"{\r\n"
    "    gl_Position = gl_in[0].gl_Position;\r\n"
    "    gl_Layer = layer;\r\n"
    "    EmitVertex();\r\n"
    "    gl_Position = gl_in[1].gl_Position;\r\n"
    "    gl_Layer = layer;\r\n"
    "    EmitVertex();\r\n"
    "    gl_Position = gl_in[2].gl_Position;\r\n"
    "    gl_Layer = layer;\r\n"
    "    EmitVertex();\r\n"
    "    EndPrimitive();\r\n"
    "}\r\n";

const char p_atmosphere_compute_transmittance_shader[] =
	"layout(location = 0) out vec3 transmittance;\r\n"
	"\r\n"
	"void main()\r\n"
	"{\r\n"
	"    transmittance = tg_compute_transmittance_to_top_atmosphere_boundary_texture(ATMOSPHERE, gl_FragCoord.xy);\r\n"
	"}\r\n";

const char p_atmosphere_compute_direct_irradiance_shader[] =
	"layout(set = 0, binding = 0) uniform sampler2D transmittance_texture;\r\n"
	"\r\n"
	"layout(location = 0) out vec3 delta_irradiance;\r\n"
	"layout(location = 1) out vec3 irradiance;\r\n"
	"\r\n"
	"void main()\r\n"
	"{\r\n"
	"    delta_irradiance = tg_compute_direct_irradiance_texture(ATMOSPHERE, transmittance_texture, gl_FragCoord.xy);\r\n"
	"    irradiance = vec3(0.0);\r\n"
	"}\r\n";

const char p_atmosphere_compute_single_scattering_shader[] =
	"layout(set = 0, binding = 1) uniform ubo\r\n"
	"{\r\n"
	"    mat4 luminance_from_radiance;\r\n" //TODO: mat3( m4[0].xyz, m4[1].xyz, m4[2].xyz)
	"    int layer;\r\n"
	"};\r\n"
	"\r\n"
	"layout(set = 0, binding = 2) uniform sampler2D transmittance_texture;\r\n"
	"\r\n"
	"layout(location = 0) out vec3 delta_rayleigh;\r\n"
	"layout(location = 1) out vec3 delta_mie;\r\n"
	"layout(location = 2) out vec4 scattering;\r\n"
	"layout(location = 3) out vec3 single_mie_scattering;\r\n"
	"\r\n"
	"void main()\r\n"
	"{\r\n"
	"    mat3 lum_from_rad = mat3(luminance_from_radiance);"
	"    tg_compute_single_scattering_texture(ATMOSPHERE, transmittance_texture, vec3(gl_FragCoord.xy, layer + 0.5), delta_rayleigh, delta_mie);\r\n"
	"    scattering = vec4(lum_from_rad * delta_rayleigh.rgb, (lum_from_rad * delta_mie).r);\r\n"
	"    single_mie_scattering = lum_from_rad * delta_mie;\r\n"
	"}\r\n";

const char p_atmosphere_compute_single_scattering_shader_no_single_mie_scattering_texture[] =
	"layout(set = 0, binding = 1) uniform ubo\r\n"
	"{\r\n"
	"    mat4 luminance_from_radiance;\r\n" //TODO: mat3( m4[0].xyz, m4[1].xyz, m4[2].xyz)
	"    int layer;\r\n"
	"};\r\n"
	"\r\n"
	"layout(set = 0, binding = 2) uniform sampler2D transmittance_texture;\r\n"
	"\r\n"
	"layout(location = 0) out vec3 delta_rayleigh;\r\n"
	"layout(location = 1) out vec3 delta_mie;\r\n"
	"layout(location = 2) out vec4 scattering;\r\n"
	"\r\n"
	"void main()\r\n"
	"{\r\n"
	"    mat3 lum_from_rad = mat3(luminance_from_radiance);"
	"    tg_compute_single_scattering_texture(ATMOSPHERE, transmittance_texture, vec3(gl_FragCoord.xy, layer + 0.5), delta_rayleigh, delta_mie);\r\n"
	"    scattering = vec4(lum_from_rad * delta_rayleigh.rgb, (lum_from_rad * delta_mie).r);\r\n"
	"}\r\n";

const char p_atmosphere_compute_scattering_density_shader[] =
	"layout(set = 0, binding = 1) uniform ubo\r\n"
	"{\r\n"
	"    int scattering_order;\r\n"
	"    int layer;\r\n"
	"};\r\n"
	"\r\n"
	"layout(set = 0, binding = 2) uniform sampler2D transmittance_texture;\r\n"
	"layout(set = 0, binding = 3) uniform sampler3D single_rayleigh_scattering_texture;\r\n"
	"layout(set = 0, binding = 4) uniform sampler3D single_mie_scattering_texture;\r\n"
	"layout(set = 0, binding = 5) uniform sampler3D multiple_scattering_texture;\r\n"
	"layout(set = 0, binding = 6) uniform sampler2D irradiance_texture;\r\n"
	"\r\n"
	"layout(location = 0) out vec3 scattering_density;\r\n"
	"\r\n"
	"void main()\r\n"
	"{\r\n"
	"    scattering_density = tg_compute_scattering_density_texture(\r\n"
	"        ATMOSPHERE, transmittance_texture, single_rayleigh_scattering_texture,\r\n"
	"        single_mie_scattering_texture, multiple_scattering_texture,\r\n"
	"        irradiance_texture, vec3(gl_FragCoord.xy, layer + 0.5),\r\n"
	"        scattering_order\r\n"
	"    );\r\n"
	"}\r\n";

const char p_atmosphere_compute_indirect_irradiance_shader[] =
	"layout(set = 0, binding = 0) uniform ubo\r\n"
	"{\r\n"
	"    mat4 luminance_from_radiance;\r\n"
	"    int scattering_order;\r\n"
	"};\r\n"
	"\r\n"
	"layout(set = 0, binding = 1) uniform sampler3D single_rayleigh_scattering_texture;\r\n"
	"layout(set = 0, binding = 2) uniform sampler3D single_mie_scattering_texture;\r\n"
	"layout(set = 0, binding = 3) uniform sampler3D multiple_scattering_texture;\r\n"
	"\r\n"
	"layout(location = 0) out vec3 delta_irradiance;\r\n"
	"layout(location = 1) out vec3 irradiance;\r\n"
	"\r\n"
	"void main()\r\n"
	"{\r\n"
	"    mat3 lum_from_rad = mat3(luminance_from_radiance);"
	"    delta_irradiance = tg_compute_indirect_irradiance_texture(\r\n"
	"        ATMOSPHERE, single_rayleigh_scattering_texture,\r\n"
	"        single_mie_scattering_texture, multiple_scattering_texture,\r\n"
	"        gl_FragCoord.xy, scattering_order\r\n"
	"    );\r\n"
	"    irradiance = lum_from_rad * delta_irradiance;\r\n"
	"}\r\n";

const char p_atmosphere_compute_multiple_scattering_shader[] =
	"layout(set = 0, binding = 1) uniform ubo\r\n"
	"{\r\n"
	"    mat4 luminance_from_radiance;\r\n"
	"    int layer;\r\n"
	"};\r\n"
	"\r\n"
	"layout(set = 0, binding = 2) uniform sampler2D transmittance_texture;\r\n"
	"layout(set = 0, binding = 3) uniform sampler3D scattering_density_texture;\r\n"
	"\r\n"
	"layout(location = 0) out vec3 delta_multiple_scattering;\r\n"
	"layout(location = 1) out vec4 scattering;\r\n"
	"\r\n"
	"void main()\r\n"
	"{\r\n"
	"    mat3 lum_from_rad = mat3(luminance_from_radiance);"
	"    float nu;\r\n"
	"    delta_multiple_scattering = tg_compute_multiple_scattering_texture(\r\n"
	"        ATMOSPHERE, transmittance_texture, scattering_density_texture,\r\n"
	"        vec3(gl_FragCoord.xy, layer + 0.5), nu\r\n"
	"    );\r\n"
	"    scattering = vec4(lum_from_rad * delta_multiple_scattering.rgb / tg_rayleigh_phase_function(nu), 0.0);\r\n"
	"}\r\n";

const char p_atmosphere_shader[] =
    "layout(set = 0, binding = 0) uniform sampler2D transmittance_texture;\r\n"
    "layout(set = 0, binding = 1) uniform sampler3D scattering_texture;\r\n"
    "layout(set = 0, binding = 2) uniform sampler3D single_mie_scattering_texture;\r\n"
    "layout(set = 0, binding = 3) uniform sampler2D irradiance_texture;\r\n"
	"\r\n"
    "#ifdef TG_RADIANCE_API_ENABLED\r\n"
	"\r\n"
    "tg_radiance_spectrum tg_get_solar_radiance()\r\n"
	"{\r\n"
	"    return ATMOSPHERE.solar_irradiance / (TG_PI * ATMOSPHERE.sun_angular_radius * ATMOSPHERE.sun_angular_radius);\r\n"
    "}\r\n"
	"\r\n"
    "#endif\r\n"
	"\r\n"
    "tg_luminance3 tg_get_solar_luminance()\r\n"
	"{\r\n"
	"    return ATMOSPHERE.solar_irradiance / (TG_PI * ATMOSPHERE.sun_angular_radius * ATMOSPHERE.sun_angular_radius) * TG_SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;\r\n"
    "}\r\n"
	"\r\n"
    "tg_luminance3 tg_get_sky_luminance(\r\n"
	"    TG_IN(tg_atmosphere_parameters)         atmosphere,\r\n"
	"    TG_IN(tg_transmittance_texture)         transmittance_texture,\r\n"
	"    TG_IN(tg_reduced_scattering_texture)    scattering_texture,\r\n"
	"    TG_IN(tg_reduced_scattering_texture)    single_mie_scattering_texture,\r\n"
	"    tg_position                             camera,\r\n"
	"    TG_IN(tg_direction)                     view_ray,\r\n"
	"    tg_length                               shadow_length,\r\n"
	"    TG_IN(tg_direction)                     sun_direction,\r\n"
	"    TG_OUT(tg_dimensionless_spectrum)       transmittance\r\n"
	")\r\n"
	"{\r\n"
	"    return tg_get_sky_radiance(\r\n"
	"        atmosphere,\r\n"
	"        transmittance_texture,\r\n"
	"        scattering_texture,\r\n"
	"        single_mie_scattering_texture,\r\n"
	"        camera,\r\n"
	"        view_ray,\r\n"
	"        shadow_length,\r\n"
	"        sun_direction,\r\n"
	"        transmittance\r\n"
	"    ) * TG_SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;\r\n"
    "}\r\n"
	"\r\n"
    "tg_luminance3 tg_get_sky_luminance_to_point(\r\n"
	"    TG_IN(tg_atmosphere_parameters)         atmosphere,\r\n"
	"    TG_IN(tg_transmittance_texture)         transmittance_texture,\r\n"
	"    TG_IN(tg_reduced_scattering_texture)    scattering_texture,\r\n"
	"    TG_IN(tg_reduced_scattering_texture)    single_mie_scattering_texture,\r\n"
	"    tg_position                             camera,\r\n"
	"    TG_IN(tg_position)                      point,\r\n"
	"    tg_length                               shadow_length,\r\n"
	"    TG_IN(tg_direction)                     sun_direction,\r\n"
	"    TG_OUT(tg_dimensionless_spectrum)       transmittance\r\n"
")\r\n"
	"{\r\n"
	"    return tg_get_sky_radiance_to_point(\r\n"
	"        atmosphere,\r\n"
	"        transmittance_texture,\r\n"
	"        scattering_texture,\r\n"
	"        single_mie_scattering_texture,\r\n"
	"        camera,\r\n"
	"        point,\r\n"
	"        shadow_length,\r\n"
	"        sun_direction,\r\n"
	"        transmittance\r\n"
	"    ) * TG_SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;\r\n"
    "}\r\n"
	"\r\n"
	"#ifdef TG_USE_LUMINANCE\r\n"
	"#define tg_get_solar_radiance            tg_get_solar_luminance\r\n"
	"#define tg_get_sky_radiance              tg_get_sky_luminance\r\n"
	"#define tg_get_sky_radiance_to_point     tg_get_sky_luminance_to_point\r\n"
	"#endif\r\n";

#endif