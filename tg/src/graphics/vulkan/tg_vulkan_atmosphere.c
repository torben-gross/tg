// source: http://www-evasion.imag.fr/Membres/Eric.Bruneton/

#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "util/tg_string.h"



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
	u32    num_precomputed_wavelengths;
	b32    half_precision;
	b32    rgb_format_supported;
} tg_model;



static const char TG_VERTEX_SHADER[] =
	"#version 330"
	""
	"layout(location = 0) in vec4 vertex;"
	""
	"uniform mat4 model_from_view;"
	"uniform mat4 view_from_clip;"
	""
	"out vec3 view_ray;"
	""
	"void main()"
	"{"
	"    view_ray = (model_from_view * vec4((view_from_clip * vertex).xyz, 0.0)).xyz;"
	"    gl_Position = vertex;"
	"}";

const char TG_COMPUTE_TRANSMITTANCE_SHADER[] =
	"layout(location = 0) out vec3 transmittance;"
	""
	"void main()"
	"{"
	"    transmittance = tg_compute_transmittance_to_top_atmosphere_boundary_texture(ATMOSPHERE, gl_FragCoord.xy);"
	"}";

const char TG_COMPUTE_DIRECT_IRRADIANCE_SHADER[] =
	"layout(location = 0) out vec3 delta_irradiance;"
	"layout(location = 1) out vec3 irradiance;"
	""
	"uniform sampler2D transmittance_texture;"
	""
	"void main()"
	"{"
	"    delta_irradiance = tg_compute_direction_irradiance_texture(ATMOSPHERE, transmittance_texture, gl_FragCoord.xy);"
	"    irradiance = vec3(0.0);"
	"}";

const char TG_COMPUTE_SINGLE_SCATTERING_SHADER[] =
	"uniform mat3 luminance_from_radiance;"
	"uniform sampler2D transmittance_texture;"
	""
	"layout(location = 0) out vec3 delta_rayleigh;"
	"layout(location = 1) out vec3 delta_mie;"
	"layout(location = 2) out vec4 scattering;"
	"layout(location = 3) out vec3 single_mie_scattering;"
	"uniform int layer;"
	""
	"void main()"
	"{"
	"    tg_compute_single_scattering_texture(ATMOSPHERE, transmittance_texture, vec3(gl_FragCoord.xy, layer + 0.5), delta_rayleigh, delta_mie);"
	"    scattering = vec4(luminance_from_radiance * delta_rayleigh.rgb, (luminance_from_radiance * delta_mie).r);"
	"    single_mie_scattering = luminance_from_radiance * delta_mie;"
	"}";

const char TG_COMPUTE_SCATTERING_DENSITY_SHADER[] =
	"uniform sampler2D transmittance_texture;"
	"uniform sampler3D single_rayleigh_scattering_texture;"
	"uniform sampler3D single_mie_scattering_texture;"
	"uniform sampler3D multiple_scattering_texture;"
	"uniform sampler2D irradiance_texture;"
	"uniform int scattering_order;"
	"uniform int layer;"
	""
	"layout(location = 0) out vec3 scattering_density;"
	""
	"void main()"
	"{"
	"    scattering_density = ComputeScatteringDensityTexture("
	"        ATMOSPHERE, transmittance_texture, single_rayleigh_scattering_texture,"
	"        single_mie_scattering_texture, multiple_scattering_texture,"
	"        irradiance_texture, vec3(gl_FragCoord.xy, layer + 0.5),"
	"        scattering_order"
	"    );"
	"}";

const char TG_COMPUTE_INDIRECT_IRRADIANCE_SHADER[] =
	"uniform mat3 luminance_from_radiance;"
	"uniform sampler3D single_rayleigh_scattering_texture;"
	"uniform sampler3D single_mie_scattering_texture;"
	"uniform sampler3D multiple_scattering_texture;"
	"uniform int scattering_order;"
	""
	"layout(location = 0) out vec3 delta_irradiance;"
	"layout(location = 1) out vec3 irradiance;"
	""
	"void main()"
	"{"
	"    delta_irradiance = ComputeIndirectIrradianceTexture("
	"        ATMOSPHERE, single_rayleigh_scattering_texture,"
	"        single_mie_scattering_texture, multiple_scattering_texture,"
	"        gl_FragCoord.xy, scattering_order"
	"    );"
	"    irradiance = luminance_from_radiance * delta_irradiance;"
	"}";

