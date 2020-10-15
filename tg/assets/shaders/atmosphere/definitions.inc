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

#define tg_length            float
#define tg_wavelength        float
#define tg_angle             float
#define tg_solid_angle       float
#define tg_power             float
#define tg_luminous_power    float

#define tg_number                       float
#define tg_inverse_length               float
#define tg_area                         float
#define tg_volume                       float
#define tg_number_density               float
#define tg_irradiance                   float
#define tg_radiance                     float
#define tg_spectral_power               float
#define tg_spectral_irradiance          float
#define tg_spectral_radiance            float
#define tg_spectral_radiance_density    float
#define tg_scattering_coefficient       float
#define tg_inverse_solid_angle          float
#define tg_luminous_intensity           float
#define tg_luminance                    float
#define tg_illuminance                  float

#define tg_abstract_spectrum            vec3
#define tg_dimensionless_spectrum       vec3
#define tg_power_spectrum               vec3
#define tg_irradiance_spectrum          vec3
#define tg_radiance_spectrum            vec3
#define tg_radiance_density_spectrum    vec3
#define tg_scattering_spectrum          vec3

#define tg_position        vec3
#define tg_direction       vec3
#define tg_luminance3      vec3
#define tg_illuminance3    vec3

#define tg_transmittance_texture          sampler2D
#define tg_abstract_scattering_texture    sampler3D
#define tg_reduced_scattering_texture     sampler3D
#define tg_scattering_texture             sampler3D
#define tg_scatteringDensity_texture      sampler3D
#define tg_irradiance_texture             sampler2D

const tg_length            m    = 1.0;
const tg_wavelength        nm   = 1.0;
const tg_angle             rad  = 1.0;
const tg_solid_angle       sr   = 1.0;
const tg_power             watt = 1.0;
const tg_luminous_power    lm   = 1.0;

const float TG_PI = 3.14159265358979323846;

const tg_length                       km                                  = 1000.0 * m;
const tg_area                         m2                                  = m * m;
const tg_volume                       m3                                  = m * m * m;
const tg_angle                        pi                                  = PI * rad;
const tg_angle                        deg                                 = pi / 180.0;
const tg_irradiance                   watt_per_square_meter               = watt / m2;
const tg_radiance                     watt_per_square_meter_per_sr        = watt / (m2 * sr);
const tg_spectral_irradiance          watt_per_square_meter_per_nm        = watt / (m2 * nm);
const tg_spectral_radiance            watt_per_square_meter_per_sr_per_nm = watt / (m2 * sr * nm);
const tg_spectral_radiance_density    watt_per_cubic_meter_per_sr_per_nm  = watt / (m3 * sr * nm);
const tg_luminous_intensity           cd                                  = lm / sr;
const tg_luminous_intensity           kcd                                 = 1000.0 * cd;
const tg_luminance                    cd_per_square_meter                 = cd / m2;
const tg_luminance                    kcd_per_square_meter                = kcd / m2;

struct tg_density_profile_layer
{
  tg_length            width;
  tg_number            exp_term;
  tg_inverse_length    exp_scale;
  tg_inverse_length    linear_term;
  tg_number            constant_term;
};

struct tg_density_profile
{
  tg_density_profile_layer    layers[2];
};

struct tg_atmosphere_parameters
{
  tg_irradiance_spectrum       solar_irradiance;
  tg_angle                     sun_angular_radius;
  tg_length                    bottom_radius;
  tg_length                    top_radius;
  tg_density_profile           rayleigh_density;
  tg_scattering_spectrum       rayleigh_scattering;
  tg_density_profile           mie_density;
  tg_scattering_spectrum       mie_scattering;
  tg_scattering_spectrum       mie_extinction;
  tg_number                    mie_phase_function_g;
  tg_density_profile           absorption_density;
  tg_scattering_spectrum       absorption_extinction;
  tg_dimensionless_spectrum    ground_albedo;
  tg_number                    mu_s_min;
};
