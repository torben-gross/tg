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

#ifndef TG_VULKAN_ATMOSPHERE_DEFINITIONS_H
#define TG_VULKAN_ATMOSPHERE_DEFINITIONS_H

static const char p_atmosphere_definitions[] =
"#define tg_length            float\r\n"
"#define tg_wavelength        float\r\n"
"#define tg_angle             float\r\n"
"#define tg_solid_angle       float\r\n"
"#define tg_power             float\r\n"
"#define tg_luminous_power    float\r\n"
"\r\n"
"#define tg_number                       float\r\n"
"#define tg_inverse_length               float\r\n"
"#define tg_area                         float\r\n"
"#define tg_volume                       float\r\n"
"#define tg_number_density               float\r\n"
"#define tg_irradiance                   float\r\n"
"#define tg_radiance                     float\r\n"
"#define tg_spectral_power               float\r\n"
"#define tg_spectral_irradiance          float\r\n"
"#define tg_spectral_radiance            float\r\n"
"#define tg_spectral_radiance_density    float\r\n"
"#define tg_scattering_coefficient       float\r\n"
"#define tg_inverse_solid_angle          float\r\n"
"#define tg_luminous_intensity           float\r\n"
"#define tg_luminance                    float\r\n"
"#define tg_illuminance                  float\r\n"
"\r\n"
"#define tg_abstract_spectrum            vec3\r\n"
"#define tg_dimensionless_spectrum       vec3\r\n"
"#define tg_power_spectrum               vec3\r\n"
"#define tg_irradiance_spectrum          vec3\r\n"
"#define tg_radiance_spectrum            vec3\r\n"
"#define tg_radiance_density_spectrum    vec3\r\n"
"#define tg_scattering_spectrum          vec3\r\n"
"\r\n"
"#define tg_position        vec3\r\n"
"#define tg_direction       vec3\r\n"
"#define tg_luminance3      vec3\r\n"
"#define tg_illuminance3    vec3\r\n"
"\r\n"
"#define tg_transmittance_texture          sampler2D\r\n"
"#define tg_abstract_scattering_texture    sampler3D\r\n"
"#define tg_reduced_scattering_texture     sampler3D\r\n"
"#define tg_scattering_texture             sampler3D\r\n"
"#define tg_scattering_density_texture     sampler3D\r\n"
"#define tg_irradiance_texture             sampler2D\r\n"
"\r\n"
"const tg_length            m    = 1.0;\r\n"
"const tg_wavelength        nm   = 1.0;\r\n"
"const tg_angle             rad  = 1.0;\r\n"
"const tg_solid_angle       sr   = 1.0;\r\n"
"const tg_power             watt = 1.0;\r\n"
"const tg_luminous_power    lm   = 1.0;\r\n"
"\r\n"
"const float TG_PI = 3.14159265358979323846;\r\n"
"\r\n"
"const tg_length                       km                                  = 1000.0 * m;\r\n"
"const tg_area                         m2                                  = m * m;\r\n"
"const tg_volume                       m3                                  = m * m * m;\r\n"
"const tg_angle                        pi                                  = TG_PI * rad;\r\n"
"const tg_angle                        deg                                 = pi / 180.0;\r\n"
"const tg_irradiance                   watt_per_square_meter               = watt / m2;\r\n"
"const tg_radiance                     watt_per_square_meter_per_sr        = watt / (m2 * sr);\r\n"
"const tg_spectral_irradiance          watt_per_square_meter_per_nm        = watt / (m2 * nm);\r\n"
"const tg_spectral_radiance            watt_per_square_meter_per_sr_per_nm = watt / (m2 * sr * nm);\r\n"
"const tg_spectral_radiance_density    watt_per_cubic_meter_per_sr_per_nm  = watt / (m3 * sr * nm);\r\n"
"const tg_luminous_intensity           cd                                  = lm / sr;\r\n"
"const tg_luminous_intensity           kcd                                 = 1000.0 * cd;\r\n"
"const tg_luminance                    cd_per_square_meter                 = cd / m2;\r\n"
"const tg_luminance                    kcd_per_square_meter                = kcd / m2;\r\n"
"\r\n"
"struct tg_density_profile_layer\r\n"
"{\r\n"
"    tg_length            width;\r\n"
"    tg_number            exp_term;\r\n"
"    tg_inverse_length    exp_scale;\r\n"
"    tg_inverse_length    linear_term;\r\n"
"    tg_number            constant_term;\r\n"
"};\r\n"
"\r\n"
"struct tg_density_profile\r\n"
"{\r\n"
"    tg_density_profile_layer    layers[2];\r\n"
"};\r\n"
"\r\n"
"struct tg_atmosphere_parameters\r\n"
"{\r\n"
"    tg_irradiance_spectrum       solar_irradiance;\r\n"
"    tg_angle                     sun_angular_radius;\r\n"
"    tg_length                    bottom_radius;\r\n"
"    tg_length                    top_radius;\r\n"
"    tg_density_profile           rayleigh_density;\r\n"
"    tg_scattering_spectrum       rayleigh_scattering;\r\n"
"    tg_density_profile           mie_density;\r\n"
"    tg_scattering_spectrum       mie_scattering;\r\n"
"    tg_scattering_spectrum       mie_extinction;\r\n"
"    tg_number                    mie_phase_function_g;\r\n"
"    tg_density_profile           absorption_density;\r\n"
"    tg_scattering_spectrum       absorption_extinction;\r\n"
"    tg_dimensionless_spectrum    ground_albedo;\r\n"
"    tg_number                    mu_s_min;\r\n"
"};\r\n";

#endif
