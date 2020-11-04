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
 * 
 * http://www-evasion.imag.fr/Membres/Eric.Bruneton/
 */

#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "graphics/tg_atmosphere_lookup_tables.h"
#include "graphics/vulkan/tg_vulkan_atmosphere_definitions.h"
#include "graphics/vulkan/tg_vulkan_atmosphere_demo_shaders.h"
#include "graphics/vulkan/tg_vulkan_atmosphere_functions.h"
#include "graphics/vulkan/tg_vulkan_atmosphere_shaders.h"
#include "util/tg_string.h"

#define TO_COLOR_ATTACHMENT_LAYOUT(texture)           \
	tgvk_cmd_transition_image_layout(                 \
		p_command_buffer,                             \
		&(texture),                                   \
		0,                                            \
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,         \
		VK_IMAGE_LAYOUT_UNDEFINED,                    \
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,     \
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,            \
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT \
	)

#define TO_COLOR_ATTACHMENT_LAYOUT_LAYERED(texture)   \
	tgvk_cmd_transition_layered_image_layout(         \
		p_command_buffer,                             \
		&(texture),                                   \
		0,                                            \
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,         \
		VK_IMAGE_LAYOUT_UNDEFINED,                    \
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,     \
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,            \
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT \
	)

#define COLOR_ATTACHMENT_TO_SHADER_READ(texture)       \
	tgvk_cmd_transition_image_layout(                  \
		p_command_buffer,                              \
		&(texture),                                    \
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,          \
		VK_ACCESS_SHADER_READ_BIT,                     \
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,      \
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,      \
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, \
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT          \
	)

#define COLOR_ATTACHMENT_TO_SHADER_READ_LAYERED(texture) \
	tgvk_cmd_transition_layered_image_layout(            \
		p_command_buffer,                                \
		&(texture),                                      \
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,            \
		VK_ACCESS_SHADER_READ_BIT,                       \
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,        \
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,        \
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,   \
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT            \
	)

#define SHADER_READ_TO_COLOR_ATTACHMENT(texture)      \
	tgvk_cmd_transition_image_layout(                 \
		p_command_buffer,                             \
		&(texture),                                   \
		VK_ACCESS_SHADER_READ_BIT,                    \
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,         \
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,     \
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,     \
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,        \
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT \
	)

#define SHADER_READ_TO_COLOR_ATTACHMENT_LAYERED(texture) \
	tgvk_cmd_transition_layered_image_layout(            \
		p_command_buffer,                                \
		&(texture),                                      \
		VK_ACCESS_SHADER_READ_BIT,                       \
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,            \
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,        \
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,        \
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,           \
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT    \
	)



#define TG_PI64                            3.1415926
#define TG_SUN_ANGULAR_RADIUS              (0.00935 / 2.0)
#define TG_SUN_SOLIG_ANGLE                 (TG_PI64 * TG_SUN_ANGULAR_RADIUS * TG_SUN_ANGULAR_RADIUS)
#define TG_LENGTH_UNIT_IN_METERS           1000.0

#define TG_DOPSON_UNIT                     2.687e20
#define TG_MAX_OZONE_NUMBER_DENSITY        (300.0 * TG_DOPSON_UNIT / 15000.0)
#define TG_CONSTANT_SOLAR_IRRADIANCE       1.5
#define TG_BOTTOM_RADIUS                   6360000.0
#define TG_TOP_RADIUS                      6420000.0

#define TG_RAYLEIGH                        1.24062e-6
#define TG_RAYLEIGH_SCALE_HEIGHT           8000.0

#define TG_MIE_SCALE_HEIGHT                1200.0
#define TG_MIE_ANGSTROM_ALPHA              0.0
#define TG_MIE_ANGSTROM_BETA               5.328e-3
#define TG_MIE_SINGLE_SCATTERING_ALBEDO    0.9
#define TG_MIE_PHASE_FUNCTION_G            0.8

#define TG_GROUND_ALBEDO                   0.1
#define TG_MAX_SUN_ZENITH_ANGLE(use_half_precision) \
	(((use_half_precision) ? 102.0 : 120.0) / 180.0 * TG_PI64)

#define TG_LAMBDA_MIN                      360
#define TG_LAMBDA_MAX                      830
#define TG_LAMBDA_R                        680.0
#define TG_LAMBDA_G                        550.0
#define TG_LAMBDA_B                        440.0



#define TG_TRANSMITTANCE_TEXTURE_WIDTH     256
#define TG_TRANSMITTANCE_TEXTURE_HEIGHT    64

#define TG_SCATTERING_TEXTURE_R_SIZE       32
#define TG_SCATTERING_TEXTURE_MU_SIZE      128
#define TG_SCATTERING_TEXTURE_MU_S_SIZE    32
#define TG_SCATTERING_TEXTURE_NU_SIZE      8

#define TG_SCATTERING_TEXTURE_WIDTH        (TG_SCATTERING_TEXTURE_NU_SIZE * TG_SCATTERING_TEXTURE_MU_S_SIZE)
#define TG_SCATTERING_TEXTURE_HEIGHT       TG_SCATTERING_TEXTURE_MU_SIZE
#define TG_SCATTERING_TEXTURE_DEPTH        TG_SCATTERING_TEXTURE_R_SIZE

#define TG_IRRADIANCE_TEXTURE_WIDTH        64
#define TG_IRRADIANCE_TEXTURE_HEIGHT       16

#define TG_MAX_LUMINOUS_EFFICACY           683.0



static f64 tg__interpolate(u32 count, const f64* p_wavelengths, const f64* p_wavelength_function, f64 wavelength)
{
	f64 result = 0.0;

	if (wavelength < p_wavelengths[0])
	{
		result = p_wavelength_function[0];
	}
	else
	{
		b32 found = TG_FALSE;
		for (unsigned int i = 0; i < count - 1; ++i)
		{
			if (wavelength < p_wavelengths[i + 1])
			{
				found = TG_TRUE;

				const f64 u = (wavelength - p_wavelengths[i]) / (p_wavelengths[i + 1] - p_wavelengths[i]);
				result = p_wavelength_function[i] * (1.0 - u) + p_wavelength_function[i + 1] * u;
				break;
			}
		}

		if (!found)
		{
			result = p_wavelength_function[count - 1];
		}
	}

	return result;
}

static f64 tg__cie_color_matching_function_table_value(f64 wavelength, i32 column)
{
	f64 result = 0.0;

	if (wavelength > TG_LAMBDA_MIN && wavelength < TG_LAMBDA_MAX)
	{
		f64 u = (wavelength - TG_LAMBDA_MIN) / 5.0;
		const i32 row = tgm_f64_floor_to_i32(u);

		TG_ASSERT(row >= 0 && row + 1 < 95);
		TG_ASSERT(p_cie_2_deg_color_matching_functions[4 * row] <= wavelength && p_cie_2_deg_color_matching_functions[4 * (row + 1)] >= wavelength);

		u -= row;
		result = p_cie_2_deg_color_matching_functions[4 * row + column] * (1.0 - u) + p_cie_2_deg_color_matching_functions[4 * (row + 1) + column] * u;
	}

	return result;
}

static void tg__compute_spectral_radiance_to_luminance_factors(const f64* p_wavelengths, f64 lambda_power, f64* p_k_r, f64* p_k_g, f64* p_k_b)
{
	*p_k_r = 0.0;
	*p_k_g = 0.0;
	*p_k_b = 0.0;

	const f64 solar_r = tg__interpolate(48, p_wavelengths, p_solar_irradiance, TG_LAMBDA_R);
	const f64 solar_g = tg__interpolate(48, p_wavelengths, p_solar_irradiance, TG_LAMBDA_G);
	const f64 solar_b = tg__interpolate(48, p_wavelengths, p_solar_irradiance, TG_LAMBDA_B);

	const u32 dlambda = 1;

	for (u32 lambda = TG_LAMBDA_MIN; lambda < TG_LAMBDA_MAX; lambda += dlambda)
	{
		const f64 x_bar = tg__cie_color_matching_function_table_value(lambda, 1);
		const f64 y_bar = tg__cie_color_matching_function_table_value(lambda, 2);
		const f64 z_bar = tg__cie_color_matching_function_table_value(lambda, 3);

		const f64 r_bar = p_xyz_to_srgb[0] * x_bar + p_xyz_to_srgb[1] * y_bar + p_xyz_to_srgb[2] * z_bar;
		const f64 g_bar = p_xyz_to_srgb[3] * x_bar + p_xyz_to_srgb[4] * y_bar + p_xyz_to_srgb[5] * z_bar;
		const f64 b_bar = p_xyz_to_srgb[6] * x_bar + p_xyz_to_srgb[7] * y_bar + p_xyz_to_srgb[8] * z_bar;

		const f64 irradiance = tg__interpolate(48, p_wavelengths, p_solar_irradiance, lambda);

		*p_k_r += r_bar * irradiance / solar_r * tgm_f64_pow(lambda / TG_LAMBDA_R, lambda_power);
		*p_k_g += g_bar * irradiance / solar_g * tgm_f64_pow(lambda / TG_LAMBDA_G, lambda_power);
		*p_k_b += b_bar * irradiance / solar_b * tgm_f64_pow(lambda / TG_LAMBDA_B, lambda_power);
	}
	*p_k_r *= TG_MAX_LUMINOUS_EFFICACY * dlambda;
	*p_k_g *= TG_MAX_LUMINOUS_EFFICACY * dlambda;
	*p_k_b *= TG_MAX_LUMINOUS_EFFICACY * dlambda;
}