const char TG_COMPUTE_MULTIPLE_SCATTERING_SHADER[] =
	"uniform mat3 luminance_from_radiance;"
	"uniform sampler2D transmittance_texture;"
	"uniform sampler3D scattering_density_texture;"
	"uniform int layer;"
	""
	"layout(location = 0) out vec3 delta_multiple_scattering;"
	"layout(location = 1) out vec4 scattering;"
	""
	"void main()"
	"{"
	"    float nu;"
	"    delta_multiple_scattering = ComputeMultipleScatteringTexture("
	"        ATMOSPHERE, transmittance_texture, scattering_density_texture,"
	"        vec3(gl_FragCoord.xy, layer + 0.5), nu"
	"    );"
	"    scattering = vec4(luminance_from_radiance * delta_multiple_scattering.rgb / RayleighPhaseFunction(nu), 0.0);"
	"}";

const char TG_ATMOSPHRE_SHADER[] =
    "uniform sampler2D transmittance_texture;"
    "uniform sampler3D scattering_texture;"
    "uniform sampler3D single_mie_scattering_texture;"
    "uniform sampler2D irradiance_texture;"
	""
    "#ifdef RADIANCE_API_ENABLED"
	""
    "tg_radiance_spectrum tg_get_solar_radiance()"
	"{"
	"    return ATMOSPHERE.solar_irradiance / (PI * ATMOSPHERE.sun_angular_radius * ATMOSPHERE.sun_angular_radius);"
    "}"
	""
    "tg_radiance_spectrum tg_get_sky_radiance("
	"    tg_position camera, tg_direction view_ray, tg_length shadow_length,"
	"    tg_direction sun_direction, out tg_dimensionless_spectrum transmittance"
	")"
	"{"
	"    return GetSkyRadiance("
	"        ATMOSPHERE, transmittance_texture,"
	"        scattering_texture, single_mie_scattering_texture,"
	"        camera, view_ray, shadow_length, sun_direction, transmittance"
	"    );"
    "}"
	""
    "tg_radiance_spectrum tg_get_sky_radiance_to_point("
	"    tg_position camera, tg_position point, tg_length shadow_length,"
	"    tg_direction sun_direction, out tg_dimensionless_spectrum transmittance"
	")"
	"{"
	"    return tg_get_sky_radiance_to_point("
	"        ATMOSPHERE, transmittance_texture,"
	"        scattering_texture, single_mie_scattering_texture,"
	"        camera, point, shadow_length, sun_direction, transmittance"
	"    );"
    "}"
	""
    "tg_irradiance_spectrum tg_get_sun_and_sky_irradiance("
	"    tg_position p, tg_direction normal, tg_direction sun_direction,"
	"    out tg_irradiance_spectrum sky_irradiance"
	")"
	"{"
	"    return tg_get_sun_and_sky_irradiance(ATMOSPHERE, transmittance_texture, irradiance_texture, p, normal, sun_direction, sky_irradiance);"
    "}"
	""
    "#endif"
	""
    "tg_luminance3 tg_get_solar_luminance()"
	"{"
	"    return ATMOSPHERE.solar_irradiance / (PI * ATMOSPHERE.sun_angular_radius * ATMOSPHERE.sun_angular_radius) * TG_SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;"
    "}"
	""
    "tg_luminance3 tg_get_sky_luminance("
	"    tg_position camera, tg_direction view_ray, tg_length shadow_length,"
	"    tg_direction sun_direction, out tg_dimensionless_spectrum transmittance)"
	"{"
	"    return tg_get_sky_radiance("
	"        ATMOSPHERE, transmittance_texture, scattering_texture, single_mie_scattering_texture,"
	"        camera, view_ray, shadow_length, sun_direction, transmittance"
	"    ) * TG_SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;"
    "}"
	""
    "tg_luminance3 tg_get_sky_luminance_to_point("
	"    tg_position camera, tg_position point, tg_length shadow_length,"
	"    tg_direction sun_direction, out tg_dimensionless_spectrum transmittance)"
	"{"
	"    return GetSkyRadianceToPoint("
	"        ATMOSPHERE, transmittance_texture, scattering_texture, single_mie_scattering_texture,"
	"        camera, point, shadow_length, sun_direction, transmittance"
	"    ) * TG_SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;"
    "}"
	""
    "tg_illuminance3 tg_get_sun_and_sky_illuminance("
	"    tg_position p, tg_direction normal, tg_direction sun_direction,"
	"    out tg_irradianceSpectrum sky_irradiance)"
	"{"
	"    tg_irradiance_spectrum sun_irradiance = tg_get_sun_and_sky_irradiance("
	"        ATMOSPHERE, transmittance_texture, irradiance_texture, p, normal, sun_direction, sky_irradiance"
	"    );"
	"    sky_irradiance *= TG_SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;"
	"    return sun_irradiance * TG_SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;"
    "}";

