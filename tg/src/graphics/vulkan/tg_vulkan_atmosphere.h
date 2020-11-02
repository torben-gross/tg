#ifndef TG_VULKAN_ATMOSPHERE_H
#define TG_VULKAN_ATMOSPHERE_H

#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN



typedef enum tg_luminance
{
	TG_LUMINANCE_NONE,
	TG_LUMINANCE_APPROXIMATE,
	TG_LUMINANCE_PRECOMPUTED
} tg_luminance;



typedef struct tg_density_profile_layer
{
	f64    width;
	f64    exp_term;
	f64    exp_scale;
	f64    linear_term;
	f64    constant_term;
} tg_density_profile_layer;

typedef struct tg_model
{
	tg_density_profile_layer    rayleigh_density_profile_layer;
	tg_density_profile_layer    mie_density_profile_layer;
	tg_density_profile_layer    p_absorption_density_profile_layers[2];

	b32                         use_constant_solar_spectrum;
	b32                         use_ozone;
	b32                         use_combined_textures;
	b32                         use_half_precision;
	tg_luminance                use_luminance;
	b32                         do_white_balance;
	f64                         exposure;

	f64                         p_wavelengths[48];
	f64                         p_solar_irradiances[48];
	f64                         p_rayleigh_scatterings[48];
	f64                         p_mie_scatterings[48];
	f64                         p_mie_extinctions[48];
	f64                         p_absorption_extinctions[48];
	f64                         p_ground_albedos[48];

	u32                         precomputed_wavelength_count;
	f64                         sky_k_r;
	f64                         sky_k_g;
	f64                         sky_k_b;
	f64                         sun_k_r;
	f64                         sun_k_g;
	f64                         sun_k_b;

	b32                         combine_scattering_textures;
	b32                         rgb_format_supported;

	struct
	{
		u32                     capacity;
		u32                     header_count;
		u32                     total_count;
		char*                   p_source;
	} shader;

	tgvk_image                  transmittance_texture;
	tgvk_layered_image          scattering_texture;
	tgvk_layered_image          optional_single_mie_scattering_texture;
	tgvk_image                  irradiance_texture;
} tg_model;

#endif

#endif