static u32 tg__glsl_header_factory(const tgvk_atmosphere_model* p_model, u32 size, char* p_buffer)
{
#pragma warning(push)
#pragma warning(disable:4296)
#pragma warning(disable:6294)

#define PUSH(p_string)     ((void)(i += tg_strcpy_no_nul(size - i, &p_buffer[i], p_string)))
#define PUSH_F32(value)    ((void)(i += tg_string_parse_f32_no_nul(size - i, &p_buffer[i], value)))
#define PUSH_F64(value)    ((void)(i += tg_string_parse_f64_no_nul(size - i, &p_buffer[i], value)))
#define PUSH_I32(value)    ((void)(i += tg_string_parse_i32_no_nul(size - i, &p_buffer[i], value)))
#define TO_STRING(p_v, scale)                                                                     \
	{                                                                                             \
		const f64 r = tg__interpolate(48, p_wavelengths, p_v, TG_LAMBDA_R) * scale;               \
		const f64 g = tg__interpolate(48, p_wavelengths, p_v, TG_LAMBDA_G) * scale;               \
		const f64 b = tg__interpolate(48, p_wavelengths, p_v, TG_LAMBDA_B) * scale;               \
		PUSH("vec3("); PUSH_F64(r); PUSH(", "); PUSH_F64(g); PUSH(", "); PUSH_F64(b); PUSH(")");  \
	}
#define DENSITY_LAYER(density_profile_layer)                                                  \
	{                                                                                         \
		PUSH("            tg_density_profile_layer(");                                        \
		PUSH_F64((density_profile_layer).width / TG_LENGTH_UNIT_IN_METERS);       PUSH(", "); \
		PUSH_F64((density_profile_layer).exp_term);                               PUSH(", "); \
		PUSH_F64((density_profile_layer).exp_scale * TG_LENGTH_UNIT_IN_METERS);   PUSH(", "); \
		PUSH_F64((density_profile_layer).linear_term * TG_LENGTH_UNIT_IN_METERS); PUSH(", "); \
		PUSH_F64((density_profile_layer).constant_term);                          PUSH(")");  \
	}
#define LAYER_COUNT 2
#define DENSITY_PROFILE(count, p_density_profile_layers)                            \
	{                                                                               \
		TG_ASSERT(count);                                                           \
		PUSH("    tg_density_profile(\r\n");                                        \
		PUSH("        tg_density_profile_layer[2](\r\n");                           \
		tgvk_atmosphere_density_profile_layer empty_density_profile_layer =  { 0 }; \
		for (u32 idx = 0; idx < LAYER_COUNT - count; idx++)                         \
		{                                                                           \
			DENSITY_LAYER(empty_density_profile_layer); PUSH(",\r\n");              \
		}                                                                           \
		for (u32 idx = 0; idx < count; idx++)                                       \
		{                                                                           \
			DENSITY_LAYER((p_density_profile_layers)[idx]);                         \
			if (idx < count - 1)                                                    \
			{                                                                       \
				PUSH(",\r\n");                                                      \
			}                                                                       \
			else                                                                    \
			{                                                                       \
				PUSH("\r\n");                                                       \
				PUSH("        )\r\n");                                              \
				PUSH("    )");                                                      \
			}                                                                       \
		}                                                                           \
	}

	u32 i = 0;

	PUSH("#version 450\r\n");
	PUSH("\r\n");
	PUSH("#define TG_IN(x) const in x\r\n");
	PUSH("#define TG_OUT(x) out x\r\n");
	PUSH("#define TG_TEMPLATE(x)\r\n");
	PUSH("#define TG_TEMPLATE_ARGUMENT(x)\r\n");
	PUSH("#define TG_ASSERT(x)\r\n");
	PUSH("\r\n");
	PUSH("const int TG_TRANSMITTANCE_TEXTURE_WIDTH  = "); PUSH_I32(TG_TRANSMITTANCE_TEXTURE_WIDTH); PUSH(";\r\n");
	PUSH("const int TG_TRANSMITTANCE_TEXTURE_HEIGHT = "); PUSH_I32(TG_TRANSMITTANCE_TEXTURE_HEIGHT); PUSH(";\r\n");
	PUSH("const int TG_SCATTERING_TEXTURE_R_SIZE    = "); PUSH_I32(TG_SCATTERING_TEXTURE_R_SIZE); PUSH(";\r\n");
	PUSH("const int TG_SCATTERING_TEXTURE_MU_SIZE   = "); PUSH_I32(TG_SCATTERING_TEXTURE_MU_SIZE); PUSH(";\r\n");
	PUSH("const int TG_SCATTERING_TEXTURE_MU_S_SIZE = "); PUSH_I32(TG_SCATTERING_TEXTURE_MU_S_SIZE); PUSH(";\r\n");
	PUSH("const int TG_SCATTERING_TEXTURE_NU_SIZE   = "); PUSH_I32(TG_SCATTERING_TEXTURE_NU_SIZE); PUSH(";\r\n");
	PUSH("const int TG_IRRADIANCE_TEXTURE_WIDTH     = "); PUSH_I32(TG_IRRADIANCE_TEXTURE_WIDTH); PUSH(";\r\n");
	PUSH("const int TG_IRRADIANCE_TEXTURE_HEIGHT    = "); PUSH_I32(TG_IRRADIANCE_TEXTURE_HEIGHT); PUSH(";\r\n");
	PUSH("\r\n");

	if (p_model->settings.combine_scattering_textures)
	{
		PUSH("#define TG_COMBINED_SCATTERING_TEXTURES\r\n");
		PUSH("\r\n");
	}

	PUSH(p_atmosphere_definitions);
	PUSH("\r\n");

	f64 p_wavelengths[48] = { 0 };
	f64 p_solar_irradiances[48] = { 0 };
	f64 p_rayleigh_scatterings[48] = { 0 };
	f64 p_mie_scatterings[48] = { 0 };
	f64 p_mie_extinctions[48] = { 0 };
	f64 p_absorption_extinctions[48] = { 0 };
	f64 p_ground_albedos[48] = { 0 };

	for (u32 idx = 0, l = TG_LAMBDA_MIN; l <= TG_LAMBDA_MAX; idx++, l += 10)
	{
		const f64 lambda = (f64)l * 1e-3;
		const f64 mie = TG_MIE_ANGSTROM_BETA / TG_MIE_SCALE_HEIGHT * tgm_f64_pow(lambda, -TG_MIE_ANGSTROM_ALPHA);

		p_wavelengths[idx] = l;
		p_solar_irradiances[idx] = p_model->settings.use_constant_solar_spectrum ? TG_CONSTANT_SOLAR_IRRADIANCE : p_solar_irradiance[(l - TG_LAMBDA_MIN) / 10];
		p_rayleigh_scatterings[idx] = TG_RAYLEIGH * tgm_f64_pow(lambda, -4.0);
		p_mie_scatterings[idx] = mie * TG_MIE_SINGLE_SCATTERING_ALBEDO;
		p_mie_extinctions[idx] = mie;
		p_absorption_extinctions[idx] = p_model->settings.use_ozone ? TG_MAX_OZONE_NUMBER_DENSITY * p_ozone_cross_section[(l - TG_LAMBDA_MIN) / 10] : 0.0;
		p_ground_albedos[idx] = TG_GROUND_ALBEDO;
	}

	f64 sun_k_r, sun_k_g, sun_k_b;
	tg__compute_spectral_radiance_to_luminance_factors(p_wavelengths, 0.0, &sun_k_r, &sun_k_g, &sun_k_b);

	PUSH("const tg_atmosphere_parameters ATMOSPHERE = tg_atmosphere_parameters(\r\n");
	PUSH("    "); TO_STRING(p_solar_irradiances, 1.0); PUSH(",\r\n");
	PUSH("    "); PUSH_F64(TG_SUN_ANGULAR_RADIUS); PUSH(",\r\n");
	PUSH("    "); PUSH_F64(TG_BOTTOM_RADIUS / TG_LENGTH_UNIT_IN_METERS); PUSH(",\r\n");
	PUSH("    "); PUSH_F64(TG_TOP_RADIUS / TG_LENGTH_UNIT_IN_METERS); PUSH(",\r\n");

	tgvk_atmosphere_density_profile_layer rayleigh_density_profile_layer = { 0 };
	rayleigh_density_profile_layer.width = 0.0;
	rayleigh_density_profile_layer.exp_term = 1.0;
	rayleigh_density_profile_layer.exp_scale = -1.0 / TG_RAYLEIGH_SCALE_HEIGHT;
	rayleigh_density_profile_layer.linear_term = 0.0;
	rayleigh_density_profile_layer.constant_term = 0.0;

	DENSITY_PROFILE(1, &rayleigh_density_profile_layer); PUSH(",\r\n");
	PUSH("    "); TO_STRING(p_rayleigh_scatterings, TG_LENGTH_UNIT_IN_METERS); PUSH(",\r\n");

	tgvk_atmosphere_density_profile_layer mie_density_profile_layer = { 0 };
	mie_density_profile_layer.width = 0.0;
	mie_density_profile_layer.exp_term = 1.0;
	mie_density_profile_layer.exp_scale = -1.0 / TG_MIE_SCALE_HEIGHT;
	mie_density_profile_layer.linear_term = 0.0;
	mie_density_profile_layer.constant_term = 0.0;

	DENSITY_PROFILE(1, &mie_density_profile_layer); PUSH(",\r\n");
	PUSH("    "); TO_STRING(p_mie_scatterings, TG_LENGTH_UNIT_IN_METERS); PUSH(",\r\n");
	PUSH("    "); TO_STRING(p_mie_extinctions, TG_LENGTH_UNIT_IN_METERS); PUSH(",\r\n");
	PUSH("    "); PUSH_F64(TG_MIE_PHASE_FUNCTION_G); PUSH(",\r\n");

	tgvk_atmosphere_density_profile_layer p_absorption_density_profile_layers[2] = { 0 };
	p_absorption_density_profile_layers[0].width = 25000.0;
	p_absorption_density_profile_layers[0].exp_term = 0.0;
	p_absorption_density_profile_layers[0].exp_scale = 0.0;
	p_absorption_density_profile_layers[0].linear_term = 1.0 / 15000.0;
	p_absorption_density_profile_layers[0].constant_term = -2.0 / 3.0;
	p_absorption_density_profile_layers[1].width = 0.0;
	p_absorption_density_profile_layers[1].exp_term = 0.0;
	p_absorption_density_profile_layers[1].exp_scale = 0.0;
	p_absorption_density_profile_layers[1].linear_term = -1.0 / 15000.0;
	p_absorption_density_profile_layers[1].constant_term = 8.0 / 3.0;

	DENSITY_PROFILE(2, p_absorption_density_profile_layers); PUSH(",\r\n");
	PUSH("    "); TO_STRING(p_absorption_extinctions, TG_LENGTH_UNIT_IN_METERS); PUSH(",\r\n");
	PUSH("    "); TO_STRING(p_ground_albedos, 1.0); PUSH(",\r\n");
	PUSH("    "); PUSH_F64(tgm_f64_cos(TG_MAX_SUN_ZENITH_ANGLE(p_model->settings.use_half_precision))); PUSH("\r\n");
	PUSH(");\r\n");
	PUSH("\r\n");

	const u32 precomputed_wavelength_count = p_model->settings.precomputed_wavelength_count;
	const b32 precompute_illuminance = precomputed_wavelength_count > 3;
	f64 sky_k_r, sky_k_g, sky_k_b;
	if (precompute_illuminance)
	{
		sky_k_r = TG_MAX_LUMINOUS_EFFICACY;
		sky_k_g = TG_MAX_LUMINOUS_EFFICACY;
		sky_k_b = TG_MAX_LUMINOUS_EFFICACY;
	}
	else
	{
		tg__compute_spectral_radiance_to_luminance_factors(p_wavelengths, -3.0, &sky_k_r, &sky_k_g, &sky_k_b);
	}

	PUSH("const vec3 TG_SKY_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3("); PUSH_F64(sky_k_r); PUSH(", "); PUSH_F64(sky_k_g); PUSH(", "); PUSH_F64(sky_k_b); PUSH(");\r\n");
	PUSH("const vec3 TG_SUN_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3("); PUSH_F64(sun_k_r); PUSH(", "); PUSH_F64(sun_k_g); PUSH(", "); PUSH_F64(sun_k_b); PUSH(");\r\n");
	PUSH("\r\n");

	PUSH(p_atmosphere_functions);
	PUSH("\0");

	return i;

#undef DENSITY_PROFILE
#undef LAYER_COUNT
#undef DENSITY_LAYER
#undef TO_STRING
#undef PUSH_I32
#undef PUSH_F64
#undef PUSH_F32
#undef PUSH

#pragma warning(pop)
}