static const f64 p_solar_irradiance[48] = {
	1.11776, 1.14259, 1.01249, 1.14716, 1.72765, 1.73054, 1.68870, 1.61253,
	1.91198, 2.03474, 2.02042, 2.02212, 1.93377, 1.95809, 1.91686, 1.82980,
	1.86850, 1.89310, 1.85149, 1.85040, 1.83410, 1.83450, 1.81470, 1.78158,
	1.75330, 1.69650, 1.68194, 1.64654, 1.60480, 1.52143, 1.55622, 1.51130,
	1.47400, 1.44820, 1.41018, 1.36775, 1.34188, 1.31429, 1.28303, 1.26758,
	1.23670, 1.20820, 1.18737, 1.14683, 1.12362, 1.10580, 1.07124, 1.04992
};

static const f64 p_ozone_cross_section[48] = {
	1.180e-27, 2.182e-28, 2.818e-28, 6.636e-28, 1.527e-27, 2.763e-27, 5.520e-27,
	8.451e-27, 1.582e-26, 2.316e-26, 3.669e-26, 4.924e-26, 7.752e-26, 9.016e-26,
	1.480e-25, 1.602e-25, 2.139e-25, 2.755e-25, 3.091e-25, 3.500e-25, 4.266e-25,
	4.672e-25, 4.398e-25, 4.701e-25, 5.019e-25, 4.305e-25, 3.740e-25, 3.215e-25,
	2.662e-25, 2.238e-25, 1.852e-25, 1.473e-25, 1.209e-25, 9.423e-26, 7.455e-26,
	6.566e-26, 5.105e-26, 4.15e0 - 26, 4.228e-26, 3.237e-26, 2.451e-26, 2.801e-26,
	2.534e-26, 1.624e-26, 1.465e-26, 2.078e-26, 1.383e-26, 7.105e-27
};

