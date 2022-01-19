#ifndef TGVK_ATMOSPHERE_H
#define TGVK_ATMOSPHERE_H

#include "graphics/vulkan/tgvk_core.h"



typedef enum tgvk_atmosphere_luminance
{
    TG_LUMINANCE_NONE,
    TG_LUMINANCE_APPROXIMATE,
    TG_LUMINANCE_PRECOMPUTED
} tgvk_atmosphere_luminance;



typedef struct tg_atmosphere_model
{
    struct
    {
        b32                          use_constant_solar_spectrum;
        b32                          use_ozone;
        b32                          use_combined_textures;
        b32                          use_half_precision;
        tgvk_atmosphere_luminance    use_luminance;
        u32                          precomputed_wavelength_count;
        b32                          combine_scattering_textures;
    } settings;

    struct
    {
        u32                          capacity;
        u32                          header_count;
        u32                          total_count;
        char* p_source;
    } api_shader;

    struct
    {
        tgvk_image                   transmittance_texture;
        tgvk_layered_image           scattering_texture;
        tgvk_layered_image           optional_single_mie_scattering_texture;
        tgvk_layered_image           no_single_mie_scattering_black_texture; // TODO: remove this and change 'p_atmosphere_shader' to not use 'single_mie_scattering_texture'
        tgvk_image                   irradiance_texture;
        tgvk_buffer                  vertex_shader_ubo;
        tgvk_buffer                  ubo;
    } rendering;
} tg_atmosphere_model;



void    tgvk_atmosphere_model_create(TG_OUT tg_atmosphere_model* p_model);
void    tgvk_atmosphere_model_destroy(tg_atmosphere_model* p_model);
void    tgvk_atmosphere_model_update_descriptor_set(tg_atmosphere_model* p_model, tgvk_descriptor_set* p_descriptor_set);
void    tgvk_atmosphere_model_update(tg_atmosphere_model* p_model, m4 inv_view, m4 inv_proj);

#endif