static void tg__precompute(tgvk_atmosphere_model* p_model, VkFormat layered_image_format)
{
	tg_sampler_create_info sampler_create_info = { 0 };
	sampler_create_info.min_filter = TG_IMAGE_FILTER_LINEAR;
	sampler_create_info.mag_filter = TG_IMAGE_FILTER_LINEAR;
	sampler_create_info.address_mode_u = TG_IMAGE_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.address_mode_v = TG_IMAGE_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.address_mode_w = TG_IMAGE_ADDRESS_MODE_CLAMP_TO_EDGE;

	tgvk_image delta_irradiance_texture = tgvk_image_create(TGVK_IMAGE_TYPE_COLOR, TG_IRRADIANCE_TEXTURE_WIDTH, TG_IRRADIANCE_TEXTURE_HEIGHT, VK_FORMAT_R32G32B32A32_SFLOAT, &sampler_create_info);

	tgvk_layered_image delta_rayleigh_scattering_texture = tgvk_layered_image_create(
		TGVK_IMAGE_TYPE_COLOR,
		TG_SCATTERING_TEXTURE_WIDTH, TG_SCATTERING_TEXTURE_HEIGHT, TG_SCATTERING_TEXTURE_DEPTH,
		layered_image_format, &sampler_create_info
	);

	tgvk_layered_image delta_mie_scattering_texture = tgvk_layered_image_create(
		TGVK_IMAGE_TYPE_COLOR,
		TG_SCATTERING_TEXTURE_WIDTH, TG_SCATTERING_TEXTURE_HEIGHT, TG_SCATTERING_TEXTURE_DEPTH,
		layered_image_format, &sampler_create_info
	);

	tgvk_layered_image delta_scattering_density_texture = tgvk_layered_image_create(
		TGVK_IMAGE_TYPE_COLOR,
		TG_SCATTERING_TEXTURE_WIDTH, TG_SCATTERING_TEXTURE_HEIGHT, TG_SCATTERING_TEXTURE_DEPTH,
		layered_image_format, &sampler_create_info
	);

	tgvk_layered_image* p_delta_multiple_scattering_texture = &delta_rayleigh_scattering_texture;

	p_model->api_shader.capacity = 1 << 16;
	p_model->api_shader.header_count = 0;
	p_model->api_shader.total_count = 0;
	if (!p_model->api_shader.p_source)
	{
		p_model->api_shader.p_source = TG_MEMORY_ALLOC(p_model->api_shader.capacity);
	}
	p_model->api_shader.header_count = tg__glsl_header_factory(p_model, p_model->api_shader.capacity, p_model->api_shader.p_source);
	p_model->api_shader.total_count = p_model->api_shader.header_count;
	p_model->api_shader.total_count += tg_strcpy_no_nul(p_model->api_shader.capacity - p_model->api_shader.total_count, &p_model->api_shader.p_source[p_model->api_shader.total_count], "\r\n");
	const b32 precompute_illuminance = p_model->settings.precomputed_wavelength_count > 3;
	if (!precompute_illuminance)
	{
		p_model->api_shader.total_count += tg_strcpy_no_nul(
			p_model->api_shader.capacity - p_model->api_shader.total_count,
			&p_model->api_shader.p_source[p_model->api_shader.total_count],
			"#define TG_RADIANCE_API_ENABLED\r\n\r\n"
		);
	}
	p_model->api_shader.total_count += tg_strcpy_no_nul(
		p_model->api_shader.capacity - p_model->api_shader.total_count,
		&p_model->api_shader.p_source[p_model->api_shader.total_count],
		p_atmosphere_shader
	);

	const u32 precomputed_wavelength_count = p_model->settings.precomputed_wavelength_count;
	if (precomputed_wavelength_count <= 3)
	{
		const m4 luminance_from_radiance = tgm_m4_identity();
		const tg_blend_mode blend = TG_BLEND_MODE_NONE;

		const u32 fragment_shader_buffer_size = 1 << 16;
		char* p_fragment_shader_buffer = TG_MEMORY_STACK_ALLOC((u64)fragment_shader_buffer_size);

		u32 header_count = tg_strncpy_no_nul(fragment_shader_buffer_size, p_fragment_shader_buffer, p_model->api_shader.header_count, p_model->api_shader.p_source);
		header_count += tg_strcpy_no_nul(fragment_shader_buffer_size - header_count, &p_fragment_shader_buffer[header_count], "\r\n");

		tgvk_shader* p_vertex_shader = TG_MEMORY_STACK_ALLOC(sizeof(*p_vertex_shader));
		*p_vertex_shader = tgvk_shader_create_from_glsl(TG_SHADER_TYPE_VERTEX, p_atmosphere_vertex_shader);

		tgvk_shader* p_geometry_shader = TG_MEMORY_STACK_ALLOC(sizeof(*p_geometry_shader));
		*p_geometry_shader = tgvk_shader_create_from_glsl(TG_SHADER_TYPE_GEOMETRY, p_atmosphere_geometry_shader);

		// TODO: use
		VkSemaphore semaphore = tgvk_semaphore_create();
		tgvk_buffer ubo = tgvk_buffer_create(sizeof(m4) + sizeof(i32), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		tgvk_buffer geometry_ubo = tgvk_buffer_create(sizeof(i32), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);



		VkAttachmentDescription p_attachment_descriptions[4] = { 0 };
		VkAttachmentReference p_color_attachment_references[4] = { 0 };
		VkSubpassDescription subpass_description = { 0 };
		VkImageView p_attachments[4] = { 0 };
		tg_blend_mode p_blend_modes[4] = { 0 };
		tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };

		for (u32 i = 0; i < 4; i++)
		{
			p_attachment_descriptions[i].flags = 0;
			p_attachment_descriptions[i].samples = VK_SAMPLE_COUNT_1_BIT;
			p_attachment_descriptions[i].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			p_attachment_descriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			p_attachment_descriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			p_attachment_descriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			p_attachment_descriptions[i].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			p_attachment_descriptions[i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			p_color_attachment_references[i].attachment = i;
			p_color_attachment_references[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			p_blend_modes[i] = TG_BLEND_MODE_NONE;
		}

		subpass_description.flags = 0;
		subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_description.inputAttachmentCount = 0;
		subpass_description.pInputAttachments = TG_NULL;
		subpass_description.pColorAttachments = p_color_attachment_references;
		subpass_description.pResolveAttachments = TG_NULL;
		subpass_description.pDepthStencilAttachment = TG_NULL;
		subpass_description.preserveAttachmentCount = 0;
		subpass_description.pPreserveAttachments = TG_NULL;

		graphics_pipeline_create_info.p_vertex_shader = p_vertex_shader;
		graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
		graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
		graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
		graphics_pipeline_create_info.p_blend_modes = p_blend_modes;
		graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;



		// transmittance

		VkRenderPass transmittance_render_pass;
		tgvk_framebuffer transmittance_framebuffer;
		tgvk_pipeline transmittance_pipeline;

		tg_strcpy(fragment_shader_buffer_size - header_count, &p_fragment_shader_buffer[header_count], p_atmosphere_compute_transmittance_shader);
		tgvk_shader* p_transmittance_fragment_shader = TG_MEMORY_STACK_ALLOC(sizeof(*p_transmittance_fragment_shader));
		*p_transmittance_fragment_shader = tgvk_shader_create_from_glsl(TG_SHADER_TYPE_FRAGMENT, p_fragment_shader_buffer);

		p_attachment_descriptions[0].format = p_model->rendering.transmittance_texture.format;
		subpass_description.colorAttachmentCount = 1;
		transmittance_render_pass = tgvk_render_pass_create(p_attachment_descriptions, &subpass_description);

		p_attachments[0] = p_model->rendering.transmittance_texture.image_view;
		transmittance_framebuffer = tgvk_framebuffer_create(transmittance_render_pass, 1, p_attachments, TG_TRANSMITTANCE_TEXTURE_WIDTH, TG_TRANSMITTANCE_TEXTURE_HEIGHT);

		graphics_pipeline_create_info.p_fragment_shader = p_transmittance_fragment_shader;
		graphics_pipeline_create_info.render_pass = transmittance_render_pass;
		graphics_pipeline_create_info.viewport_size = (v2){ TG_TRANSMITTANCE_TEXTURE_WIDTH, TG_TRANSMITTANCE_TEXTURE_HEIGHT };
		transmittance_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);

		tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		{
			TO_COLOR_ATTACHMENT_LAYOUT(p_model->rendering.transmittance_texture);

			tgvk_cmd_begin_render_pass(p_command_buffer, transmittance_render_pass, &transmittance_framebuffer, VK_SUBPASS_CONTENTS_INLINE);

			tgvk_cmd_bind_pipeline(p_command_buffer, &transmittance_pipeline);
			tgvk_cmd_bind_and_draw_screen_quad(p_command_buffer);

			vkCmdEndRenderPass(p_command_buffer->command_buffer);
		}
		tgvk_command_buffer_end_and_submit(p_command_buffer);



		// direct irradiance

		VkRenderPass direct_irradiance_render_pass;
		tgvk_framebuffer direct_irradiance_framebuffer;
		tgvk_shader* p_direct_irradiance_fragment_shader;
		tgvk_pipeline direct_irradiance_pipeline;
		tgvk_descriptor_set direct_irradiance_descriptor_set;

		p_attachment_descriptions[0].format = delta_irradiance_texture.format;
		p_attachment_descriptions[1].format = p_model->rendering.irradiance_texture.format;
		subpass_description.colorAttachmentCount = 2;
		direct_irradiance_render_pass = tgvk_render_pass_create(p_attachment_descriptions, &subpass_description);

		p_attachments[0] = delta_irradiance_texture.image_view;
		p_attachments[1] = p_model->rendering.irradiance_texture.image_view;
		direct_irradiance_framebuffer = tgvk_framebuffer_create(direct_irradiance_render_pass, 2, p_attachments, TG_IRRADIANCE_TEXTURE_WIDTH, TG_IRRADIANCE_TEXTURE_HEIGHT);

		tg_strcpy(fragment_shader_buffer_size - header_count, &p_fragment_shader_buffer[header_count], p_atmosphere_compute_direct_irradiance_shader);
		p_direct_irradiance_fragment_shader = TG_MEMORY_STACK_ALLOC(sizeof(*p_direct_irradiance_fragment_shader));
		*p_direct_irradiance_fragment_shader = tgvk_shader_create_from_glsl(TG_SHADER_TYPE_FRAGMENT, p_fragment_shader_buffer);

		graphics_pipeline_create_info.p_fragment_shader = p_direct_irradiance_fragment_shader;
		graphics_pipeline_create_info.render_pass = direct_irradiance_render_pass;
		graphics_pipeline_create_info.viewport_size = (v2){ TG_IRRADIANCE_TEXTURE_WIDTH, TG_IRRADIANCE_TEXTURE_HEIGHT };
		p_blend_modes[1] = blend;
		direct_irradiance_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);

		direct_irradiance_descriptor_set = tgvk_descriptor_set_create(&direct_irradiance_pipeline);
		tgvk_descriptor_set_update_image(direct_irradiance_descriptor_set.descriptor_set, &p_model->rendering.transmittance_texture, 0);

		tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		{
			COLOR_ATTACHMENT_TO_SHADER_READ(p_model->rendering.transmittance_texture);

			TO_COLOR_ATTACHMENT_LAYOUT(delta_irradiance_texture);
			TO_COLOR_ATTACHMENT_LAYOUT(p_model->rendering.irradiance_texture);

			tgvk_cmd_begin_render_pass(p_command_buffer, direct_irradiance_render_pass, &direct_irradiance_framebuffer, VK_SUBPASS_CONTENTS_INLINE);

			tgvk_cmd_bind_pipeline(p_command_buffer, &direct_irradiance_pipeline);
			tgvk_cmd_bind_descriptor_set(p_command_buffer, &direct_irradiance_pipeline, &direct_irradiance_descriptor_set);
			tgvk_cmd_bind_and_draw_screen_quad(p_command_buffer);

			vkCmdEndRenderPass(p_command_buffer->command_buffer);
		}
		tgvk_command_buffer_end_and_submit(p_command_buffer);

		tgvk_descriptor_set_destroy(&direct_irradiance_descriptor_set);
		tgvk_pipeline_destroy(&direct_irradiance_pipeline);
		tgvk_shader_destroy(p_direct_irradiance_fragment_shader);
		TG_MEMORY_STACK_FREE(sizeof(*p_direct_irradiance_fragment_shader));
		tgvk_framebuffer_destroy(&direct_irradiance_framebuffer);
		tgvk_render_pass_destroy(direct_irradiance_render_pass);



		// single scattering

		VkRenderPass single_scattering_render_pass;
		tgvk_framebuffer single_scattering_framebuffer;
		tgvk_shader* p_single_scattering_fragment_shader;
		tgvk_pipeline single_scattering_pipeline;
		tgvk_descriptor_set single_scattering_descriptor_set;

		p_attachment_descriptions[0].format = delta_rayleigh_scattering_texture.format;
		p_attachment_descriptions[1].format = delta_mie_scattering_texture.format;
		p_attachment_descriptions[2].format = p_model->rendering.scattering_texture.format;
		if (p_model->rendering.optional_single_mie_scattering_texture.image)
		{
			p_attachment_descriptions[3].format = p_model->rendering.optional_single_mie_scattering_texture.format;
			subpass_description.colorAttachmentCount = 4;
			single_scattering_render_pass = tgvk_render_pass_create(p_attachment_descriptions, &subpass_description);
		}
		else
		{
			subpass_description.colorAttachmentCount = 3;
			single_scattering_render_pass = tgvk_render_pass_create(p_attachment_descriptions, &subpass_description);
		}

		p_attachments[0] = delta_rayleigh_scattering_texture.write_image_view;
		p_attachments[1] = delta_mie_scattering_texture.write_image_view;
		p_attachments[2] = p_model->rendering.scattering_texture.write_image_view;
		if (p_model->rendering.optional_single_mie_scattering_texture.image)
		{
			p_attachments[3] = p_model->rendering.optional_single_mie_scattering_texture.write_image_view;
			single_scattering_framebuffer = tgvk_framebuffer_create_layered(single_scattering_render_pass, 4, p_attachments, TG_SCATTERING_TEXTURE_WIDTH, TG_SCATTERING_TEXTURE_HEIGHT, TG_SCATTERING_TEXTURE_DEPTH);

			tg_strcpy(fragment_shader_buffer_size - header_count, &p_fragment_shader_buffer[header_count], p_atmosphere_compute_single_scattering_shader);
		}
		else
		{
			single_scattering_framebuffer = tgvk_framebuffer_create_layered(single_scattering_render_pass, 3, p_attachments, TG_SCATTERING_TEXTURE_WIDTH, TG_SCATTERING_TEXTURE_HEIGHT, TG_SCATTERING_TEXTURE_DEPTH);

			tg_strcpy(fragment_shader_buffer_size - header_count, &p_fragment_shader_buffer[header_count], p_atmosphere_compute_single_scattering_shader_no_single_mie_scattering_texture);
		}

		p_single_scattering_fragment_shader = TG_MEMORY_STACK_ALLOC(sizeof(*p_single_scattering_fragment_shader));
		*p_single_scattering_fragment_shader = tgvk_shader_create_from_glsl(TG_SHADER_TYPE_FRAGMENT, p_fragment_shader_buffer);

		graphics_pipeline_create_info.p_fragment_shader = p_single_scattering_fragment_shader;
		graphics_pipeline_create_info.p_geometry_shader = p_geometry_shader;
		graphics_pipeline_create_info.render_pass = single_scattering_render_pass;
		graphics_pipeline_create_info.viewport_size = (v2){ TG_SCATTERING_TEXTURE_WIDTH, TG_SCATTERING_TEXTURE_HEIGHT };
		p_blend_modes[1] = TG_BLEND_MODE_NONE;
		p_blend_modes[2] = blend;
		p_blend_modes[3] = blend;
		single_scattering_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
		
		single_scattering_descriptor_set = tgvk_descriptor_set_create(&single_scattering_pipeline);
		tgvk_descriptor_set_update_uniform_buffer(single_scattering_descriptor_set.descriptor_set, geometry_ubo.buffer, 0);
		tgvk_descriptor_set_update_uniform_buffer(single_scattering_descriptor_set.descriptor_set, ubo.buffer, 1);
		tgvk_descriptor_set_update_image(single_scattering_descriptor_set.descriptor_set, &p_model->rendering.transmittance_texture, 2);

		*(m4*)ubo.memory.p_mapped_device_memory = luminance_from_radiance;

		for (u32 layer = 0; layer < TG_SCATTERING_TEXTURE_DEPTH; layer++)
		{
			*(i32*)geometry_ubo.memory.p_mapped_device_memory = (i32)layer;
			*(i32*)&((m4*)ubo.memory.p_mapped_device_memory)[1] = (i32)layer;

			tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			{
				if (layer == 0)
				{
					TO_COLOR_ATTACHMENT_LAYOUT_LAYERED(delta_rayleigh_scattering_texture);
					TO_COLOR_ATTACHMENT_LAYOUT_LAYERED(delta_mie_scattering_texture);
					TO_COLOR_ATTACHMENT_LAYOUT_LAYERED(p_model->rendering.scattering_texture);
					if (p_model->rendering.optional_single_mie_scattering_texture.image)
					{
						TO_COLOR_ATTACHMENT_LAYOUT_LAYERED(p_model->rendering.optional_single_mie_scattering_texture);
					}
				}

				tgvk_cmd_begin_render_pass(p_command_buffer, single_scattering_render_pass, &single_scattering_framebuffer, VK_SUBPASS_CONTENTS_INLINE);

				tgvk_cmd_bind_pipeline(p_command_buffer, &single_scattering_pipeline);
				tgvk_cmd_bind_descriptor_set(p_command_buffer, &single_scattering_pipeline, &single_scattering_descriptor_set);
				tgvk_cmd_bind_and_draw_screen_quad(p_command_buffer);

				vkCmdEndRenderPass(p_command_buffer->command_buffer);
			}
			tgvk_command_buffer_end_and_submit(p_command_buffer);
		}

		tgvk_descriptor_set_destroy(&single_scattering_descriptor_set);
		tgvk_pipeline_destroy(&single_scattering_pipeline);
		tgvk_shader_destroy(p_single_scattering_fragment_shader);
		TG_MEMORY_STACK_FREE(sizeof(*p_single_scattering_fragment_shader));
		tgvk_framebuffer_destroy(&single_scattering_framebuffer);
		tgvk_render_pass_destroy(single_scattering_render_pass);



		// scattering density, indirect irradiance, multiple scattering

		VkRenderPass scattering_density_render_pass;
		tgvk_framebuffer scattering_density_framebuffer;
		tgvk_shader* p_scattering_density_fragment_shader;
		tgvk_pipeline scattering_density_pipeline;
		tgvk_descriptor_set scattering_density_descriptor_set;

		p_attachment_descriptions[0].format = delta_scattering_density_texture.format;
		subpass_description.colorAttachmentCount = 1;
		scattering_density_render_pass = tgvk_render_pass_create(p_attachment_descriptions, &subpass_description);

		p_attachments[0] = delta_scattering_density_texture.write_image_view;
		scattering_density_framebuffer = tgvk_framebuffer_create_layered(scattering_density_render_pass, 1, p_attachments, TG_SCATTERING_TEXTURE_WIDTH, TG_SCATTERING_TEXTURE_HEIGHT, TG_SCATTERING_TEXTURE_DEPTH);

		tg_strcpy(fragment_shader_buffer_size - header_count, &p_fragment_shader_buffer[header_count], p_atmosphere_compute_scattering_density_shader);
		p_scattering_density_fragment_shader = TG_MEMORY_STACK_ALLOC(sizeof(*p_scattering_density_fragment_shader));
		*p_scattering_density_fragment_shader = tgvk_shader_create_from_glsl(TG_SHADER_TYPE_FRAGMENT, p_fragment_shader_buffer);

		graphics_pipeline_create_info.p_fragment_shader = p_scattering_density_fragment_shader;
		graphics_pipeline_create_info.render_pass = scattering_density_render_pass;
		scattering_density_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);

		scattering_density_descriptor_set = tgvk_descriptor_set_create(&scattering_density_pipeline);
		tgvk_descriptor_set_update_uniform_buffer(scattering_density_descriptor_set.descriptor_set, geometry_ubo.buffer, 0);
		tgvk_descriptor_set_update_uniform_buffer_range(scattering_density_descriptor_set.descriptor_set, 0, 2 * sizeof(i32), ubo.buffer, 1);
		tgvk_descriptor_set_update_image(scattering_density_descriptor_set.descriptor_set, &p_model->rendering.transmittance_texture, 2);
		tgvk_descriptor_set_update_layered_image(scattering_density_descriptor_set.descriptor_set, &delta_rayleigh_scattering_texture, 3);
		tgvk_descriptor_set_update_layered_image(scattering_density_descriptor_set.descriptor_set, &delta_mie_scattering_texture, 4);
		tgvk_descriptor_set_update_layered_image(scattering_density_descriptor_set.descriptor_set, p_delta_multiple_scattering_texture, 5);
		tgvk_descriptor_set_update_image(scattering_density_descriptor_set.descriptor_set, &delta_irradiance_texture, 6);


		VkRenderPass indirect_irradiance_render_pass;
		tgvk_framebuffer indirect_irradiance_framebuffer;
		tgvk_shader* p_indirect_irradiance_fragment_shader;
		tgvk_pipeline indirect_irradiance_pipeline;
		tgvk_descriptor_set indirect_irradiance_descriptor_set;

		p_attachment_descriptions[0].format = delta_irradiance_texture.format;
		p_attachment_descriptions[1].format = p_model->rendering.irradiance_texture.format;
		subpass_description.colorAttachmentCount = 2;
		indirect_irradiance_render_pass = tgvk_render_pass_create(p_attachment_descriptions, &subpass_description);

		p_attachments[0] = delta_irradiance_texture.image_view;
		p_attachments[1] = p_model->rendering.irradiance_texture.image_view;
		indirect_irradiance_framebuffer = tgvk_framebuffer_create(indirect_irradiance_render_pass, 2, p_attachments, TG_IRRADIANCE_TEXTURE_WIDTH, TG_IRRADIANCE_TEXTURE_HEIGHT);

		tg_strcpy(fragment_shader_buffer_size - header_count, &p_fragment_shader_buffer[header_count], p_atmosphere_compute_indirect_irradiance_shader);
		p_indirect_irradiance_fragment_shader = TG_MEMORY_STACK_ALLOC(sizeof(*p_indirect_irradiance_fragment_shader));
		*p_indirect_irradiance_fragment_shader = tgvk_shader_create_from_glsl(TG_SHADER_TYPE_FRAGMENT, p_fragment_shader_buffer);

		graphics_pipeline_create_info.p_fragment_shader = p_indirect_irradiance_fragment_shader;
		graphics_pipeline_create_info.p_geometry_shader = TG_NULL;
		graphics_pipeline_create_info.render_pass = indirect_irradiance_render_pass;
		graphics_pipeline_create_info.viewport_size = (v2){ TG_IRRADIANCE_TEXTURE_WIDTH, TG_IRRADIANCE_TEXTURE_HEIGHT };
		p_blend_modes[1] = TG_BLEND_MODE_ADD;
		indirect_irradiance_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);

		indirect_irradiance_descriptor_set = tgvk_descriptor_set_create(&indirect_irradiance_pipeline);
		tgvk_descriptor_set_update_uniform_buffer(indirect_irradiance_descriptor_set.descriptor_set, ubo.buffer, 0);
		tgvk_descriptor_set_update_layered_image(indirect_irradiance_descriptor_set.descriptor_set, &delta_rayleigh_scattering_texture, 1);
		tgvk_descriptor_set_update_layered_image(indirect_irradiance_descriptor_set.descriptor_set, &delta_mie_scattering_texture, 2);
		tgvk_descriptor_set_update_layered_image(indirect_irradiance_descriptor_set.descriptor_set, p_delta_multiple_scattering_texture, 3);


		VkRenderPass multiple_scattering_render_pass;
		tgvk_framebuffer multiple_scattering_framebuffer;
		tgvk_shader* p_multiple_scattering_fragment_shader;
		tgvk_pipeline multiple_scattering_pipeline;
		tgvk_descriptor_set multiple_scattering_descriptor_set;

		p_attachment_descriptions[0].format = p_delta_multiple_scattering_texture->format;
		p_attachment_descriptions[1].format = p_model->rendering.scattering_texture.format;
		multiple_scattering_render_pass = tgvk_render_pass_create(p_attachment_descriptions, &subpass_description);

		p_attachments[0] = p_delta_multiple_scattering_texture->write_image_view;
		p_attachments[1] = p_model->rendering.scattering_texture.write_image_view;
		multiple_scattering_framebuffer = tgvk_framebuffer_create_layered(multiple_scattering_render_pass, 2, p_attachments, TG_SCATTERING_TEXTURE_WIDTH, TG_SCATTERING_TEXTURE_HEIGHT, TG_SCATTERING_TEXTURE_DEPTH);

		tg_strcpy(fragment_shader_buffer_size - header_count, &p_fragment_shader_buffer[header_count], p_atmosphere_compute_multiple_scattering_shader);
		p_multiple_scattering_fragment_shader = TG_MEMORY_STACK_ALLOC(sizeof(*p_multiple_scattering_fragment_shader));
		*p_multiple_scattering_fragment_shader = tgvk_shader_create_from_glsl(TG_SHADER_TYPE_FRAGMENT, p_fragment_shader_buffer);

		graphics_pipeline_create_info.p_fragment_shader = p_multiple_scattering_fragment_shader;
		graphics_pipeline_create_info.p_geometry_shader = p_geometry_shader;
		graphics_pipeline_create_info.render_pass = multiple_scattering_render_pass;
		graphics_pipeline_create_info.viewport_size = (v2){ TG_SCATTERING_TEXTURE_WIDTH, TG_SCATTERING_TEXTURE_HEIGHT };
		multiple_scattering_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);

		multiple_scattering_descriptor_set = tgvk_descriptor_set_create(&multiple_scattering_pipeline);
		tgvk_descriptor_set_update_uniform_buffer(multiple_scattering_descriptor_set.descriptor_set, geometry_ubo.buffer, 0);
		tgvk_descriptor_set_update_uniform_buffer(multiple_scattering_descriptor_set.descriptor_set, ubo.buffer, 1);
		tgvk_descriptor_set_update_image(multiple_scattering_descriptor_set.descriptor_set, &p_model->rendering.transmittance_texture, 2);
		tgvk_descriptor_set_update_layered_image(multiple_scattering_descriptor_set.descriptor_set, &delta_scattering_density_texture, 3);


		for (u32 scattering_order = 2; scattering_order <= 4; scattering_order++)
		{
			((i32*)ubo.memory.p_mapped_device_memory)[0] = (i32)scattering_order;

			for (u32 layer = 0; layer < TG_SCATTERING_TEXTURE_DEPTH; layer++)
			{
				*(i32*)geometry_ubo.memory.p_mapped_device_memory = layer;
				((i32*)ubo.memory.p_mapped_device_memory)[1] = (i32)layer;

				tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
				{
					if (layer == 0)
					{
						if (scattering_order == 2)
						{
							COLOR_ATTACHMENT_TO_SHADER_READ_LAYERED(delta_mie_scattering_texture);
							TO_COLOR_ATTACHMENT_LAYOUT_LAYERED(delta_scattering_density_texture);
						}
						COLOR_ATTACHMENT_TO_SHADER_READ(delta_irradiance_texture);
						COLOR_ATTACHMENT_TO_SHADER_READ_LAYERED(delta_rayleigh_scattering_texture);
					}

					tgvk_cmd_begin_render_pass(p_command_buffer, scattering_density_render_pass, &scattering_density_framebuffer, VK_SUBPASS_CONTENTS_INLINE);

					tgvk_cmd_bind_pipeline(p_command_buffer, &scattering_density_pipeline);
					tgvk_cmd_bind_descriptor_set(p_command_buffer, &scattering_density_pipeline, &scattering_density_descriptor_set);
					tgvk_cmd_bind_and_draw_screen_quad(p_command_buffer);

					vkCmdEndRenderPass(p_command_buffer->command_buffer);

					if (layer == TG_SCATTERING_TEXTURE_DEPTH - 1)
					{
						SHADER_READ_TO_COLOR_ATTACHMENT(delta_irradiance_texture);
					}
				}
				tgvk_command_buffer_end_and_submit(p_command_buffer);
			}

			*(m4*)ubo.memory.p_mapped_device_memory = luminance_from_radiance;
			*(i32*)&((m4*)ubo.memory.p_mapped_device_memory)[1] = (i32)scattering_order - 1;

			tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			{
				tgvk_cmd_begin_render_pass(p_command_buffer, indirect_irradiance_render_pass, &indirect_irradiance_framebuffer, VK_SUBPASS_CONTENTS_INLINE);

				tgvk_cmd_bind_pipeline(p_command_buffer, &indirect_irradiance_pipeline);
				tgvk_cmd_bind_descriptor_set(p_command_buffer, &indirect_irradiance_pipeline, &indirect_irradiance_descriptor_set);
				tgvk_cmd_bind_and_draw_screen_quad(p_command_buffer);

				vkCmdEndRenderPass(p_command_buffer->command_buffer);

				SHADER_READ_TO_COLOR_ATTACHMENT_LAYERED(delta_rayleigh_scattering_texture);
			}
			tgvk_command_buffer_end_and_submit(p_command_buffer);

			for (u32 layer = 0; layer < TG_SCATTERING_TEXTURE_DEPTH; layer++)
			{
				*(i32*)geometry_ubo.memory.p_mapped_device_memory = layer;
				*(i32*)&((m4*)ubo.memory.p_mapped_device_memory)[1] = (i32)layer;

				tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
				{
					if (layer == 0)
					{
						COLOR_ATTACHMENT_TO_SHADER_READ_LAYERED(delta_scattering_density_texture);
					}

					tgvk_cmd_begin_render_pass(p_command_buffer, multiple_scattering_render_pass, &multiple_scattering_framebuffer, VK_SUBPASS_CONTENTS_INLINE);

					tgvk_cmd_bind_pipeline(p_command_buffer, &multiple_scattering_pipeline);
					tgvk_cmd_bind_descriptor_set(p_command_buffer, &multiple_scattering_pipeline, &multiple_scattering_descriptor_set);
					tgvk_cmd_bind_and_draw_screen_quad(p_command_buffer);

					tgvk_cmd_draw_indexed(p_command_buffer, 6);

					vkCmdEndRenderPass(p_command_buffer->command_buffer);

					if (layer == TG_SCATTERING_TEXTURE_DEPTH - 1)
					{
						SHADER_READ_TO_COLOR_ATTACHMENT_LAYERED(delta_scattering_density_texture);
					}
				}
				tgvk_command_buffer_end_and_submit(p_command_buffer);
			}
		}

		tgvk_descriptor_set_destroy(&multiple_scattering_descriptor_set);
		tgvk_pipeline_destroy(&multiple_scattering_pipeline);
		tgvk_shader_destroy(p_multiple_scattering_fragment_shader);
		TG_MEMORY_STACK_FREE(sizeof(*p_multiple_scattering_fragment_shader));
		tgvk_framebuffer_destroy(&multiple_scattering_framebuffer);
		tgvk_render_pass_destroy(multiple_scattering_render_pass);

		tgvk_descriptor_set_destroy(&indirect_irradiance_descriptor_set);
		tgvk_pipeline_destroy(&indirect_irradiance_pipeline);
		tgvk_shader_destroy(p_indirect_irradiance_fragment_shader);
		TG_MEMORY_STACK_FREE(sizeof(*p_indirect_irradiance_fragment_shader));
		tgvk_framebuffer_destroy(&indirect_irradiance_framebuffer);
		tgvk_render_pass_destroy(indirect_irradiance_render_pass);

		tgvk_descriptor_set_destroy(&scattering_density_descriptor_set);
		tgvk_pipeline_destroy(&scattering_density_pipeline);
		tgvk_shader_destroy(p_scattering_density_fragment_shader);
		TG_MEMORY_STACK_FREE(sizeof(*p_scattering_density_fragment_shader));
		tgvk_framebuffer_destroy(&scattering_density_framebuffer);
		tgvk_render_pass_destroy(scattering_density_render_pass);



		// recompute transmittance
		// TODO: is this even necessary?

		tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		{
			SHADER_READ_TO_COLOR_ATTACHMENT(p_model->rendering.transmittance_texture);

			tgvk_cmd_begin_render_pass(p_command_buffer, transmittance_render_pass, &transmittance_framebuffer, VK_SUBPASS_CONTENTS_INLINE);

			tgvk_cmd_bind_pipeline(p_command_buffer, &transmittance_pipeline);
			tgvk_cmd_bind_and_draw_screen_quad(p_command_buffer);

			vkCmdEndRenderPass(p_command_buffer->command_buffer);
		}
		tgvk_command_buffer_end_and_submit(p_command_buffer);

		tgvk_pipeline_destroy(&transmittance_pipeline);
		TG_MEMORY_STACK_FREE(sizeof(*p_transmittance_fragment_shader));
		tgvk_shader_destroy(p_transmittance_fragment_shader);
		tgvk_framebuffer_destroy(&transmittance_framebuffer);
		tgvk_render_pass_destroy(transmittance_render_pass);



		tgvk_buffer_destroy(&geometry_ubo);
		tgvk_buffer_destroy(&ubo);
		tgvk_semaphore_destroy(semaphore);

		tgvk_shader_destroy(p_geometry_shader);
		TG_MEMORY_STACK_FREE(sizeof(*p_geometry_shader));
		tgvk_shader_destroy(p_vertex_shader);
		TG_MEMORY_STACK_FREE(sizeof(*p_vertex_shader));
		TG_MEMORY_STACK_FREE(fragment_shader_buffer_size);
	}
	else
	{
		// This is not yet supported...
		TG_INVALID_CODEPATH();
	}

	tgvk_layered_image_destroy(&delta_scattering_density_texture);
	tgvk_layered_image_destroy(&delta_mie_scattering_texture);
	tgvk_layered_image_destroy(&delta_rayleigh_scattering_texture);
	tgvk_image_destroy(&delta_irradiance_texture);

	char* p_demo_fragment_shader_buffer = TG_MEMORY_STACK_ALLOC(p_model->api_shader.capacity);
	tg_stringf(
		p_model->api_shader.capacity,
		p_demo_fragment_shader_buffer,
		"%s\r\n%s%s\r\n",
		p_model->api_shader.p_source,
		p_model->settings.use_luminance != TG_LUMINANCE_NONE ? "#define TG_USE_LUMINANCE\r\n\r\n" : "",
		p_atmosphere_demo_fragment_shader
	);
	p_model->rendering.fragment_shader = tgvk_shader_create_from_glsl(TG_SHADER_TYPE_FRAGMENT, p_demo_fragment_shader_buffer);
	TG_MEMORY_STACK_FREE(p_model->api_shader.capacity);
}