static const f64 p_cie_2_deg_color_matching_functions[380] = {
	360.0, 0.000129900000, 0.000003917000, 0.000606100000,
	365.0, 0.000232100000, 0.000006965000, 0.001086000000,
	370.0, 0.000414900000, 0.000012390000, 0.001946000000,
	375.0, 0.000741600000, 0.000022020000, 0.003486000000,
	380.0, 0.001368000000, 0.000039000000, 0.006450001000,
	385.0, 0.002236000000, 0.000064000000, 0.010549990000,
	390.0, 0.004243000000, 0.000120000000, 0.020050010000,
	395.0, 0.007650000000, 0.000217000000, 0.036210000000,
	400.0, 0.014310000000, 0.000396000000, 0.067850010000,
	405.0, 0.023190000000, 0.000640000000, 0.110200000000,
	410.0, 0.043510000000, 0.001210000000, 0.207400000000,
	415.0, 0.077630000000, 0.002180000000, 0.371300000000,
	420.0, 0.134380000000, 0.004000000000, 0.645600000000,
	425.0, 0.214770000000, 0.007300000000, 1.039050100000,
	430.0, 0.283900000000, 0.011600000000, 1.385600000000,
	435.0, 0.328500000000, 0.016840000000, 1.622960000000,
	440.0, 0.348280000000, 0.023000000000, 1.747060000000,
	445.0, 0.348060000000, 0.029800000000, 1.782600000000,
	450.0, 0.336200000000, 0.038000000000, 1.772110000000,
	455.0, 0.318700000000, 0.048000000000, 1.744100000000,
	460.0, 0.290800000000, 0.060000000000, 1.669200000000,
	465.0, 0.251100000000, 0.073900000000, 1.528100000000,
	470.0, 0.195360000000, 0.090980000000, 1.287640000000,
	475.0, 0.142100000000, 0.112600000000, 1.041900000000,
	480.0, 0.095640000000, 0.139020000000, 0.812950100000,
	485.0, 0.057950010000, 0.169300000000, 0.616200000000,
	490.0, 0.032010000000, 0.208020000000, 0.465180000000,
	495.0, 0.014700000000, 0.258600000000, 0.353300000000,
	500.0, 0.004900000000, 0.323000000000, 0.272000000000,
	505.0, 0.002400000000, 0.407300000000, 0.212300000000,
	510.0, 0.009300000000, 0.503000000000, 0.158200000000,
	515.0, 0.029100000000, 0.608200000000, 0.111700000000,
	520.0, 0.063270000000, 0.710000000000, 0.078249990000,
	525.0, 0.109600000000, 0.793200000000, 0.057250010000,
	530.0, 0.165500000000, 0.862000000000, 0.042160000000,
	535.0, 0.225749900000, 0.914850100000, 0.029840000000,
	540.0, 0.290400000000, 0.954000000000, 0.020300000000,
	545.0, 0.359700000000, 0.980300000000, 0.013400000000,
	550.0, 0.433449900000, 0.994950100000, 0.008749999000,
	555.0, 0.512050100000, 1.000000000000, 0.005749999000,
	560.0, 0.594500000000, 0.995000000000, 0.003900000000,
	565.0, 0.678400000000, 0.978600000000, 0.002749999000,
	570.0, 0.762100000000, 0.952000000000, 0.002100000000,
	575.0, 0.842500000000, 0.915400000000, 0.001800000000,
	580.0, 0.916300000000, 0.870000000000, 0.001650001000,
	585.0, 0.978600000000, 0.816300000000, 0.001400000000,
	590.0, 1.026300000000, 0.757000000000, 0.001100000000,
	595.0, 1.056700000000, 0.694900000000, 0.001000000000,
	600.0, 1.062200000000, 0.631000000000, 0.000800000000,
	605.0, 1.045600000000, 0.566800000000, 0.000600000000,
	610.0, 1.002600000000, 0.503000000000, 0.000340000000,
	615.0, 0.938400000000, 0.441200000000, 0.000240000000,
	620.0, 0.854449900000, 0.381000000000, 0.000190000000,
	625.0, 0.751400000000, 0.321000000000, 0.000100000000,
	630.0, 0.642400000000, 0.265000000000, 0.000049999990,
	635.0, 0.541900000000, 0.217000000000, 0.000030000000,
	640.0, 0.447900000000, 0.175000000000, 0.000020000000,
	645.0, 0.360800000000, 0.138200000000, 0.000010000000,
	650.0, 0.283500000000, 0.107000000000, 0.000000000000,
	655.0, 0.218700000000, 0.081600000000, 0.000000000000,
	660.0, 0.164900000000, 0.061000000000, 0.000000000000,
	665.0, 0.121200000000, 0.044580000000, 0.000000000000,
	670.0, 0.087400000000, 0.032000000000, 0.000000000000,
	675.0, 0.063600000000, 0.023200000000, 0.000000000000,
	680.0, 0.046770000000, 0.017000000000, 0.000000000000,
	685.0, 0.032900000000, 0.011920000000, 0.000000000000,
	690.0, 0.022700000000, 0.008210000000, 0.000000000000,
	695.0, 0.015840000000, 0.005723000000, 0.000000000000,
	700.0, 0.011359160000, 0.004102000000, 0.000000000000,
	705.0, 0.008110916000, 0.002929000000, 0.000000000000,
	710.0, 0.005790346000, 0.002091000000, 0.000000000000,
	715.0, 0.004109457000, 0.001484000000, 0.000000000000,
	720.0, 0.002899327000, 0.001047000000, 0.000000000000,
	725.0, 0.002049190000, 0.000740000000, 0.000000000000,
	730.0, 0.001439971000, 0.000520000000, 0.000000000000,
	735.0, 0.000999949300, 0.000361100000, 0.000000000000,
	740.0, 0.000690078600, 0.000249200000, 0.000000000000,
	745.0, 0.000476021300, 0.000171900000, 0.000000000000,
	750.0, 0.000332301100, 0.000120000000, 0.000000000000,
	755.0, 0.000234826100, 0.000084800000, 0.000000000000,
	760.0, 0.000166150500, 0.000060000000, 0.000000000000,
	765.0, 0.000117413000, 0.000042400000, 0.000000000000,
	770.0, 0.000083075270, 0.000030000000, 0.000000000000,
	775.0, 0.000058706520, 0.000021200000, 0.000000000000,
	780.0, 0.000041509940, 0.000014990000, 0.000000000000,
	785.0, 0.000029353260, 0.000010600000, 0.000000000000,
	790.0, 0.000020673830, 0.000007465700, 0.000000000000,
	795.0, 0.000014559770, 0.000005257800, 0.000000000000,
	800.0, 0.000010253980, 0.000003702900, 0.000000000000,
	805.0, 0.000007221456, 0.000002607800, 0.000000000000,
	810.0, 0.000005085868, 0.000001836600, 0.000000000000,
	815.0, 0.000003581652, 0.000001293400, 0.000000000000,
	820.0, 0.000002522525, 0.000000910930, 0.000000000000,
	825.0, 0.000001776509, 0.000000641530, 0.000000000000,
	830.0, 0.000001251141, 0.000000451810, 0.000000000000,
};

