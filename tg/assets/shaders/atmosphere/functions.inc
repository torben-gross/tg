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
 * ARE DISCLAIMED. TG_IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER TG_IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING TG_IN ANY WAY TG_OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Precomputed Atmospheric Scattering
 * Copyright (c) 2008 INRIA
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
 * ARE DISCLAIMED. TG_IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER TG_IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING TG_IN ANY WAY TG_OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

tg_length tg_clamp_cosine(tg_length mu)
{
    return clamp(mu, tg_length(-1.0), tg_length(1.0));
}

tg_length tg_clamp_distance(tg_length d)
{
    return max(d, 0.0 * m);
}

tg_length tg_clamp_radius(TG_IN(tg_atmosphere_parameters) atmosphere, tg_length r)
{
    return clamp(r, atmosphere.bottom_radius, atmosphere.top_radius);
}

tg_length tg_safe_sqrt(tg_area a)
{
    return sqrt(max(a, 0.0 * m2));
}

tg_length tg_distance_to_top_atmosphere_boundary(TG_IN(tg_atmosphere_parameters) atmosphere, tg_length r, tg_length mu)
{
    TG_ASSERT(r <= atmosphere.top_radius);
    TG_ASSERT(mu >= -1.0 && mu <= 1.0);
	
    tg_area discriminant = r * r * (mu * mu - 1.0) + atmosphere.top_radius * atmosphere.top_radius;
    return tg_clamp_distance(-r * mu + tg_safe_sqrt(discriminant));
}

tg_length tg_distance_to_bottom_atmosphere_boundary(TG_IN(tg_atmosphere_parameters) atmosphere, tg_length r, tg_length mu)
{
    TG_ASSERT(r >= atmosphere.bottom_radius);
    TG_ASSERT(mu >= -1.0 && mu <= 1.0);
	
    tg_area discriminant = r * r * (mu * mu - 1.0) + atmosphere.bottom_radius * atmosphere.bottom_radius;
    return tg_clamp_distance(-r * mu - tg_safe_sqrt(discriminant));
}

bool tg_ray_intersects_ground(TG_IN(tg_atmosphere_parameters) atmosphere, tg_length r, tg_length mu)
{
    TG_ASSERT(r >= atmosphere.bottom_radius);
    TG_ASSERT(mu >= -1.0 && mu <= 1.0);
    
	return mu < 0.0 && r * r * (mu * mu - 1.0) + atmosphere.bottom_radius * atmosphere.bottom_radius >= 0.0 * m2;
}

tg_length tg_get_layer_density(TG_IN(tg_density_profile_layer) layer, tg_length altitude) {
    tg_length density = layer.exp_term * exp(layer.exp_scale * altitude) + layer.linear_term * altitude + layer.constant_term;
    return clamp(density, tg_length(0.0), tg_length(1.0));
}

tg_length tg_get_profile_density(TG_IN(tg_density_profile) profile, tg_length altitude)
{
    return altitude < profile.layers[0].width ?
	    tg_get_layer_density(profile.layers[0], altitude) :
        tg_get_layer_density(profile.layers[1], altitude);
}

tg_length tg_compute_optical_length_to_top_atmosphere_boundary(TG_IN(tg_atmosphere_parameters) atmosphere, TG_IN(tg_density_profile) profile, tg_length r, tg_length mu)
{
    TG_ASSERT(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
    TG_ASSERT(mu >= -1.0 && mu <= 1.0);
	
    const int TG_SAMPLE_COUNT = 500;
    tg_length dx = tg_distance_to_top_atmosphere_boundary(atmosphere, r, mu) / tg_length(TG_SAMPLE_COUNT);
    tg_length result = 0.0 * m;
	
    for (int i = 0; i <= TG_SAMPLE_COUNT; ++i)
	{
        tg_length d_i = tg_length(i) * dx;
        tg_length r_i = sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r);
        tg_length y_i = tg_get_profile_density(profile, r_i - atmosphere.bottom_radius);
        tg_length weight_i = i == 0 || i == TG_SAMPLE_COUNT ? 0.5 : 1.0;
        result += y_i * weight_i * dx;
    }
	
    return result;
}