void tgvk_atmosphere_model_create(tgvk_image* p_color_attachment, tgvk_image* p_depth_attachment, TG_OUT tgvk_atmosphere_model* p_model)
{
	p_model->settings.use_constant_solar_spectrum = TG_FALSE;
	p_model->settings.use_ozone = TG_TRUE;
	p_model->settings.use_combined_textures = TG_TRUE;
	p_model->settings.use_half_precision = TG_TRUE;
	p_model->settings.use_luminance = TG_LUMINANCE_NONE;
	p_model->settings.precomputed_wavelength_count = p_model->settings.use_luminance == TG_LUMINANCE_PRECOMPUTED ? 15 : 3;
	p_model->settings.do_white_balance = TG_FALSE;
	p_model->settings.exposure = 10.0;
	p_model->settings.combine_scattering_textures = TG_TRUE;

	tg_sampler_create_info sampler_create_info = { 0 };
	sampler_create_info.min_filter = TG_IMAGE_FILTER_LINEAR;
	sampler_create_info.mag_filter = TG_IMAGE_FILTER_LINEAR;
	sampler_create_info.address_mode_u = TG_IMAGE_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.address_mode_v = TG_IMAGE_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.address_mode_w = TG_IMAGE_ADDRESS_MODE_CLAMP_TO_EDGE;

	p_model->rendering.transmittance_texture = tgvk_image_create(TGVK_IMAGE_TYPE_COLOR, TG_TRANSMITTANCE_TEXTURE_WIDTH, TG_TRANSMITTANCE_TEXTURE_HEIGHT, VK_FORMAT_R32G32B32A32_SFLOAT, &sampler_create_info);

	VkFormat layered_image_format = VK_FORMAT_UNDEFINED;
	const VkFormatFeatureFlags layered_image_required_flags = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
	VkFormatProperties format_properties = { 0 };
	if (p_model->settings.use_half_precision)
	{
		tgvk_get_physical_device_format_properties(VK_FORMAT_R16G16B16_SFLOAT, &format_properties);
		layered_image_format = format_properties.optimalTilingFeatures & layered_image_required_flags ? VK_FORMAT_R16G16B16_SFLOAT : VK_FORMAT_R16G16B16A16_SFLOAT;
	}
	else
	{
		tgvk_get_physical_device_format_properties(VK_FORMAT_R32G32B32_SFLOAT, &format_properties);
		layered_image_format = format_properties.optimalTilingFeatures & layered_image_required_flags ? VK_FORMAT_R32G32B32_SFLOAT : VK_FORMAT_R32G32B32A32_SFLOAT;
	}

	p_model->rendering.scattering_texture = tgvk_layered_image_create(
		TGVK_IMAGE_TYPE_COLOR,
		TG_SCATTERING_TEXTURE_WIDTH, TG_SCATTERING_TEXTURE_HEIGHT, TG_SCATTERING_TEXTURE_DEPTH,
		layered_image_format, &sampler_create_info
	);

	// TODO: these two could be the same image to reduce memory consumption
	if (p_model->settings.combine_scattering_textures)
	{
		p_model->rendering.optional_single_mie_scattering_texture = (tgvk_layered_image){ 0 };
		p_model->rendering.no_single_mie_scattering_black_texture = tgvk_layered_image_create(
			TGVK_IMAGE_TYPE_COLOR,
			TG_SCATTERING_TEXTURE_WIDTH, TG_SCATTERING_TEXTURE_HEIGHT, TG_SCATTERING_TEXTURE_DEPTH,
			layered_image_format, &sampler_create_info
		);
	}
	else
	{
		p_model->rendering.optional_single_mie_scattering_texture = tgvk_layered_image_create(
			TGVK_IMAGE_TYPE_COLOR,
			TG_SCATTERING_TEXTURE_WIDTH, TG_SCATTERING_TEXTURE_HEIGHT, TG_SCATTERING_TEXTURE_DEPTH,
			layered_image_format, &sampler_create_info
		);
	}

	p_model->rendering.irradiance_texture = tgvk_image_create(TGVK_IMAGE_TYPE_COLOR, TG_IRRADIANCE_TEXTURE_WIDTH, TG_IRRADIANCE_TEXTURE_HEIGHT, VK_FORMAT_R32G32B32A32_SFLOAT, &sampler_create_info);



	tg__precompute(p_model, layered_image_format);



	p_model->rendering.vertex_shader = tgvk_shader_create_from_glsl(TG_SHADER_TYPE_VERTEX, p_atmosphere_demo_vertex_shader);

	p_model->rendering.framebuffer = tgvk_framebuffer_create(shared_render_resources.atmosphere_render_pass, 1, &p_color_attachment->image_view, p_color_attachment->width, p_color_attachment->height);

	const tg_blend_mode blend_mode = TG_BLEND_MODE_BLEND;
	tgvk_graphics_pipeline_create_info demo_graphics_pipeline_create_info = { 0 };
	demo_graphics_pipeline_create_info.p_vertex_shader = &p_model->rendering.vertex_shader;
	demo_graphics_pipeline_create_info.p_fragment_shader = &p_model->rendering.fragment_shader;
	demo_graphics_pipeline_create_info.p_blend_modes = &blend_mode;
	demo_graphics_pipeline_create_info.render_pass = shared_render_resources.atmosphere_render_pass;
	demo_graphics_pipeline_create_info.viewport_size = (v2){ (f32)p_model->rendering.framebuffer.width, (f32)p_model->rendering.framebuffer.height };
	p_model->rendering.graphics_pipeline = tgvk_pipeline_create_graphics(&demo_graphics_pipeline_create_info);

	p_model->rendering.vertex_shader_ubo = tgvk_buffer_create(2 * sizeof(m4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	p_model->rendering.fragment_shader_ubo = tgvk_buffer_create(6 * sizeof(v4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	p_model->rendering.descriptor_set = tgvk_descriptor_set_create(&p_model->rendering.graphics_pipeline);
	tgvk_descriptor_set_update_image(p_model->rendering.descriptor_set.descriptor_set, &p_model->rendering.transmittance_texture, 0);
	tgvk_descriptor_set_update_layered_image(p_model->rendering.descriptor_set.descriptor_set, &p_model->rendering.scattering_texture, 1);
	if (p_model->rendering.optional_single_mie_scattering_texture.image)
	{
		tgvk_descriptor_set_update_layered_image(p_model->rendering.descriptor_set.descriptor_set, &p_model->rendering.optional_single_mie_scattering_texture, 2);
	}
	else
	{
		tgvk_descriptor_set_update_layered_image(p_model->rendering.descriptor_set.descriptor_set, &p_model->rendering.no_single_mie_scattering_black_texture, 2);
	}
	tgvk_descriptor_set_update_image(p_model->rendering.descriptor_set.descriptor_set, &p_model->rendering.irradiance_texture, 3);
	tgvk_descriptor_set_update_uniform_buffer(p_model->rendering.descriptor_set.descriptor_set, p_model->rendering.vertex_shader_ubo.buffer, 4);
	tgvk_descriptor_set_update_uniform_buffer(p_model->rendering.descriptor_set.descriptor_set, p_model->rendering.fragment_shader_ubo.buffer, 5);
	tgvk_descriptor_set_update_image(p_model->rendering.descriptor_set.descriptor_set, p_depth_attachment, 6);

	const f32 exposure = 10.0f;
	const f32 white_point_r = 1.0f;
	const f32 white_point_g = 1.0f;
	const f32 white_point_b = 1.0f;
	const v3 earth_center = { 0.0f, (f32)(-TG_BOTTOM_RADIUS / TG_LENGTH_UNIT_IN_METERS) + 200.0f, 0.0f };
	const v2 sun_size = { tgm_f32_tan((f32)TG_SUN_ANGULAR_RADIUS), tgm_f32_cos((f32)TG_SUN_ANGULAR_RADIUS) };

	// TODO:
	//if (do_white_balance)
	//{
	//	...
	//}

	((v4*)p_model->rendering.fragment_shader_ubo.memory.p_mapped_device_memory)[1].x = p_model->settings.use_luminance != TG_LUMINANCE_NONE ? exposure * 1e-5f : exposure;
	((v4*)p_model->rendering.fragment_shader_ubo.memory.p_mapped_device_memory)[2].xyz = (v3){ white_point_r, white_point_g, white_point_b };
	((v4*)p_model->rendering.fragment_shader_ubo.memory.p_mapped_device_memory)[3].xyz = earth_center;
	((v4*)p_model->rendering.fragment_shader_ubo.memory.p_mapped_device_memory)[5].xy = sun_size;

	p_model->rendering.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	tgvk_command_buffer_begin(&p_model->rendering.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	{
		tgvk_cmd_transition_image_layout(
			&p_model->rendering.command_buffer,
			&p_model->rendering.transmittance_texture,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		);
		tgvk_cmd_transition_layered_image_layout(
			&p_model->rendering.command_buffer,
			&p_model->rendering.scattering_texture,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		);
		if (p_model->rendering.optional_single_mie_scattering_texture.image)
		{
			tgvk_cmd_transition_layered_image_layout(
				&p_model->rendering.command_buffer,
				&p_model->rendering.optional_single_mie_scattering_texture,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
			);
		}
		else
		{
			tgvk_cmd_transition_layered_image_layout(
				&p_model->rendering.command_buffer,
				&p_model->rendering.no_single_mie_scattering_black_texture,
				0,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT
			);
			tgvk_cmd_clear_layered_image(&p_model->rendering.command_buffer, &p_model->rendering.no_single_mie_scattering_black_texture);
			tgvk_cmd_transition_layered_image_layout(
				&p_model->rendering.command_buffer,
				&p_model->rendering.no_single_mie_scattering_black_texture,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
			);
		}
		tgvk_cmd_transition_image_layout(
			&p_model->rendering.command_buffer,
			&p_model->rendering.irradiance_texture,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		);
	}
	tgvk_command_buffer_end_and_submit(&p_model->rendering.command_buffer);

	tgvk_command_buffer_begin(&p_model->rendering.command_buffer, 0);
	{
		tgvk_cmd_transition_image_layout(
			&p_model->rendering.command_buffer,
			p_depth_attachment,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		);

		tgvk_cmd_begin_render_pass(&p_model->rendering.command_buffer, shared_render_resources.atmosphere_render_pass, &p_model->rendering.framebuffer, VK_SUBPASS_CONTENTS_INLINE);

		tgvk_cmd_bind_descriptor_set(&p_model->rendering.command_buffer, &p_model->rendering.graphics_pipeline, &p_model->rendering.descriptor_set);
		tgvk_cmd_bind_pipeline(&p_model->rendering.command_buffer, &p_model->rendering.graphics_pipeline);
		tgvk_cmd_bind_and_draw_screen_quad(&p_model->rendering.command_buffer);

		vkCmdEndRenderPass(p_model->rendering.command_buffer.command_buffer);

		tgvk_cmd_transition_image_layout(
			&p_model->rendering.command_buffer,
			p_depth_attachment,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
		);
	}
	vkEndCommandBuffer(p_model->rendering.command_buffer.command_buffer);
}

void tgvk_atmosphere_model_destroy(tgvk_atmosphere_model* p_model)
{
	tgvk_command_buffer_destroy(&p_model->rendering.command_buffer);
	tgvk_buffer_destroy(&p_model->rendering.fragment_shader_ubo);
	tgvk_buffer_destroy(&p_model->rendering.vertex_shader_ubo);
	tgvk_descriptor_set_destroy(&p_model->rendering.descriptor_set);
	tgvk_pipeline_destroy(&p_model->rendering.graphics_pipeline);
	tgvk_framebuffer_destroy(&p_model->rendering.framebuffer);
	tgvk_shader_destroy(&p_model->rendering.fragment_shader);
	tgvk_shader_destroy(&p_model->rendering.vertex_shader);

	if (p_model->api_shader.p_source)
	{
		TG_MEMORY_FREE(p_model->api_shader.p_source);
	}

	tgvk_image_destroy(&p_model->rendering.irradiance_texture);
	if (p_model->settings.combine_scattering_textures)
	{
		tgvk_layered_image_destroy(&p_model->rendering.no_single_mie_scattering_black_texture);
	}
	else
	{
		tgvk_layered_image_destroy(&p_model->rendering.optional_single_mie_scattering_texture);
	}
	tgvk_layered_image_destroy(&p_model->rendering.scattering_texture);
	tgvk_image_destroy(&p_model->rendering.transmittance_texture);
}

void tgvk_atmosphere_model_update(tgvk_atmosphere_model* p_model, v3 sun_direction, m4 inv_view, m4 inv_proj)
{
	((m4*)p_model->rendering.vertex_shader_ubo.memory.p_mapped_device_memory)[0] = inv_view;
	((m4*)p_model->rendering.vertex_shader_ubo.memory.p_mapped_device_memory)[1] = inv_proj;
	((v4*)p_model->rendering.fragment_shader_ubo.memory.p_mapped_device_memory)[0].xyz = (v3){ inv_view.m03, inv_view.m13, inv_view.m23 };
	((v4*)p_model->rendering.fragment_shader_ubo.memory.p_mapped_device_memory)[4].xyz = tgm_v3_neg(sun_direction);
}



#undef SHADER_READ_TO_COLOR_ATTACHMENT_LAYERED
#undef SHADER_READ_TO_COLOR_ATTACHMENT
#undef COLOR_ATTACHMENT_TO_SHADER_READ_LAYERED
#undef COLOR_ATTACHMENT_TO_SHADER_READ
#undef TO_COLOR_ATTACHMENT_LAYOUT_LAYERED
#undef TO_COLOR_ATTACHMENT_LAYOUT
#undef TG_DEBUG_STORE_LAYERED_IMAGE_SLICE
#undef TG_DEBUG_STORE_IMAGE

#endif