static const f64 p_xyz_to_srgb[9] = {
	+3.2406, -1.5372, -0.4986,
	-0.9689, +1.8758, +0.0415,
	+0.0557, -0.2040, +1.0570
};




f64 tg__interpolate(u32 count, const f64* p_wavelengths, const f64* p_wavelength_function, f64 wavelength)
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

void tg__glsl_header_factory(
	b32                                use_half_precision,
	b32                                combine_scattering_textures,

	const f64*                         p_wavelengths,           // 48
	const f64*                         p_solar_irrad,           // 48
	const f64*                         p_rayleigh_scattering,   // 48
	const f64*                         p_mie_scattering,        // 48
	const f64*                         p_mie_extinction,        // 48
	const f64*                         p_absorption_extinction, // 48
	const f64*                         p_ground_albedo,         // 48

	const tg_density_profile_layer*    p_rayleigh_density,      // 1
	const tg_density_profile_layer*    p_mie_density,           // 1
	const tg_density_profile_layer*    p_absorption_density,    // 2

	f64 sky_k_r,
	f64 sky_k_g,
	f64 sky_k_b,
	f64 sun_k_r,
	f64 sun_k_g,
	f64 sun_k_b,

	u32                                size,
	char*                              p_buffer
)
{
#pragma warning(push)
#pragma warning(disable:6294)

#define PUSH(p_string)     ((void)(i += tg_string_copy_no_nul(size - i, &p_buffer[i], p_string)))
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
#define DENSITY_PROFILE(count, p_density_profile_layers)               \
	{                                                                  \
		TG_ASSERT(count);                                              \
		PUSH("    tg_density_profile(\n");                             \
		PUSH("        tg_density_profile_layer[2](\n");                \
		tg_density_profile_layer empty_density_profile_layer =  { 0 }; \
		for (u32 idx = 0; idx < LAYER_COUNT - count; idx++)            \
		{                                                              \
			DENSITY_LAYER(empty_density_profile_layer); PUSH(",\n");   \
		}                                                              \
		for (u32 idx = 0; idx < count; idx++)                          \
		{                                                              \
			DENSITY_LAYER((p_density_profile_layers)[idx]);            \
			if (idx < count - 1)                                       \
			{                                                          \
				PUSH(",\n");                                           \
			}                                                          \
			else                                                       \
			{                                                          \
				PUSH("\n");                                            \
				PUSH("        )\n");                                   \
				PUSH("    )");                                         \
			}                                                          \
		}                                                              \
	}

	u32 i = 0;

	PUSH("#version 330\n");
	PUSH("\n");
	PUSH("#define TG_IN(x) const in x\n");
	PUSH("#define TG_OUT(x) out x\n");
	PUSH("#define TG_TEMPLATE(x)\n");
	PUSH("#define TG_TEMPLATE_ARGUMENT(x)\n");
	PUSH("#define TG_ASSERT(x)\n");
	PUSH("\n");
	PUSH("const int TG_TRANSMITTANCE_TEXTURE_WIDTH  = "); PUSH_I32(TG_TRANSMITTANCE_TEXTURE_WIDTH); PUSH(";\n");
	PUSH("const int TG_TRANSMITTANCE_TEXTURE_HEIGHT = "); PUSH_I32(TG_TRANSMITTANCE_TEXTURE_HEIGHT); PUSH(";\n");
	PUSH("const int TG_SCATTERING_TEXTURE_R_SIZE    = "); PUSH_I32(TG_SCATTERING_TEXTURE_R_SIZE); PUSH(";\n");
	PUSH("const int TG_SCATTERING_TEXTURE_MU_SIZE   = "); PUSH_I32(TG_SCATTERING_TEXTURE_MU_SIZE); PUSH(";\n");
	PUSH("const int TG_SCATTERING_TEXTURE_MU_S_SIZE = "); PUSH_I32(TG_SCATTERING_TEXTURE_MU_S_SIZE); PUSH(";\n");
	PUSH("const int TG_SCATTERING_TEXTURE_NU_SIZE   = "); PUSH_I32(TG_SCATTERING_TEXTURE_NU_SIZE); PUSH(";\n");
	PUSH("const int TG_IRRADIANCE_TEXTURE_WIDTH     = "); PUSH_I32(TG_IRRADIANCE_TEXTURE_WIDTH); PUSH(";\n");
	PUSH("const int TG_IRRADIANCE_TEXTURE_HEIGHT    = "); PUSH_I32(TG_IRRADIANCE_TEXTURE_HEIGHT); PUSH(";\n");
	PUSH("\n");

	if (combine_scattering_textures)
	{
		PUSH("#define TG_COMBINED_SCATTERING_TEXTURES\n");
		PUSH("\n");
	}
	// + definitions_glsl

	PUSH("const tg_atmosphere_parameters ATMOSPHERE = tg_atmosphere_parameters(\n");
	PUSH("    "); TO_STRING(p_solar_irrad, 1.0); PUSH(",\n");
	PUSH("    "); PUSH_F64(TG_SUN_ANGULAR_RADIUS); PUSH(",\n");
	PUSH("    "); PUSH_F64(TG_BOTTOM_RADIUS / TG_LENGTH_UNIT_IN_METERS); PUSH(",\n");
	PUSH("    "); PUSH_F64(TG_TOP_RADIUS / TG_LENGTH_UNIT_IN_METERS); PUSH(",\n");
	DENSITY_PROFILE(1, p_rayleigh_density); PUSH(",\n");
	PUSH("    "); TO_STRING(p_rayleigh_scattering, TG_LENGTH_UNIT_IN_METERS); PUSH(",\n");
	DENSITY_PROFILE(1, p_mie_density); PUSH(",\n");
	PUSH("    "); TO_STRING(p_mie_scattering, TG_LENGTH_UNIT_IN_METERS); PUSH(",\n");
	PUSH("    "); TO_STRING(p_mie_extinction, TG_LENGTH_UNIT_IN_METERS); PUSH(",\n");
	PUSH("    "); PUSH_F64(TG_MIE_PHASE_FUNCTION_G); PUSH(",\n");
	DENSITY_PROFILE(2, p_absorption_density); PUSH(",\n");
	PUSH("    "); TO_STRING(p_absorption_extinction, TG_LENGTH_UNIT_IN_METERS); PUSH(",\n");
	PUSH("    "); TO_STRING(p_ground_albedo, 1.0); PUSH(",\n");
	PUSH("    "); PUSH_F64(tgm_f64_cos(TG_MAX_SUN_ZENITH_ANGLE(use_half_precision))); PUSH("\n");
	PUSH("};\n");
	PUSH("\n");
	PUSH("const vec3 TG_SKY_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3("); PUSH_F64(sky_k_r); PUSH(", "); PUSH_F64(sky_k_g); PUSH(", "); PUSH_F64(sky_k_b); PUSH(");\n");
	PUSH("const vec3 TG_SUN_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3("); PUSH_F64(sun_k_r); PUSH(", "); PUSH_F64(sun_k_g); PUSH(", "); PUSH_F64(sun_k_b); PUSH(");\n");

	// + functions_glsl

	int bh = 0;

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

f64 tg__cie_color_matching_function_table_value(f64 wavelength, i32 column)
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

void tg__compute_spectral_radiance_to_luminance_factors(const f64* p_wavelengths, f64 lambda_power, f64* p_k_r, f64* p_k_g, f64* p_k_b)
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



void tg_atmosphere_precompute(void)
{
	tg_density_profile_layer rayleigh_layer = { 0 };
	rayleigh_layer.width = 0.0;
	rayleigh_layer.exp_term = 1.0;
	rayleigh_layer.exp_scale = -1.0 / TG_RAYLEIGH_SCALE_HEIGHT;
	rayleigh_layer.linear_term = 0.0;
	rayleigh_layer.constant_term = 0.0;

	tg_density_profile_layer mie_layer = { 0 };
	mie_layer.width = 0.0;
	mie_layer.exp_term = 1.0;
	mie_layer.exp_scale = -1.0 / TG_MIE_SCALE_HEIGHT;
	mie_layer.linear_term = 0.0;
	mie_layer.constant_term = 0.0;

	tg_density_profile_layer p_ozone_density[2] = { 0 };

	p_ozone_density[0].width = 25000.0;
	p_ozone_density[0].exp_term = 0.0;
	p_ozone_density[0].exp_scale = 0.0;
	p_ozone_density[0].linear_term = 1.0 / 15000.0;
	p_ozone_density[0].constant_term = -2.0 / 3.0;

	p_ozone_density[1].width = 0.0;
	p_ozone_density[1].exp_term = 0.0;
	p_ozone_density[1].exp_scale = 0.0;
	p_ozone_density[1].linear_term = -1.0 / 15000.0;
	p_ozone_density[1].constant_term = 8.0 / 3.0;

	f64 p_wavelengths[48] = { 0 };
	f64 p_solar_irrad[48] = { 0 };
	f64 p_rayleigh_scattering[48] = { 0 };
	f64 p_mie_scattering[48] = { 0 };
	f64 p_mie_extinction[48] = { 0 };
	f64 p_absorption_extinction[48] = { 0 };
	f64 p_ground_albedo[48] = { 0 };

	const b32 use_constant_solar_spectrum = TG_FALSE;
	const b32 use_ozone = TG_TRUE;
	const b32 use_combined_textures = TG_TRUE;
	const b32 use_half_precision = TG_TRUE;
	const tg_luminance use_luminance = TG_LUMINANCE_NONE;
	const b32 do_white_balance = TG_FALSE;
	const f64 view_distance_meters = 9000.0;
    const f64 view_zenith_angle_radians = 1.47;
	const f64 view_azimuth_angle_radians = -0.1;
	const f64 sun_zenith_angle_radians = 1.3;
	const f64 sun_azimuth_angle_radians = 2.9;
	const f64 exposure = 10.0;
	
	u32 i = 0;
	for (u32 l = TG_LAMBDA_MIN; l <= TG_LAMBDA_MAX; l += 10)
	{
		f64 lambda = (f64)l * 1e-3;
		f64 mie = TG_MIE_ANGSTROM_BETA / TG_MIE_SCALE_HEIGHT * tgm_f64_pow(lambda, -TG_MIE_ANGSTROM_ALPHA);

		p_wavelengths[i] = l;
		p_solar_irrad[i] = use_constant_solar_spectrum ? TG_CONSTANT_SOLAR_IRRADIANCE : p_solar_irradiance[(l - TG_LAMBDA_MIN) / 10];
		p_rayleigh_scattering[i] = TG_RAYLEIGH * tgm_f64_pow(lambda, -4.0);
		p_mie_scattering[i] = mie * TG_MIE_SINGLE_SCATTERING_ALBEDO;
		p_mie_extinction[i] = mie;
		p_absorption_extinction[i] = use_ozone ? TG_MAX_OZONE_NUMBER_DENSITY * p_ozone_cross_section[(l - TG_LAMBDA_MIN) / 10] : 0.0;
		p_ground_albedo[i] = TG_GROUND_ALBEDO;

		i++;
	}

	tg_model model = { 0 };

	model.num_precomputed_wavelengths = use_luminance == TG_LUMINANCE_PRECOMPUTED ? 15 : 3;
	model.half_precision = use_half_precision;
	model.rgb_format_supported = TG_FALSE; // TODO(torben): ask vulkan for this

	const b32 precompute_illuminance = model.num_precomputed_wavelengths > 3;
	f64 sky_k_r;
	f64 sky_k_g;
	f64 sky_k_b;
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

	f64 sun_k_r;
	f64 sun_k_g;
	f64 sun_k_b;
	tg__compute_spectral_radiance_to_luminance_factors(p_wavelengths, 0.0, &sun_k_r, &sun_k_g, &sun_k_b);

	tgvk_image transmittance_texture = tgvk_color_image_create(
		TG_TRANSMITTANCE_TEXTURE_WIDTH,
		TG_TRANSMITTANCE_TEXTURE_HEIGHT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		TG_NULL
	);

	const b32 combine_scattering_textures = TG_TRUE;
	tgvk_image_3d scattering_texture = tgvk_color_image_3d_create(
		TG_SCATTERING_TEXTURE_WIDTH,
		TG_SCATTERING_TEXTURE_HEIGHT,
		TG_SCATTERING_TEXTURE_DEPTH,
		model.half_precision ?
			(!combine_scattering_textures && model.rgb_format_supported ? VK_FORMAT_R16G16B16_SFLOAT : VK_FORMAT_R16G16B16A16_SFLOAT) :
			(!combine_scattering_textures && model.rgb_format_supported ? VK_FORMAT_R32G32B32_SFLOAT : VK_FORMAT_R32G32B32A32_SFLOAT),
		TG_NULL
	);

	tgvk_image_3d optional_single_mie_scatterin_texture = { 0 };
	if (combine_scattering_textures)
	{
		optional_single_mie_scatterin_texture = tgvk_color_image_3d_create(
			TG_SCATTERING_TEXTURE_WIDTH,
			TG_SCATTERING_TEXTURE_HEIGHT,
			TG_SCATTERING_TEXTURE_DEPTH,
			model.half_precision ?
				(model.rgb_format_supported ? VK_FORMAT_R16G16B16_SFLOAT : VK_FORMAT_R16G16B16A16_SFLOAT) :
				(model.rgb_format_supported ? VK_FORMAT_R32G32B32_SFLOAT : VK_FORMAT_R32G32B32A32_SFLOAT),
			TG_NULL
		);
	}
	
	tgvk_image irradiance_texture = tgvk_color_image_create(
		TG_IRRADIANCE_TEXTURE_WIDTH,
		TG_IRRADIANCE_TEXTURE_HEIGHT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		TG_NULL
	);

	char p_shader_buffer[4096] = { 0 };
	tg__glsl_header_factory(
		use_half_precision,
		combine_scattering_textures,
		
		p_wavelengths,
		p_solar_irrad,
		p_rayleigh_scattering,
		p_mie_scattering,
		p_mie_extinction,
		p_absorption_extinction,
		p_ground_albedo,

		&rayleigh_layer,
		&mie_layer,
		p_ozone_density,

		sky_k_r,
		sky_k_g,
		sky_k_b,
		sun_k_r,
		sun_k_g,
		sun_k_b,

		(u32)sizeof(p_shader_buffer),
		p_shader_buffer
	);
}



#endif