tg_dimensionless_spectrum tg_compute_transmittance_top_to_atmosphere_boundary(TG_IN(tg_atmosphere_parameters) atmosphere, tg_length r, tg_length mu)
{
    TG_ASSERT(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
    TG_ASSERT(mu >= -1.0 && mu <= 1.0);
    
    return exp(-(
        atmosphere.rayleigh_scattering *
            tg_compute_optical_length_to_top_atmosphere_boundary(atmosphere, atmosphere.rayleigh_density, r, mu) +
        atmosphere.mie_extinction *
            tg_compute_optical_length_to_top_atmosphere_boundary(atmosphere, atmosphere.mie_density, r, mu) +
        atmosphere.absorption_extinction *
            tg_compute_optical_length_to_top_atmosphere_boundary(atmosphere, atmosphere.absorption_density, r, mu)
        )
    );
}

tg_length tg_get_texture_coord_from_unit_range(tg_length x, int texture_size)
{
    return 0.5 / tg_length(texture_size) + x * (1.0 - 1.0 / tg_length(texture_size));
}

tg_length tg_get_unit_range_from_texture_coord(tg_length u, int texture_size)
{
    return (u - 0.5 / tg_length(texture_size)) / (1.0 - 1.0 / tg_length(texture_size));
}

vec2 tg_get_transmittance_texture_uv_from_r_mu(TG_IN(tg_atmosphere_parameters) atmosphere, tg_length r, tg_length mu)
{
    TG_ASSERT(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
    TG_ASSERT(mu >= -1.0 && mu <= 1.0);
	
    tg_length H = sqrt(atmosphere.top_radius * atmosphere.top_radius - atmosphere.bottom_radius * atmosphere.bottom_radius);
    tg_length rho = tg_safe_sqrt(r * r - atmosphere.bottom_radius * atmosphere.bottom_radius);
    tg_length d = tg_distance_to_top_atmosphere_boundary(atmosphere, r, mu);
    tg_length d_min = atmosphere.top_radius - r;
    tg_length d_max = rho + H;
    tg_length x_mu = (d - d_min) / (d_max - d_min);
    tg_length x_r = rho / H;
    return vec2(
	    tg_get_texture_coord_from_unit_range(x_mu, TG_TRANSMITTANCE_TEXTURE_WIDTH),
		tg_get_texture_coord_from_unit_range(x_r, TG_TRANSMITTANCE_TEXTURE_HEIGHT)
	);
}

void tg_get_r_mu_from_transmittance_texture_uv(TG_IN(tg_atmosphere_parameters) atmosphere, TG_IN(vec2) uv, TG_OUT(tg_length) r, TG_OUT(tg_length) mu)
{
    TG_ASSERT(uv.x >= 0.0 && uv.x <= 1.0);
    TG_ASSERT(uv.y >= 0.0 && uv.y <= 1.0);
    
    tg_length x_mu = tg_get_unit_range_from_texture_coord(uv.x, TG_TRANSMITTANCE_TEXTURE_WIDTH);
    tg_length x_r = tg_get_unit_range_from_texture_coord(uv.y, TG_TRANSMITTANCE_TEXTURE_HEIGHT);
    tg_length H = sqrt(atmosphere.top_radius * atmosphere.top_radius - atmosphere.bottom_radius * atmosphere.bottom_radius);
    tg_length rho = H * x_r;
    r = sqrt(rho * rho + atmosphere.bottom_radius * atmosphere.bottom_radius);
    tg_length d_min = atmosphere.top_radius - r;
    tg_length d_max = rho + H;
    tg_length d = d_min + x_mu * (d_max - d_min);
    mu = d == 0.0 * m ? tg_length(1.0) : (H * H - rho * rho - d * d) / (2.0 * r * d);
    mu = tg_clamp_cosine(mu);
}

tg_dimensionless_spectrum tg_compute_transmittance_to_top_atmosphere_boundary_texture(TG_IN(tg_atmosphere_parameters) atmosphere, TG_IN(vec2) frag_coord)
{
    const vec2 TG_TRANSMITTANCE_TEXTURE_SIZE = vec2(TG_TRANSMITTANCE_TEXTURE_WIDTH, TG_TRANSMITTANCE_TEXTURE_HEIGHT);
    tg_length r;
    tg_length mu;
    tg_get_r_mu_from_transmittance_texture_uv(atmosphere, frag_coord / TG_TRANSMITTANCE_TEXTURE_SIZE, r, mu);
    return tg_compute_transmittance_top_to_atmosphere_boundary(atmosphere, r, mu);
}

tg_dimensionless_spectrum tg_get_transmittance_to_top_atmosphere_boundary(TG_IN(tg_atmosphere_parameters) atmosphere, TG_IN(tg_transmittance_texture) transmittance_texture, tg_length r, tg_length mu)
{
    TG_ASSERT(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
    
    vec2 uv = tg_get_transmittance_texture_uv_from_r_mu(atmosphere, r, mu);
    return tg_dimensionless_spectrum(texture(transmittance_texture, uv));
}

tg_dimensionless_spectrum tg_get_transmittance(TG_IN(tg_atmosphere_parameters) atmosphere, TG_IN(tg_transmittance_texture) transmittance_texture, tg_length r, tg_length mu, tg_length d, bool ray_r_mu_intersects_ground)
{
    TG_ASSERT(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
    TG_ASSERT(mu >= -1.0 && mu <= 1.0);
    TG_ASSERT(d >= 0.0 * m);
    
    tg_length r_d = tg_clamp_radius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
    tg_length mu_d = tg_clamp_cosine((r * mu + d) / r_d);
    
    if (ray_r_mu_intersects_ground)
    {
      return min(
          tg_get_transmittance_to_top_atmosphere_boundary(
              atmosphere, transmittance_texture, r_d, -mu_d) /
          tg_get_transmittance_to_top_atmosphere_boundary(
              atmosphere, transmittance_texture, r, -mu),
          tg_dimensionless_spectrum(1.0));
    }
	else
    {
      return min(
          tg_get_transmittance_to_top_atmosphere_boundary(
              atmosphere, transmittance_texture, r, mu) /
          tg_get_transmittance_to_top_atmosphere_boundary(
              atmosphere, transmittance_texture, r_d, mu_d),
          tg_dimensionless_spectrum(1.0));
    }
}

tg_dimensionless_spectrum tg_get_transmittance_to_sun(TG_IN(tg_atmosphere_parameters) atmosphere, TG_IN(tg_transmittance_texture) transmittance_texture, tg_length r, tg_length mu_s)
{
    tg_length sin_theta_h = atmosphere.bottom_radius / r;
    tg_length cos_theta_h = -sqrt(max(1.0 - sin_theta_h * sin_theta_h, 0.0));
    return tg_get_transmittance_to_top_atmosphere_boundary(
	    atmosphere, transmittance_texture, r, mu_s) *
        smoothstep(-sin_theta_h * atmosphere.sun_angular_radius / rad, sin_theta_h * atmosphere.sun_angular_radius / rad, mu_s - cos_theta_h
	);
}

void tg_compute_single_scattering_integrand(
    TG_IN(tg_atmosphere_parameters)      atmosphere,
	TG_IN(tg_transmittance_texture)      transmittance_texture,
	tg_length                            r,
	tg_length                            mu,
	tg_length                            mu_s,
	tg_length                            nu,
	tg_length                            d,
	bool                                 ray_r_mu_intersects_ground,
	TG_OUT(tg_dimensionless_spectrum)    rayleigh,
	TG_OUT(tg_dimensionless_spectrum)    mie
)
{
    tg_length r_d = tg_clamp_radius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
    tg_length mu_s_d = tg_clamp_cosine((r * mu_s + d * nu) / r_d);
    tg_dimensionless_spectrum transmittance =
        tg_get_transmittance(
            atmosphere, transmittance_texture, r, mu, d,
            ray_r_mu_intersects_ground) *
        tg_get_transmittance_to_sun(
            atmosphere, transmittance_texture, r_d, mu_s_d);
    rayleigh = transmittance * tg_get_profile_density(atmosphere.rayleigh_density, r_d - atmosphere.bottom_radius);
    mie = transmittance * tg_get_profile_density(atmosphere.mie_density, r_d - atmosphere.bottom_radius);
}

tg_length tg_distance_to_nearest_atmosphere_boundary(TG_IN(tg_atmosphere_parameters) atmosphere, tg_length r, tg_length mu, bool ray_r_mu_intersects_ground)
{
    if (ray_r_mu_intersects_ground)
    {
      return tg_distance_to_bottom_atmosphere_boundary(atmosphere, r, mu);
    }
    else
    {
      return tg_distance_to_top_atmosphere_boundary(atmosphere, r, mu);
    }
}

void tg_compute_single_scattering(
    TG_IN(tg_atmosphere_parameters)    atmosphere,
    TG_IN(tg_transmittance_texture)    transmittance_texture,
    tg_length                          r,
	tg_length                          mu,
	tg_length                          mu_s,
	tg_length                          nu,
    bool                               ray_r_mu_intersects_ground,
    TG_OUT(tg_irradiance_spectrum)     rayleigh,
	TG_OUT(tg_irradiance_spectrum)     mie
)
{
    TG_ASSERT(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
    TG_ASSERT(mu >= -1.0 && mu <= 1.0);
    TG_ASSERT(mu_s >= -1.0 && mu_s <= 1.0);
    TG_ASSERT(nu >= -1.0 && nu <= 1.0);
    
    const int TG_SAMPLE_COUNT = 50;
    tg_length dx = tg_distance_to_nearest_atmosphere_boundary(atmosphere, r, mu, ray_r_mu_intersects_ground) / tg_length(TG_SAMPLE_COUNT);
    tg_dimensionless_spectrum rayleigh_sum = tg_dimensionless_spectrum(0.0);
    tg_dimensionless_spectrum mie_sum = tg_dimensionless_spectrum(0.0);
    for (int i = 0; i <= TG_SAMPLE_COUNT; ++i)
    {
      tg_length d_i = tg_length(i) * dx;
      tg_dimensionless_spectrum rayleigh_i;
      tg_dimensionless_spectrum mie_i;
      tg_compute_single_scattering_integrand(atmosphere, transmittance_texture, r, mu, mu_s, nu, d_i, ray_r_mu_intersects_ground, rayleigh_i, mie_i);
      tg_length weight_i = (i == 0 || i == TG_SAMPLE_COUNT) ? 0.5 : 1.0;
      rayleigh_sum += rayleigh_i * weight_i;
      mie_sum += mie_i * weight_i;
    }
    rayleigh = rayleigh_sum * dx * atmosphere.solar_irradiance * atmosphere.rayleigh_scattering;
    mie = mie_sum * dx * atmosphere.solar_irradiance * atmosphere.mie_scattering;
}

tg_inverse_solid_angle tg_rayleigh_phase_function(tg_length nu)
{
    tg_inverse_solid_angle k = 3.0 / (16.0 * TG_PI * sr);
    return k * (1.0 + nu * nu);
}

tg_inverse_solid_angle tg_mie_phase_function(tg_length g, tg_length nu)
{
    tg_inverse_solid_angle k = 3.0 / (8.0 * TG_PI * sr) * (1.0 - g * g) / (2.0 + g * g);
    return k * (1.0 + nu * nu) / pow(1.0 + g * g - 2.0 * g * nu, 1.5);
}

vec4 tg_get_scattering_texture_uvwz_from_r_mu_mu_s_nu(TG_IN(tg_atmosphere_parameters) atmosphere, tg_length r, tg_length mu, tg_length mu_s, tg_length nu, bool ray_r_mu_intersects_ground)
{
    TG_ASSERT(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
    TG_ASSERT(mu >= -1.0 && mu <= 1.0);
    TG_ASSERT(mu_s >= -1.0 && mu_s <= 1.0);
    TG_ASSERT(nu >= -1.0 && nu <= 1.0);
    
    tg_length H = sqrt(atmosphere.top_radius * atmosphere.top_radius - atmosphere.bottom_radius * atmosphere.bottom_radius);
    tg_length rho = tg_safe_sqrt(r * r - atmosphere.bottom_radius * atmosphere.bottom_radius);
    tg_length u_r = tg_get_texture_coord_from_unit_range(rho / H, TG_SCATTERING_TEXTURE_R_SIZE);
    
    tg_length r_mu = r * mu;
    tg_area discriminant = r_mu * r_mu - r * r + atmosphere.bottom_radius * atmosphere.bottom_radius;
    tg_length u_mu;
    if (ray_r_mu_intersects_ground)
    {
        tg_length d = -r_mu - tg_safe_sqrt(discriminant);
        tg_length d_min = r - atmosphere.bottom_radius;
        tg_length d_max = rho;
        u_mu = 0.5 - 0.5 * tg_get_texture_coord_from_unit_range(d_max == d_min ? 0.0 : (d - d_min) / (d_max - d_min), TG_SCATTERING_TEXTURE_MU_SIZE / 2);
    }
    else
    {
        tg_length d = -r_mu + tg_safe_sqrt(discriminant + H * H);
        tg_length d_min = atmosphere.top_radius - r;
        tg_length d_max = rho + H;
        u_mu = 0.5 + 0.5 * tg_get_texture_coord_from_unit_range((d - d_min) / (d_max - d_min), TG_SCATTERING_TEXTURE_MU_SIZE / 2);
    }
    
    tg_length d = tg_distance_to_top_atmosphere_boundary(atmosphere, atmosphere.bottom_radius, mu_s);
    tg_length d_min = atmosphere.top_radius - atmosphere.bottom_radius;
    tg_length d_max = H;
    tg_length a = (d - d_min) / (d_max - d_min);
    tg_length A = -2.0 * atmosphere.mu_s_min * atmosphere.bottom_radius / (d_max - d_min);
    tg_length u_mu_s = tg_get_texture_coord_from_unit_range(max(1.0 - a / A, 0.0) / (1.0 + a), TG_SCATTERING_TEXTURE_MU_S_SIZE);
    
    tg_length u_nu = (nu + 1.0) / 2.0;
    return vec4(u_nu, u_mu_s, u_mu, u_r);
}

void tg_get_r_mu_mu_s_nu_from_scattering_texture_uvwz(
	TG_IN(tg_atmosphere_parameters)    atmosphere,
    TG_IN(vec4)                        uvwz,
	TG_OUT(tg_length)                  r,
	TG_OUT(tg_length)                  mu,
	TG_OUT(tg_length)                  mu_s,
    TG_OUT(tg_length)                  nu,
	TG_OUT(bool)                       ray_r_mu_intersects_ground
)
{
    TG_ASSERT(uvwz.x >= 0.0 && uvwz.x <= 1.0);
    TG_ASSERT(uvwz.y >= 0.0 && uvwz.y <= 1.0);
    TG_ASSERT(uvwz.z >= 0.0 && uvwz.z <= 1.0);
    TG_ASSERT(uvwz.w >= 0.0 && uvwz.w <= 1.0);
    
    tg_length H = sqrt(atmosphere.top_radius * atmosphere.top_radius - atmosphere.bottom_radius * atmosphere.bottom_radius);
    tg_length rho = H * tg_get_unit_range_from_texture_coord(uvwz.w, TG_SCATTERING_TEXTURE_R_SIZE);
    r = sqrt(rho * rho + atmosphere.bottom_radius * atmosphere.bottom_radius);
    
    if (uvwz.z < 0.5)
	{
        tg_length d_min = r - atmosphere.bottom_radius;
        tg_length d_max = rho;
        tg_length d = d_min + (d_max - d_min) * tg_get_unit_range_from_texture_coord(1.0 - 2.0 * uvwz.z, TG_SCATTERING_TEXTURE_MU_SIZE / 2);
        mu = d == 0.0 * m ? tg_length(-1.0) : tg_clamp_cosine(-(rho * rho + d * d) / (2.0 * r * d));
        ray_r_mu_intersects_ground = true;
    }
	else
	{
        tg_length d_min = atmosphere.top_radius - r;
        tg_length d_max = rho + H;
        tg_length d = d_min + (d_max - d_min) * tg_get_unit_range_from_texture_coord(2.0 * uvwz.z - 1.0, TG_SCATTERING_TEXTURE_MU_SIZE / 2);
        mu = d == 0.0 * m ? tg_length(1.0) : tg_clamp_cosine((H * H - rho * rho - d * d) / (2.0 * r * d));
        ray_r_mu_intersects_ground = false;
    }
    
    tg_length x_mu_s = tg_get_unit_range_from_texture_coord(uvwz.y, TG_SCATTERING_TEXTURE_MU_S_SIZE);
    tg_length d_min = atmosphere.top_radius - atmosphere.bottom_radius;
    tg_length d_max = H;
    tg_length A = -2.0 * atmosphere.mu_s_min * atmosphere.bottom_radius / (d_max - d_min);
    tg_length a = (A - x_mu_s * A) / (1.0 + x_mu_s * A);
    tg_length d = d_min + min(a, A) * (d_max - d_min);
    mu_s = d == 0.0 * m ? tg_length(1.0) : tg_clamp_cosine((H * H - d * d) / (2.0 * atmosphere.bottom_radius * d));
    
    nu = tg_clamp_cosine(uvwz.x * 2.0 - 1.0);
}

void tg_get_r_mu_mu_s_nu_from_scattering_texture_frag_coord(
    TG_IN(tg_atmosphere_parameters)    atmosphere,
	TG_IN(vec3)                        frag_coord,
    TG_OUT(tg_length)                  r,
	TG_OUT(tg_length)                  mu,
	TG_OUT(tg_length)                  mu_s,
	TG_OUT(tg_length)                  nu,
    TG_OUT(bool)                       ray_r_mu_intersects_ground
)
{
    const vec4 SCATTERING_TEXTURE_SIZE = vec4(
        TG_SCATTERING_TEXTURE_NU_SIZE - 1,
        TG_SCATTERING_TEXTURE_MU_S_SIZE,
        TG_SCATTERING_TEXTURE_MU_SIZE,
        TG_SCATTERING_TEXTURE_R_SIZE
	);
    tg_length frag_coord_nu = floor(frag_coord.x / tg_length(TG_SCATTERING_TEXTURE_MU_S_SIZE));
    tg_length frag_coord_mu_s = mod(frag_coord.x, tg_length(TG_SCATTERING_TEXTURE_MU_S_SIZE));
    vec4 uvwz = vec4(frag_coord_nu, frag_coord_mu_s, frag_coord.y, frag_coord.z) / SCATTERING_TEXTURE_SIZE;
    tg_get_r_mu_mu_s_nu_from_scattering_texture_uvwz(atmosphere, uvwz, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
    nu = clamp(nu, mu * mu_s - sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)), mu * mu_s + sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)));
}

void tg_compute_single_scattering_texture(
	TG_IN(tg_atmosphere_parameters)    atmosphere,
    TG_IN(tg_transmittance_texture)    transmittance_texture,
	TG_IN(vec3)                        frag_coord,
    TG_OUT(tg_irradiance_spectrum)     rayleigh,
	TG_OUT(tg_irradiance_spectrum)     mie
)
{
   tg_length r;
   tg_length mu;
   tg_length mu_s;
   tg_length nu;
   bool ray_r_mu_intersects_ground;
   tg_get_r_mu_mu_s_nu_from_scattering_texture_frag_coord(atmosphere, frag_coord, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
   tg_compute_single_scattering(atmosphere, transmittance_texture, r, mu, mu_s, nu, ray_r_mu_intersects_ground, rayleigh, mie);
}

TG_TEMPLATE(tg_abstract_spectrum)
tg_abstract_spectrum tg_get_scattering(
    TG_IN(tg_atmosphere_parameters)                                                  atmosphere,
    TG_IN(tg_abstract_scattering_texture TEMPLATE_ARGUMENT(tg_abstract_spectrum))    scattering_texture,
    tg_length                                                                        r,
	tg_length                                                                        mu,
	tg_length                                                                        mu_s,
	tg_length                                                                        nu,
    bool                                                                             ray_r_mu_intersects_ground
)
{
    vec4 uvwz = tg_get_scattering_texture_uvwz_from_r_mu_mu_s_nu(atmosphere, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
    tg_length tex_coord_x = uvwz.x * tg_length(TG_SCATTERING_TEXTURE_NU_SIZE - 1);
    tg_length tex_x = floor(tex_coord_x);
    tg_length lerp = tex_coord_x - tex_x;
    vec3 uvw0 = vec3((tex_x + uvwz.y) / tg_length(TG_SCATTERING_TEXTURE_NU_SIZE), uvwz.z, uvwz.w);
    vec3 uvw1 = vec3((tex_x + 1.0 + uvwz.y) / tg_length(TG_SCATTERING_TEXTURE_NU_SIZE), uvwz.z, uvwz.w);
    return tg_abstract_spectrum(texture(scattering_texture, uvw0) * (1.0 - lerp) + texture(scattering_texture, uvw1) * lerp);
}

tg_radiance_spectrum tg_get_scattering(
    TG_IN(tg_atmosphere_parameters)    atmosphere,
    TG_IN(ReducedScatteringTexture)    single_rayleigh_scattering_texture,
    TG_IN(ReducedScatteringTexture)    single_mie_scattering_texture,
    TG_IN(ScatteringTexture)           multiple_scattering_texture,
    tg_length                          r,
	tg_length                          mu,
	tg_length                          mu_s,
	tg_length                          nu,
    bool                               ray_r_mu_intersects_ground,
    int                                scattering_order
)
{
    if (scattering_order == 1)
	{
      tg_irradiance_spectrum rayleigh = tg_get_scattering(atmosphere, single_rayleigh_scattering_texture, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
      tg_irradiance_spectrum mie = tg_get_scattering(atmosphere, single_mie_scattering_texture, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
      return rayleigh * tg_rayleigh_phase_function(nu) + mie * tg_mie_phase_function(atmosphere.mie_phase_function_g, nu);
    }
	else
	{
      return tg_get_scattering(atmosphere, multiple_scattering_texture, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
    }
}

tg_irradiance_spectrum tg_get_irradiance(TG_IN(tg_atmosphere_parameters) atmosphere, TG_IN(tg_irradiance_texture) irradiance_texture, tg_length r, tg_length mu_s);

tg_radiance_density_spectrum tg_compute_scattering_density(
    TG_IN(tg_atmosphere_parameters)    atmosphere,
    TG_IN(tg_transmittance_texture)    transmittance_texture,
    TG_IN(ReducedScatteringTexture)    single_rayleigh_scattering_texture,
    TG_IN(ReducedScatteringTexture)    single_mie_scattering_texture,
    TG_IN(ScatteringTexture)           multiple_scattering_texture,
    TG_IN(tg_irradiance_texture)       irradiance_texture,
    tg_length                          r,
	tg_length                          mu,
	tg_length                          mu_s,
	tg_length                          nu,
	int                                scattering_order
)
{
    TG_ASSERT(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
    TG_ASSERT(mu >= -1.0 && mu <= 1.0);
    TG_ASSERT(mu_s >= -1.0 && mu_s <= 1.0);
    TG_ASSERT(nu >= -1.0 && nu <= 1.0);
    TG_ASSERT(scattering_order >= 2);
    
    vec3 zenith_direction = vec3(0.0, 0.0, 1.0);
    vec3 omega = vec3(sqrt(1.0 - mu * mu), 0.0, mu);
    tg_length sun_dir_x = omega.x == 0.0 ? 0.0 : (nu - mu * mu_s) / omega.x;
    tg_length sun_dir_y = sqrt(max(1.0 - sun_dir_x * sun_dir_x - mu_s * mu_s, 0.0));
    vec3 omega_s = vec3(sun_dir_x, sun_dir_y, mu_s);
    
    const int TG_SAMPLE_COUNT = 16;
    const tg_angle dphi = pi / tg_length(TG_SAMPLE_COUNT);
    const tg_angle dtheta = pi / tg_length(TG_SAMPLE_COUNT);
    tg_radiance_density_spectrum rayleigh_mie = tg_radiance_density_spectrum(0.0 * watt_per_cubic_meter_per_sr_per_nm);
    
    for (int l = 0; l < TG_SAMPLE_COUNT; ++l)
	{
        tg_angle theta = (tg_length(l) + 0.5) * dtheta;
        tg_length cos_theta = cos(theta);
        tg_length sin_theta = sin(theta);
        bool ray_r_theta_intersects_ground = tg_ray_intersects_ground(atmosphere, r, cos_theta);
	    
        tg_length distance_to_ground = 0.0 * m;
        tg_dimensionless_spectrum transmittance_to_ground = tg_dimensionless_spectrum(0.0);
        tg_dimensionless_spectrum ground_albedo = tg_dimensionless_spectrum(0.0);
        if (ray_r_theta_intersects_ground)
		{
            distance_to_ground = tg_distance_to_bottom_atmosphere_boundary(atmosphere, r, cos_theta);
            transmittance_to_ground = tg_get_transmittance(atmosphere, transmittance_texture, r, cos_theta, distance_to_ground, true);
            ground_albedo = atmosphere.ground_albedo;
        }
	    
        for (int m = 0; m < 2 * TG_SAMPLE_COUNT; ++m)
		{
            tg_angle phi = (tg_length(m) + 0.5) * dphi;
            vec3 omega_i = vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
            SolidAngle domega_i = (dtheta / rad) * (dphi / rad) * sin(theta) * sr;
		    
            tg_length nu1 = dot(omega_s, omega_i);
            tg_radiance_spectrum incident_radiance = tg_get_scattering(atmosphere,
                single_rayleigh_scattering_texture, single_mie_scattering_texture,
                multiple_scattering_texture, r, omega_i.z, mu_s, nu1,
                ray_r_theta_intersects_ground, scattering_order - 1);
		    
            vec3 ground_normal = normalize(zenith_direction * r + omega_i * distance_to_ground);
            tg_irradiance_spectrum ground_irradiance = tg_get_irradiance(atmosphere, irradiance_texture, atmosphere.bottom_radius, dot(ground_normal, omega_s));
            incident_radiance += transmittance_to_ground * ground_albedo * (1.0 / (TG_PI * sr)) * ground_irradiance;
		    
            tg_length nu2 = dot(omega, omega_i);
            tg_length rayleigh_density = tg_get_profile_density(atmosphere.rayleigh_density, r - atmosphere.bottom_radius);
            tg_length mie_density = tg_get_profile_density(atmosphere.mie_density, r - atmosphere.bottom_radius);
            rayleigh_mie += incident_radiance * (
                atmosphere.rayleigh_scattering * rayleigh_density *
                    tg_rayleigh_phase_function(nu2) +
                atmosphere.mie_scattering * mie_density *
                    tg_mie_phase_function(atmosphere.mie_phase_function_g, nu2)) *
                domega_i;
        }
    }
    return rayleigh_mie;
}

tg_radiance_spectrum tg_compute_multiple_scattering(
    TG_IN(tg_atmosphere_parameters)    atmosphere,
    TG_IN(tg_transmittance_texture)    transmittance_texture,
    TG_IN(ScatteringDensityTexture)    scattering_density_texture,
    tg_length                          r,
	tg_length                          mu,
	tg_length                          mu_s,
	tg_length                          nu,
    bool                               ray_r_mu_intersects_ground
)
{
    TG_ASSERT(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
    TG_ASSERT(mu >= -1.0 && mu <= 1.0);
    TG_ASSERT(mu_s >= -1.0 && mu_s <= 1.0);
    TG_ASSERT(nu >= -1.0 && nu <= 1.0);
    
    const int TG_SAMPLE_COUNT = 50;
    tg_length dx = tg_distance_to_nearest_atmosphere_boundary(atmosphere, r, mu, ray_r_mu_intersects_ground) / tg_length(TG_SAMPLE_COUNT);
    tg_radiance_spectrum rayleigh_mie_sum = tg_radiance_spectrum(0.0 * watt_per_square_meter_per_sr_per_nm);
    for (int i = 0; i <= TG_SAMPLE_COUNT; ++i)
	{
        tg_length d_i = tg_length(i) * dx;
	    
        tg_length r_i = tg_clamp_radius(atmosphere, sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r));
        tg_length mu_i = tg_clamp_cosine((r * mu + d_i) / r_i);
        tg_length mu_s_i = tg_clamp_cosine((r * mu_s + d_i * nu) / r_i);
	    
        tg_radiance_spectrum rayleigh_mie_i =
            tg_get_scattering(
                atmosphere, scattering_density_texture, r_i, mu_i, mu_s_i, nu,
                ray_r_mu_intersects_ground) *
            tg_get_transmittance(
                atmosphere, transmittance_texture, r, mu, d_i,
                ray_r_mu_intersects_ground) *
            dx;
        tg_length weight_i = (i == 0 || i == TG_SAMPLE_COUNT) ? 0.5 : 1.0;
        rayleigh_mie_sum += rayleigh_mie_i * weight_i;
    }
    return rayleigh_mie_sum;
}

tg_radiance_density_spectrum tg_compute_scattering_density_texture(
    TG_IN(tg_atmosphere_parameters)    atmosphere,
    TG_IN(tg_transmittance_texture)    transmittance_texture,
    TG_IN(ReducedScatteringTexture)    single_rayleigh_scattering_texture,
    TG_IN(ReducedScatteringTexture)    single_mie_scattering_texture,
    TG_IN(ScatteringTexture)           multiple_scattering_texture,
    TG_IN(tg_irradiance_texture)       irradiance_texture,
    TG_IN(vec3)                        frag_coord,
	int                                scattering_order
)
{
    tg_length r;
    tg_length mu;
    tg_length mu_s;
    tg_length nu;
    bool ray_r_mu_intersects_ground;
    tg_get_r_mu_mu_s_nu_from_scattering_texture_frag_coord(atmosphere, frag_coord, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
    return tg_compute_scattering_density(atmosphere, transmittance_texture,
        single_rayleigh_scattering_texture, single_mie_scattering_texture,
        multiple_scattering_texture, irradiance_texture, r, mu, mu_s, nu,
        scattering_order
	);
}

tg_radiance_spectrum ComputeMultipleScatteringTexture(
    TG_IN(tg_atmosphere_parameters)    atmosphere,
    TG_IN(tg_transmittance_texture)    transmittance_texture,
    TG_IN(ScatteringDensityTexture)    scattering_density_texture,
    TG_IN(vec3)                        frag_coord,
	TG_OUT(tg_length)                  nu
)
{
    tg_length r;
    tg_length mu;
    tg_length mu_s;
    bool ray_r_mu_intersects_ground;
    tg_get_r_mu_mu_s_nu_from_scattering_texture_frag_coord(atmosphere, frag_coord, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
    return tg_compute_multiple_scattering(atmosphere, transmittance_texture, scattering_density_texture, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
}

tg_irradiance_spectrum tg_compute_direct_irradiance(
    TG_IN(tg_atmosphere_parameters)    atmosphere,
    TG_IN(tg_transmittance_texture)    transmittance_texture,
    tg_length                          r,
	tg_length                          mu_s
)
{
  TG_ASSERT(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
  TG_ASSERT(mu_s >= -1.0 && mu_s <= 1.0);

  tg_length alpha_s = atmosphere.sun_angular_radius / rad;
  tg_length average_cosine_factor = mu_s < -alpha_s ? 0.0 : (mu_s > alpha_s ? mu_s : (mu_s + alpha_s) * (mu_s + alpha_s) / (4.0 * alpha_s));

  return atmosphere.solar_irradiance * tg_get_transmittance_to_top_atmosphere_boundary(atmosphere, transmittance_texture, r, mu_s) * average_cosine_factor;

}

tg_irradiance_spectrum tg_compute_indirect_irradiance(
    TG_IN(tg_atmosphere_parameters)    atmosphere,
    TG_IN(ReducedScatteringTexture)    single_rayleigh_scattering_texture,
    TG_IN(ReducedScatteringTexture)    single_mie_scattering_texture,
    TG_IN(ScatteringTexture)           multiple_scattering_texture,
    tg_length                          r,
	tg_length                          mu_s,
	int                                scattering_order
)
{
    TG_ASSERT(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
    TG_ASSERT(mu_s >= -1.0 && mu_s <= 1.0);
    TG_ASSERT(scattering_order >= 1);
  
    const int TG_SAMPLE_COUNT = 32;
    const tg_angle dphi = pi / tg_length(TG_SAMPLE_COUNT);
    const tg_angle dtheta = pi / tg_length(TG_SAMPLE_COUNT);
  
    tg_irradiance_spectrum result = tg_irradiance_spectrum(0.0 * watt_per_square_meter_per_nm);
    vec3 omega_s = vec3(sqrt(1.0 - mu_s * mu_s), 0.0, mu_s);
    for (int j = 0; j < TG_SAMPLE_COUNT / 2; ++j)
	{
        tg_angle theta = (tg_length(j) + 0.5) * dtheta;
        for (int i = 0; i < 2 * TG_SAMPLE_COUNT; ++i) {
            tg_angle phi = (tg_length(i) + 0.5) * dphi;
            vec3 omega = vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
            SolidAngle domega = (dtheta / rad) * (dphi / rad) * sin(theta) * sr;
		    
            tg_length nu = dot(omega, omega_s);
            result += tg_get_scattering(atmosphere, single_rayleigh_scattering_texture,
                single_mie_scattering_texture, multiple_scattering_texture,
                r, omega.z, mu_s, nu, false, scattering_order)
				    * omega.z * domega;
        }
    }
    return result;
}

vec2 tg_get_irradiance_texture_uv_from_r_mu_s(TG_IN(tg_atmosphere_parameters) atmosphere, tg_length r, tg_length mu_s)
{
    TG_ASSERT(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
    TG_ASSERT(mu_s >= -1.0 && mu_s <= 1.0);
    
    tg_length x_r = (r - atmosphere.bottom_radius) / (atmosphere.top_radius - atmosphere.bottom_radius);
    tg_length x_mu_s = mu_s * 0.5 + 0.5;
    return vec2(tg_get_texture_coord_from_unit_range(x_mu_s, TG_IRRADIANCE_TEXTURE_WIDTH), tg_get_texture_coord_from_unit_range(x_r, TG_IRRADIANCE_TEXTURE_HEIGHT));
}

void tg_get_r_mu_s_from_irradiance_texture_uv(TG_IN(tg_atmosphere_parameters) atmosphere, TG_IN(vec2) uv, TG_OUT(tg_length) r, TG_OUT(tg_length) mu_s)
{
    TG_ASSERT(uv.x >= 0.0 && uv.x <= 1.0);
    TG_ASSERT(uv.y >= 0.0 && uv.y <= 1.0);
    
    tg_length x_mu_s = tg_get_unit_range_from_texture_coord(uv.x, TG_IRRADIANCE_TEXTURE_WIDTH);
    tg_length x_r = tg_get_unit_range_from_texture_coord(uv.y, TG_IRRADIANCE_TEXTURE_HEIGHT);
    r = atmosphere.bottom_radius + x_r * (atmosphere.top_radius - atmosphere.bottom_radius);
    mu_s = tg_clamp_cosine(2.0 * x_mu_s - 1.0);
}

const vec2 TG_IRRADIANCE_TEXTURE_SIZE = vec2(TG_IRRADIANCE_TEXTURE_WIDTH, TG_IRRADIANCE_TEXTURE_HEIGHT);

tg_irradiance_spectrum tg_compute_direct_irradiance_texture(TG_IN(tg_atmosphere_parameters) atmosphere, TG_IN(tg_transmittance_texture) transmittance_texture, TG_IN(vec2) frag_coord)
{
  tg_length r;
  tg_length mu_s;
  tg_get_r_mu_s_from_irradiance_texture_uv(atmosphere, frag_coord / TG_IRRADIANCE_TEXTURE_SIZE, r, mu_s);
  return tg_compute_direct_irradiance(atmosphere, transmittance_texture, r, mu_s);
}

tg_irradiance_spectrum tg_compute_indirect_irradiance_texture(
    TG_IN(tg_atmosphere_parameters)    atmosphere,
    TG_IN(ReducedScatteringTexture)    single_rayleigh_scattering_texture,
    TG_IN(ReducedScatteringTexture)    single_mie_scattering_texture,
    TG_IN(ScatteringTexture)           multiple_scattering_texture,
    TG_IN(vec2)                        frag_coord,
	int                                scattering_order
)
{
    tg_length r;
    tg_length mu_s;
    tg_get_r_mu_s_from_irradiance_texture_uv(atmosphere, frag_coord / TG_IRRADIANCE_TEXTURE_SIZE, r, mu_s);
    return tg_compute_indirect_irradiance(
		atmosphere, single_rayleigh_scattering_texture, single_mie_scattering_texture, multiple_scattering_texture, r, mu_s, scattering_order
	);
}

tg_irradiance_spectrum tg_get_irradiance(TG_IN(tg_atmosphere_parameters) atmosphere, TG_IN(tg_irradiance_texture) irradiance_texture, tg_length r, tg_length mu_s)
{
    vec2 uv = tg_get_irradiance_texture_uv_from_r_mu_s(atmosphere, r, mu_s);
    return tg_irradiance_spectrum(texture(irradiance_texture, uv));
}

#ifdef TG_COMBINED_SCATTERING_TEXTURES

vec3 tg_get_extrapolated_single_mie_scattering(TG_IN(tg_atmosphere_parameters) atmosphere, TG_IN(vec4) scattering)
{
    if (scattering.r <= 0.0)
    {
      return vec3(0.0);
    }
    return scattering.rgb * scattering.a / scattering.r *
	    (atmosphere.rayleigh_scattering.r / atmosphere.mie_scattering.r) *
	    (atmosphere.mie_scattering / atmosphere.rayleigh_scattering);
}

#endif

tg_irradiance_spectrum tg_get_combined_scattering(
    TG_IN(tg_atmosphere_parameters)    atmosphere,
    TG_IN(ReducedScatteringTexture)    scattering_texture,
    TG_IN(ReducedScatteringTexture)    single_mie_scattering_texture,
    tg_length                          r,
	tg_length                          mu,
	tg_length                          mu_s,
	tg_length                          nu,
    bool                               ray_r_mu_intersects_ground,
    TG_OUT(tg_irradiance_spectrum)     single_mie_scattering
)
{
    vec4 uvwz = tg_get_scattering_texture_uvwz_from_r_mu_mu_s_nu(atmosphere, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
    tg_length tex_coord_x = uvwz.x * tg_length(TG_SCATTERING_TEXTURE_NU_SIZE - 1);
    tg_length tex_x = floor(tex_coord_x);
    tg_length lerp = tex_coord_x - tex_x;
    vec3 uvw0 = vec3((tex_x + uvwz.y) / tg_length(TG_SCATTERING_TEXTURE_NU_SIZE), uvwz.z, uvwz.w);
    vec3 uvw1 = vec3((tex_x + 1.0 + uvwz.y) / tg_length(TG_SCATTERING_TEXTURE_NU_SIZE), uvwz.z, uvwz.w);
#ifdef TG_COMBINED_SCATTERING_TEXTURES
    vec4 combined_scattering =
	    texture(scattering_texture, uvw0) * (1.0 - lerp) +
		texture(scattering_texture, uvw1) * lerp;
    tg_irradiance_spectrum scattering = tg_irradiance_spectrum(combined_scattering);
    single_mie_scattering = tg_get_extrapolated_single_mie_scattering(atmosphere, combined_scattering);
#else
    tg_irradiance_spectrum scattering = tg_irradiance_spectrum(
        texture(scattering_texture, uvw0) * (1.0 - lerp) +
        texture(scattering_texture, uvw1) * lerp);
    single_mie_scattering = tg_irradiance_spectrum(
        texture(single_mie_scattering_texture, uvw0) * (1.0 - lerp) +
        texture(single_mie_scattering_texture, uvw1) * lerp);
#endif
    return scattering;
}

tg_radiance_spectrum tg_get_sky_radiance(
    TG_IN(tg_atmosphere_parameters)      atmosphere,
    TG_IN(tg_transmittance_texture)      transmittance_texture,
    TG_IN(ReducedScatteringTexture)      scattering_texture,
    TG_IN(ReducedScatteringTexture)      single_mie_scattering_texture,
    tg_position                          camera,
	TG_IN(tg_direction)                  view_ray,
	tg_length                            shadow_length,
    TG_IN(tg_direction)                  sun_direction,
	TG_OUT(tg_dimensionless_spectrum)    transmittance
)
{
    tg_length r = length(camera);
    tg_length rmu = dot(camera, view_ray);
    tg_length distance_to_top_atmosphere_boundary = -rmu - sqrt(rmu * rmu - r * r + atmosphere.top_radius * atmosphere.top_radius);
    if (distance_to_top_atmosphere_boundary > 0.0 * m)
    {
        camera = camera + view_ray * distance_to_top_atmosphere_boundary;
        r = atmosphere.top_radius;
        rmu += distance_to_top_atmosphere_boundary;
    }
	else if (r > atmosphere.top_radius)
	{
        transmittance = tg_dimensionless_spectrum(1.0);
        return tg_radiance_spectrum(0.0 * watt_per_square_meter_per_sr_per_nm);
    }
    tg_length mu = rmu / r;
    tg_length mu_s = dot(camera, sun_direction) / r;
    tg_length nu = dot(view_ray, sun_direction);
    bool ray_r_mu_intersects_ground = tg_ray_intersects_ground(atmosphere, r, mu);
    
    transmittance = ray_r_mu_intersects_ground ? tg_dimensionless_spectrum(0.0) : tg_get_transmittance_to_top_atmosphere_boundary(atmosphere, transmittance_texture, r, mu);
    tg_irradiance_spectrum single_mie_scattering;
    tg_irradiance_spectrum scattering;
    if (shadow_length == 0.0 * m)
	{
        scattering = tg_get_combined_scattering(
            atmosphere, scattering_texture, single_mie_scattering_texture,
            r, mu, mu_s, nu, ray_r_mu_intersects_ground,
            single_mie_scattering);
    }
	else
	{
        tg_length d = shadow_length;
        tg_length r_p = tg_clamp_radius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
        tg_length mu_p = (r * mu + d) / r_p;
        tg_length mu_s_p = (r * mu_s + d * nu) / r_p;
	    
        scattering = tg_get_combined_scattering(
            atmosphere, scattering_texture, single_mie_scattering_texture,
            r_p, mu_p, mu_s_p, nu, ray_r_mu_intersects_ground,
            single_mie_scattering);
        tg_dimensionless_spectrum shadow_transmittance =
            tg_get_transmittance(atmosphere, transmittance_texture, r, mu, shadow_length, ray_r_mu_intersects_ground);
        scattering = scattering * shadow_transmittance;
        single_mie_scattering = single_mie_scattering * shadow_transmittance;
    }
    return scattering * tg_rayleigh_phase_function(nu) + single_mie_scattering * tg_mie_phase_function(atmosphere.mie_phase_function_g, nu);
}

tg_radiance_spectrum tg_get_sky_radiance(
    TG_IN(tg_atmosphere_parameters)      atmosphere,
    TG_IN(tg_transmittance_texture)      transmittance_texture,
    TG_IN(ReducedScatteringTexture)      scattering_texture,
    TG_IN(ReducedScatteringTexture)      single_mie_scattering_texture,
    tg_position                          camera,
	TG_IN(tg_position)                   point,
	tg_length                            shadow_length,
    TG_IN(tg_direction)                  sun_direction,
	TG_OUT(tg_dimensionless_spectrum)    transmittance
)
{
    tg_direction view_ray = normalize(point - camera);
    tg_length r = length(camera);
    tg_length rmu = dot(camera, view_ray);
    tg_length distance_to_top_atmosphere_boundary = -rmu - sqrt(rmu * rmu - r * r + atmosphere.top_radius * atmosphere.top_radius);
    if (distance_to_top_atmosphere_boundary > 0.0 * m)
    {
        camera = camera + view_ray * distance_to_top_atmosphere_boundary;
        r = atmosphere.top_radius;
        rmu += distance_to_top_atmosphere_boundary;
    }
    
    tg_length mu = rmu / r;
    tg_length mu_s = dot(camera, sun_direction) / r;
    tg_length nu = dot(view_ray, sun_direction);
    tg_length d = length(point - camera);
    bool ray_r_mu_intersects_ground = tg_ray_intersects_ground(atmosphere, r, mu);
    
    transmittance = tg_get_transmittance(atmosphere, transmittance_texture, r, mu, d, ray_r_mu_intersects_ground);
    
    tg_irradiance_spectrum single_mie_scattering;
    tg_irradiance_spectrum scattering = tg_get_combined_scattering(
        atmosphere, scattering_texture, single_mie_scattering_texture,
        r, mu, mu_s, nu, ray_r_mu_intersects_ground,
        single_mie_scattering);
    
    d = max(d - shadow_length, 0.0 * m);
    tg_length r_p = tg_clamp_radius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
    tg_length mu_p = (r * mu + d) / r_p;
    tg_length mu_s_p = (r * mu_s + d * nu) / r_p;
    
    tg_irradiance_spectrum single_mie_scattering_p;
    tg_irradiance_spectrum scattering_p = tg_get_combined_scattering(
        atmosphere, scattering_texture, single_mie_scattering_texture,
        r_p, mu_p, mu_s_p, nu, ray_r_mu_intersects_ground,
        single_mie_scattering_p);
    
    tg_dimensionless_spectrum shadow_transmittance = transmittance;
    if (shadow_length > 0.0 * m)
	{
        shadow_transmittance = tg_get_transmittance(atmosphere, transmittance_texture, r, mu, d, ray_r_mu_intersects_ground);
    }
    scattering = scattering - shadow_transmittance * scattering_p;
    single_mie_scattering = single_mie_scattering - shadow_transmittance * single_mie_scattering_p;
#ifdef TG_COMBINED_SCATTERING_TEXTURES
    single_mie_scattering = tg_get_extrapolated_single_mie_scattering(atmosphere, vec4(scattering, single_mie_scattering.r));
#endif

    single_mie_scattering = single_mie_scattering * smoothstep(tg_length(0.0), tg_length(0.01), mu_s);
    
    return scattering * tg_rayleigh_phase_function(nu) + single_mie_scattering * tg_mie_phase_function(atmosphere.mie_phase_function_g, nu);
}

tg_irradiance_spectrum tg_get_sun_and_sky_irradiance(
    TG_IN(tg_atmosphere_parameters)    atmosphere,
    TG_IN(tg_transmittance_texture)    transmittance_texture,
    TG_IN(tg_irradiance_texture)       irradiance_texture,
    TG_IN(tg_position)                 point,
	TG_IN(tg_direction)                normal,
	TG_IN(tg_direction)                sun_direction,
    TG_OUT(tg_irradiance_spectrum)     sky_irradiance
)
{
    tg_length r = length(point);
    tg_length mu_s = dot(point, sun_direction) / r;
    
    sky_irradiance = tg_get_irradiance(atmosphere, irradiance_texture, r, mu_s) * (1.0 + dot(normal, point) / r) * 0.5;
    
    return atmosphere.solar_irradiance *
        tg_get_transmittance_to_sun(
            atmosphere, transmittance_texture, r, mu_s) *
        max(dot(normal, sun_direction), 0.0);
}
