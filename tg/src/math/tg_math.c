#include "math/tg_math.h"

#include "memory/tg_memory_allocator.h"

#ifndef TG_CPU_x64
#include <math.h>
#endif



/*------------------------------------------------------------+
| Noise                                                       |
+------------------------------------------------------------*/

const i32 gradient_table[][3] = {
	{  1,  1,  0 }, { -1,  1,  0}, {  1, -1,  0}, { -1, -1,  0 },
	{  1,  0,  1 }, { -1,  0,  1}, {  1,  0, -1}, { -1,  0, -1 },
	{  0,  1,  1 }, {  0, -1,  1}, {  0,  1, -1}, {  0, -1, -1 }
};

const u32 perm[] = {
	151, 160, 137,  91,  90,  15, 131,  13,
	201,  95,  96,  53, 194, 233,   7, 225,
	140,  36, 103,  30,  69, 142,   8,  99,
	 37, 240,  21,  10,  23, 190,   6, 148,
	247, 120, 234,  75,   0,  26, 197,  62,
	 94, 252, 219, 203, 117,  35,  11,  32,
	 57, 177,  33,  88, 237, 149,  56,  87,
	174,  20, 125, 136, 171, 168,  68, 175,
	 74, 165,  71, 134, 139,  48,  27, 166,
	 77, 146, 158, 231,  83, 111, 229, 122,
	 60, 211, 133, 230, 220, 105,  92,  41,
	 55,  46, 245,  40, 244, 102, 143,  54,
	 65,  25,  63, 161,   1, 216,  80,  73,
	209,  76, 132, 187, 208,  89,  18, 169,
	200, 196, 135, 130, 116, 188, 159,  86,
	164, 100, 109, 198, 173, 186,   3,  64,
	 52, 217, 226, 250, 124, 123,   5, 202,
	 38, 147, 118, 126, 255,  82,  85, 212,
	207, 206,  59, 227,  47,  16,  58,  17,
	182, 189,  28,  42, 223, 183, 170, 213,
	119, 248, 152,   2,  44, 154, 163,  70,
	221, 153, 101, 155, 167,  43, 172,   9,
	129,  22,  39, 253,  19,  98, 108, 110,
	 79, 113, 224, 232, 178, 185, 112, 104,
	218, 246,  97, 228, 251,  34, 242, 193,
	238, 210, 144,  12, 191, 179, 162, 241,
	 81,  51, 145, 235, 249,  14, 239, 107,
	 49, 192, 214,  31, 181, 199, 106, 157,
	184,  84, 204, 176, 115, 121,  50,  45,
	127,   4, 150, 254, 138, 236, 205,  93,
	222, 114,  67,  29,  24,  72, 243, 141,
	128, 195,  78,  66, 215,  61, 156, 180,

	151, 160, 137,  91,  90,  15, 131,  13,
	201,  95,  96,  53, 194, 233,   7, 225,
	140,  36, 103,  30,  69, 142,   8,  99,
	 37, 240,  21,  10,  23, 190,   6, 148,
	247, 120, 234,  75,   0,  26, 197,  62,
	 94, 252, 219, 203, 117,  35,  11,  32,
	 57, 177,  33,  88, 237, 149,  56,  87,
	174,  20, 125, 136, 171, 168,  68, 175,
	 74, 165,  71, 134, 139,  48,  27, 166,
	 77, 146, 158, 231,  83, 111, 229, 122,
	 60, 211, 133, 230, 220, 105,  92,  41,
	 55,  46, 245,  40, 244, 102, 143,  54,
	 65,  25,  63, 161,   1, 216,  80,  73,
	209,  76, 132, 187, 208,  89,  18, 169,
	200, 196, 135, 130, 116, 188, 159,  86,
	164, 100, 109, 198, 173, 186,   3,  64,
	 52, 217, 226, 250, 124, 123,   5, 202,
	 38, 147, 118, 126, 255,  82,  85, 212,
	207, 206,  59, 227,  47,  16,  58,  17,
	182, 189,  28,  42, 223, 183, 170, 213,
	119, 248, 152,   2,  44, 154, 163,  70,
	221, 153, 101, 155, 167,  43, 172,   9,
	129,  22,  39, 253,  19,  98, 108, 110,
	 79, 113, 224, 232, 178, 185, 112, 104,
	218, 246,  97, 228, 251,  34, 242, 193,
	238, 210, 144,  12, 191, 179, 162, 241,
	 81,  51, 145, 235, 249,  14, 239, 107,
	 49, 192, 214,  31, 181, 199, 106, 157,
	184,  84, 204, 176, 115, 121,  50,  45,
	127,   4, 150, 254, 138, 236, 205,  93,
	222, 114,  67,  29,  24,  72, 243, 141,
	128, 195,  78,  66, 215,  61, 156, 180
};

#define TGM_PERLIN_NOISE_FADE(t)            ((t) * (t) * (t) * ((t) * ((t) * 6.0f - 15.0f) + 10.0f))
#define TGM_PERLIN_NOISE_FASTFLOOR(x)       ((i32)(x) < (x) ? (i32)(x) : (i32)(x) - 1)
#define TGM_PERLIN_NOISE_LERP(v0, v1, t)    ((v0) + (t) * ((v1) - (v0)))

f32 tgm_perlin_noise_internal_grad(i32 hash, f32 x, f32 y, f32 z)
{
	const i32 h = hash & 15;
	const f32 u = h < 8 ? x : y;
	const f32 v = h < 4 ? y : h == 12 || h == 14 ? x : z;
	const f32 result = ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
	return result;
}

f32 tgm_perlin_noise(f32 x, f32 y, f32 z)
{
	i32 ix0, iy0, ix1, iy1, iz0, iz1;
	f32 fx0, fy0, fz0, fx1, fy1, fz1;
	f32 s, t, r;
	f32 nxy0, nxy1, nx0, nx1, n0, n1;

	ix0 = TGM_PERLIN_NOISE_FASTFLOOR(x);
	iy0 = TGM_PERLIN_NOISE_FASTFLOOR(y);
	iz0 = TGM_PERLIN_NOISE_FASTFLOOR(z);
	fx0 = x - ix0;
	fy0 = y - iy0;
	fz0 = z - iz0;
	fx1 = fx0 - 1.0f;
	fy1 = fy0 - 1.0f;
	fz1 = fz0 - 1.0f;
	ix1 = (ix0 + 1) & 0xff;
	iy1 = (iy0 + 1) & 0xff;
	iz1 = (iz0 + 1) & 0xff;
	ix0 = ix0 & 0xff;
	iy0 = iy0 & 0xff;
	iz0 = iz0 & 0xff;

	r = TGM_PERLIN_NOISE_FADE(fz0);
	t = TGM_PERLIN_NOISE_FADE(fy0);
	s = TGM_PERLIN_NOISE_FADE(fx0);

	nxy0 = tgm_perlin_noise_internal_grad(perm[ix0 + perm[iy0 + perm[iz0]]], fx0, fy0, fz0);
	nxy1 = tgm_perlin_noise_internal_grad(perm[ix0 + perm[iy0 + perm[iz1]]], fx0, fy0, fz1);
	nx0 = TGM_PERLIN_NOISE_LERP(r, nxy0, nxy1);

	nxy0 = tgm_perlin_noise_internal_grad(perm[ix0 + perm[iy1 + perm[iz0]]], fx0, fy1, fz0);
	nxy1 = tgm_perlin_noise_internal_grad(perm[ix0 + perm[iy1 + perm[iz1]]], fx0, fy1, fz1);
	nx1 = TGM_PERLIN_NOISE_LERP(r, nxy0, nxy1);

	n0 = TGM_PERLIN_NOISE_LERP(t, nx0, nx1);

	nxy0 = tgm_perlin_noise_internal_grad(perm[ix1 + perm[iy0 + perm[iz0]]], fx1, fy0, fz0);
	nxy1 = tgm_perlin_noise_internal_grad(perm[ix1 + perm[iy0 + perm[iz1]]], fx1, fy0, fz1);
	nx0 = TGM_PERLIN_NOISE_LERP(r, nxy0, nxy1);

	nxy0 = tgm_perlin_noise_internal_grad(perm[ix1 + perm[iy1 + perm[iz0]]], fx1, fy1, fz0);
	nxy1 = tgm_perlin_noise_internal_grad(perm[ix1 + perm[iy1 + perm[iz1]]], fx1, fy1, fz1);
	nx1 = TGM_PERLIN_NOISE_LERP(r, nxy0, nxy1);

	n1 = TGM_PERLIN_NOISE_LERP(t, nx0, nx1);

	return 0.936f * (TGM_PERLIN_NOISE_LERP(s, n0, n1));
}

#undef TGM_PERLIN_NOISE_FADE
#undef TGM_PERLIN_NOISE_FASTFLOOR
#undef TGM_PERLIN_NOISE_LERP



/*------------------------------------------------------------+
| Random                                                      |
+------------------------------------------------------------*/

typedef struct tgm_random_lcg
{
	u32    state;
} tgm_random_lcg;

typedef struct tgm_random_xorshift
{
	u32    state;
} tgm_random_xorshift;



tgm_random_lcg_h tgm_random_lcg_create(u32 seed)
{
	tgm_random_lcg_h random_lcg_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*random_lcg_h));
	random_lcg_h->state = seed;
	return random_lcg_h;
}

f32 tgm_random_lcg_next_f32(tgm_random_lcg_h random_lcg_h)
{
	const f32 result = (f32)tgm_random_lcg_next_ui32(random_lcg_h) / (f32)TG_U32_MAX;
	return result;
}

u32 tgm_random_lcg_next_ui32(tgm_random_lcg_h random_lcg_h)
{
	const u32 result = 1103515245 * random_lcg_h->state + 12345 % tgm_ui32_pow(2, 31);
	random_lcg_h->state = result;
	return result;
}

void tgm_random_lcg_destroy(tgm_random_lcg_h random_lcg_h)
{
	TG_MEMORY_ALLOCATOR_FREE(random_lcg_h);
}



tgm_random_xorshift_h tgm_random_xorshift_create(u32 seed)
{
	TG_ASSERT(seed);

	tgm_random_xorshift_h random_xorshift_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*random_xorshift_h));
	random_xorshift_h->state = seed;
	return random_xorshift_h;
}

f32 tgm_random_xorshift_next_f32(tgm_random_xorshift_h random_xorshift_h)
{
	const f32 result = (f32)tgm_random_xorshift_next_ui32(random_xorshift_h) / (f32)TG_U32_MAX;
	return result;
}

u32 tgm_random_xorshift_next_ui32(tgm_random_xorshift_h random_xorshift_h)
{
	u32 result = random_xorshift_h->state;
	result ^= result << 13;
	result ^= result >> 17;
	result ^= result << 5;
	random_xorshift_h->state = result;
	return result;
}

void tgm_random_xorshift_destroy(tgm_random_xorshift_h random_xorshift_h)
{
	TG_MEMORY_ALLOCATOR_FREE(random_xorshift_h);
}



/*------------------------------------------------------------+
| Intrinsics                                                  |
+------------------------------------------------------------*/

#ifdef TG_CPU_x64

f32 tgm_f32_arccos(f32 v)
{
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_acos_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

f32 tgm_f32_arccosh(f32 v)
{
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_acosh_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

f32 tgm_f32_arcsin(f32 v)
{
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_asin_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

f32 tgm_f32_arcsinh(f32 v)
{
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_asinh_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

f32 tgm_f32_arctan(f32 v)
{
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_atan_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

f32 tgm_f32_arctanh(f32 v)
{
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_atanh_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

f32 tgm_f32_cos(f32 v)
{
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_cos_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

f32 tgm_f32_cosh(f32 v)
{
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_cosh_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

f32 tgm_f32_floor(f32 v)
{
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_floor_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

f32 tgm_f32_log2(f32 v)
{
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_log2_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

f32 tgm_f32_sin(f32 v)
{
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_sin_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

f32 tgm_f32_sinh(f32 v)
{
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_sinh_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

f32 tgm_f32_sqrt(f32 v)
{
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_sqrt_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

f32 tgm_f32_tan(f32 v)
{
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_tan_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

f32 tgm_f32_tanh(f32 v)
{
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_tanh_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}



f64 tgm_f64_pow(f64 base, f64 exponent)
{
	const __m128d simd_base = _mm_set_sd(base);
	const __m128d simd_exponent = _mm_set_sd(exponent);
	const __m128d simd_result = _mm_pow_pd(simd_base, simd_exponent);
	const f64 result = simd_result.m128d_f64[0];
	return result;
}



i32 tgm_i32_abs(i32 v)
{
	TG_ASSERT(v != TG_I32_MIN);

	const __m128i simd_v = _mm_set1_epi32(v);
	const __m128i simd_result = _mm_abs_epi32(simd_v);
	const i32 result = simd_result.m128i_i32[0];
	return result;
}

f32 tgm_i32_log10(i32 v)
{
	const __m128 simd_v = _mm_set_ss((f32)v);
	const __m128 simd_result = _mm_log10_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

i32 tgm_i32_pow(i32 base, i32 exponent)
{
	const __m128 simd_base = _mm_set_ss((f32)base);
	const __m128 simd_exponent = _mm_set_ss((f32)exponent);
	const __m128 simd_result = _mm_pow_ps(simd_base, simd_exponent);
	const i32 result = (i32)simd_result.m128_f32[0];
	return result;
}



u32 tgm_ui32_floor(u32 v)
{
	const __m128 simd_v = _mm_set_ss((f32)v);
	const __m128 simd_result = _mm_floor_ps(simd_v);
	const u32 result = (u32)simd_result.m128_f32[0];
	return result;
}

f32 tgm_ui32_log10(u32 v)
{
	const __m128 simd_v = _mm_set_ss((f32)v);
	const __m128 simd_result = _mm_log10_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
	return result;
}

u32 tgm_ui32_pow(u32 base, u32 exponent)
{
	const __m128 simd_base = _mm_set_ss((f32)base);
	const __m128 simd_exponent = _mm_set_ss((f32)exponent);
	const __m128 simd_result = _mm_pow_ps(simd_base, simd_exponent);
	const u32 result = (u32)simd_result.m128_f32[0];
	return result;
}

#else

f32 tgm_f32_arccos(f32 v)
{
	const f32 result = acosf(v);
	return result;
}

f32 tgm_f32_arccosh(f32 v)
{
	const f32 result = acoshf(v);
	return result;
}

f32 tgm_f32_arcsin(f32 v)
{
	const f32 result = asinf(v);
	return result;
}

f32 tgm_f32_arcsinh(f32 v)
{
	const f32 result = asinhf(v);
	return result;
}

f32 tgm_f32_arctan(f32 v)
{
	const f32 result = atanf(v);
	return result;
}

f32 tgm_f32_arctanh(f32 v)
{
	const f32 result = atanhf(v);
	return result;
}

f32 tgm_f32_cos(f32 v)
{
	const f32 result = cosf(v);
	return result;
}

f32 tgm_f32_cosh(f32 v)
{
	const f32 result = coshf(v);
	return result;
}

f32 tgm_f32_floor(f32 v)
{
	const f32 result = floorf(v);
	return result;
}

f32 tgm_f32_log2(f32 v)
{
	const f32 result = logf(v);
	return result;
}

f32 tgm_f32_sin(f32 v)
{
	const f32 result = sinf(v);
	return result;
}

f32 tgm_f32_sinh(f32 v)
{
	const f32 result = sinhf(v);
	return result;
}

f32 tgm_f32_sqrt(f32 v)
{
	const f32 result = sqrtf(v);
	return result;
}

f32 tgm_f32_tan(f32 v)
{
	const f32 result = tanf(v);
	return result;
}

f32 tgm_f32_tanh(f32 v)
{
	const f32 result = tanhf(v);
	return result;
}



f64 tgm_f64_pow(f64 base, f64 exponent)
{
	const f64 result = pow(base, exponent);
	return result;
}



i32 tgm_i32_abs(i32 v)
{
	TG_ASSERT(v != TG_I32_MIN);

	const i32 result = v >= 0 ? v : -v;
	return result;
}

f32 tgm_i32_log10(i32 v)
{
	const f32 result = log10f((f32)v);
	return result;
}

i32 tgm_i32_pow(i32 base, i32 exponent)
{
	const i32 result = (i32)pow((f64)base, (f64)exponent);
	return result;
}



u32 tgm_ui32_floor(u32 v)
{
	const u32 result = (u32)floor((f64)v);
	return result;
}

f32 tgm_ui32_log10(u32 v)
{
	const f32 result = log10f((f32)v);
	return result;
}

u32 tgm_ui32_pow(u32 base, u32 exponent)
{
	const u32 result = (u32)pow((f64)base, (f64)exponent);
	return result;
}

#endif



/*------------------------------------------------------------+
| Functional                                                  |
+------------------------------------------------------------*/

f32 tgm_f32_clamp(f32 v, f32 low, f32 high)
{
	const f32 result = tgm_f32_max(low, tgm_f32_min(high, v));
	return result;
}

f32 tgm_f32_lerp(f32 v0, f32 v1, f32 t)
{
	const f32 result = (1.0f - t) * v0 + t * v1;
	return result;
}

f32 tgm_f32_max(f32 v0, f32 v1)
{
	const f32 result = v0 > v1 ? v0 : v1;
	return result;
}

f32 tgm_f32_min(f32 v0, f32 v1)
{
	const f32 result = v0 < v1 ? v0 : v1;
	return result;
}

i32 tgm_i32_clamp(i32 v, i32 low, i32 high)
{
	const i32 result = tgm_i32_max(low, tgm_i32_min(high, v));
	return result;
}

u32 tgm_i32_digits(i32 v)
{
	if (v == TG_I32_MIN)
	{
		v += 1;
	}
	const u32 result = v == 0 ? 1 : (u32)tgm_f32_floor(tgm_ui32_log10((u32)tgm_i32_abs(v))) + 1;
	return result;
}

i32 tgm_i32_max(i32 v0, i32 v1)
{
	const i32 result = v0 > v1 ? v0 : v1;
	return result;
}

i32 tgm_i32_min(i32 v0, i32 v1)
{
	const i32 result = v0 < v1 ? v0 : v1;
	return result;
}

u32 tgm_u32_clamp(u32 v, u32 low, u32 high)
{
	const u32 result = tgm_u32_max(low, tgm_u32_min(high, v));
	return result;
}

u32 tgm_u32_digits(u32 v)
{
	const u32 result = v == 0 ? 1 : (u32)tgm_f32_floor(tgm_ui32_log10(v)) + 1;
	return result;
}

u32 tgm_u32_max(u32 v0, u32 v1)
{
	const u32 result = v0 > v1 ? v0 : v1;
	return result;
}

u32 tgm_u32_min(u32 v0, u32 v1)
{
	const u32 result = v0 < v1 ? v0 : v1;
	return result;
}

u64 tgm_u64_max(u64 v0, u64 v1)
{
	const u64 result = v0 > v1 ? v0 : v1;
	return result;
}

u64 tgm_u64_min(u64 v0, u64 v1)
{
	const u64 result = v0 < v1 ? v0 : v1;
	return result;
}



/*------------------------------------------------------------+
| Vectors                                                     |
+------------------------------------------------------------*/

v2 tgm_v2_subtract_v2(const v2* p_v0, const v2* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	v2 result = { 0 };

	result.x = p_v0->x - p_v1->x;
	result.y = p_v0->y - p_v1->y;

	return result;
}



v3 tgm_v3_add_v3(const v3* p_v0, const v3* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	v3 result = { 0 };

	result.x = p_v0->x + p_v1->x;
	result.y = p_v0->y + p_v1->y;
	result.z = p_v0->z + p_v1->z;

	return result;
}

v3 tgm_v3_add_f(const v3* p_v, f32 f)
{
	TG_ASSERT(p_v);

	v3 result = { 0 };

	result.x = p_v->x + f;
	result.y = p_v->y + f;
	result.z = p_v->z + f;

	return result;
}

v3 tgm_v3_cross(const v3* p_v0, const v3* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	v3 result = { 0 };

	result.x = p_v0->y * p_v1->z - p_v0->z * p_v1->y;
	result.y = p_v0->z * p_v1->x - p_v0->x * p_v1->z;
	result.z = p_v0->x * p_v1->y - p_v0->y * p_v1->x;

	return result;
}

v3 tgm_v3_divide_v3(const v3* p_v0, const v3* p_v1)
{
	TG_ASSERT(p_v0 && p_v1 && p_v1->x && p_v1->y && p_v1->z);

	v3 result = { 0 };

	result.x = p_v0->x / p_v1->x;
	result.y = p_v0->y / p_v1->y;
	result.z = p_v0->z / p_v1->z;

	return result;
}

v3 tgm_v3_divide_f(const v3* p_v, f32 f)
{
	TG_ASSERT(p_v && f);

	v3 result = { 0 };

	result.x = p_v->x / f;
	result.y = p_v->y / f;
	result.z = p_v->z / f;

	return result;
}

f32 tgm_v3_dot(const v3* p_v0, const v3* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	const f32 result = p_v0->x * p_v1->x + p_v0->y * p_v1->y + p_v0->z * p_v1->z;
	return result;
}

b32 tgm_v3_equal(const v3* p_v0, const v3* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	const b32 result = p_v0 == p_v1 || p_v0->x == p_v1->x && p_v0->y == p_v1->y && p_v0->z == p_v1->z;
	return result;
}

f32 tgm_v3_magnitude(const v3* p_v)
{
	TG_ASSERT(p_v);

	const f32 result = tgm_f32_sqrt(p_v->x * p_v->x + p_v->y * p_v->y + p_v->z * p_v->z);
	return result;
}

f32 tgm_v3_magnitude_squared(const v3* p_v)
{
	TG_ASSERT(p_v);

	const f32 result = p_v->x * p_v->x + p_v->y * p_v->y + p_v->z * p_v->z;
	return result;
}

v3 tgm_v3_multiply_v3(const v3* p_v0, const v3* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	v3 result = { 0 };

	result.x = p_v0->x * p_v1->x;
	result.y = p_v0->y * p_v1->y;
	result.z = p_v0->z * p_v1->z;

	return result;
}

v3 tgm_v3_multiply_f(const v3* p_v, f32 f)
{
	TG_ASSERT(p_v);

	v3 result = { 0 };

	result.x = p_v->x * f;
	result.y = p_v->y * f;
	result.z = p_v->z * f;

	return result;
}

v3 tgm_v3_negated(const v3* p_v)
{
	TG_ASSERT(p_v);

	v3 result = { 0 };

	result.x = -p_v->x;
	result.y = -p_v->y;
	result.z = -p_v->z;

	return result;
}

v3 tgm_v3_normalized(const v3* p_v)
{
	TG_ASSERT(p_v);

	v3 result = { 0 };

	const f32 magnitude = tgm_v3_magnitude(p_v);
	TG_ASSERT(magnitude);

	result.x = p_v->x / magnitude;
	result.y = p_v->y / magnitude;
	result.z = p_v->z / magnitude;

	return result;
}

v3 tgm_v3_subtract_v3(const v3* p_v0, const v3* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	v3 result = { 0 };

	result.x = p_v0->x - p_v1->x;
	result.y = p_v0->y - p_v1->y;
	result.z = p_v0->z - p_v1->z;

	return result;
}

v3 tgm_v3_subtract_f(const v3* p_v, f32 f)
{
	TG_ASSERT(p_v);

	v3 result = { 0 };

	result.x = p_v->x - f;
	result.y = p_v->y - f;
	result.z = p_v->z - f;

	return result;
}

v4 tgm_v3_to_v4(const v3* p_v, f32 w)
{
	TG_ASSERT(p_v);

	v4 result = { 0 };

	result.x = p_v->x;
	result.y = p_v->y;
	result.z = p_v->z;
	result.w = w;

	return result;
}



v4 tgm_v4_add_v4(const v4* p_v0, const v4* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);
	
	v4 result = { 0 };

	result.x = p_v0->x + p_v1->x;
	result.y = p_v0->y + p_v1->y;
	result.z = p_v0->z + p_v1->z;
	result.w = p_v0->w + p_v1->w;

	return result;
}

v4 tgm_v4_add_f(const v4* p_v, f32 f)
{
	TG_ASSERT(p_v);

	v4 result = { 0 };

	result.x = p_v->x + f;
	result.y = p_v->y + f;
	result.z = p_v->z + f;
	result.w = p_v->w + f;

	return result;
}

v4 tgm_v4_divide_v4(const v4* p_v0, const v4* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	v4 result = { 0 };

	result.x = p_v0->x / p_v1->x;
	result.y = p_v0->y / p_v1->y;
	result.z = p_v0->z / p_v1->z;
	result.w = p_v0->w / p_v1->w;

	return result;
}

v4 tgm_v4_divide_f(const v4* p_v, f32 f)
{
	TG_ASSERT(p_v);
	
	v4 result = { 0 };

	result.x = p_v->x / f;
	result.y = p_v->y / f;
	result.z = p_v->z / f;
	result.w = p_v->w / f;

	return result;
}

f32 tgm_v4_dot(const v4* p_v0, const v4* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	const f32 result = p_v0->x * p_v1->x + p_v0->y * p_v1->y + p_v0->z * p_v1->z + p_v0->w * p_v1->w;
	return result;
}

b32 tgm_v4_equal(const v4* p_v0, const v4* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	const b32 result = p_v0 == p_v1 || p_v0->x == p_v1->x && p_v0->y == p_v1->y && p_v0->z == p_v1->z && p_v0->w == p_v1->w;
	return result;
}

f32 tgm_v4_magnitude(const v4* p_v)
{
	TG_ASSERT(p_v);

	const f32 result = tgm_f32_sqrt(p_v->x * p_v->x + p_v->y * p_v->y + p_v->z * p_v->z + p_v->w * p_v->w);
	return result;
}

f32 tgm_v4_magnitude_squared(const v4* p_v)
{
	TG_ASSERT(p_v);

	const f32 result = p_v->x * p_v->x + p_v->y * p_v->y + p_v->z * p_v->z + p_v->w * p_v->w;
	return result;
}

v4 tgm_v4_multiply_v4(const v4* p_v0, const v4* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	v4 result = { 0 };

	result.x = p_v0->x * p_v1->x;
	result.y = p_v0->y * p_v1->y;
	result.z = p_v0->z * p_v1->z;
	result.w = p_v0->w * p_v1->w;

	return result;
}

v4 tgm_v4_multiply_f(const v4* p_v, f32 f)
{
	TG_ASSERT(p_v);

	v4 result = { 0 };

	result.x = p_v->x * f;
	result.y = p_v->y * f;
	result.z = p_v->z * f;
	result.w = p_v->w * f;

	return result;
}

v4 tgm_v4_negated(const v4* p_v)
{
	TG_ASSERT(p_v);

	v4 result = { 0 };

	result.x = -p_v->x;
	result.y = -p_v->y;
	result.z = -p_v->z;
	result.w = -p_v->w;

	return result;
}

v4 tgm_v4_normalized(const v4* p_v)
{
	TG_ASSERT(p_v);

	v4 result = { 0 };

	const f32 magnitude = tgm_v4_magnitude(p_v);
	TG_ASSERT(magnitude);

	result.x = p_v->x / magnitude;
	result.y = p_v->y / magnitude;
	result.z = p_v->z / magnitude;
	result.w = p_v->w / magnitude;

	return result;
}

v4 tgm_v4_subtract_v4(const v4* p_v0, const v4* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	v4 result = { 0 };

	result.x = p_v0->x - p_v1->x;
	result.y = p_v0->y - p_v1->y;
	result.z = p_v0->z - p_v1->z;
	result.w = p_v0->w - p_v1->w;

	return result;
}

v4 tgm_v4_subtract_f(const v4* p_v, f32 f)
{
	TG_ASSERT(p_v);

	v4 result = { 0 };

	result.x = p_v->x - f;
	result.y = p_v->y - f;
	result.z = p_v->z - f;
	result.w = p_v->w - f;

	return result;
}

v3 tgm_v4_to_v3(const v4* p_v)
{
	TG_ASSERT(p_v);

	v3 result = { 0 };

	if (p_v->w == 0.0f)
	{
		result.x = p_v->x;
		result.y = p_v->y;
		result.z = p_v->z;
	}
	else
	{
		result.x = p_v->x / p_v->w;
		result.y = p_v->y / p_v->w;
		result.z = p_v->z / p_v->w;
	}
	return result;
}



/*------------------------------------------------------------+
| Matrices                                                    |
+------------------------------------------------------------*/

m2 tgm_m2_identity()
{
	m2 result = { 0 };

	result.m00 = 1.0f;
	result.m10 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = 1.0f;

	return result;
}

m2 tgm_m2_multiply_m2(const m2* p_m0, const m2* p_m1)
{
	TG_ASSERT(p_m0 && p_m1);

	m2 result = { 0 };

	result.m00 = p_m0->m00 * p_m1->m00 + p_m0->m01 * p_m1->m10;
	result.m10 = p_m0->m10 * p_m1->m00 + p_m0->m11 * p_m1->m10;

	result.m01 = p_m0->m00 * p_m1->m01 + p_m0->m01 * p_m1->m11;
	result.m11 = p_m0->m10 * p_m1->m01 + p_m0->m11 * p_m1->m11;

	return result;
}

v2 tgm_m2_multiply_v2(const m2* p_m, const v2* p_v)
{
	TG_ASSERT(p_m && p_v);

	v2 result = { 0 };

	result.x = p_v->x * p_m->m00 + p_v->y * p_m->m01;
	result.y = p_v->x * p_m->m10 + p_v->y * p_m->m11;

	return result;
}

m2 tgm_m2_transposed(const m2* p_m)
{
	TG_ASSERT(p_m);

	m2 result = { 0 };

	result.m00 = p_m->m00;
	result.m10 = p_m->m01;
	result.m01 = p_m->m10;
	result.m11 = p_m->m11;

	return result;
}



m3 tgm_m3_identity()
{
	m3 result = { 0 };

	result.m00 = 1.0f;
	result.m10 = 0.0f;
	result.m20 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = 1.0f;
	result.m21 = 0.0f;

	result.m02 = 0.0f;
	result.m12 = 0.0f;
	result.m22 = 1.0f;

	return result;
}

m3 tgm_m3_multiply_m3(const m3* p_m0, const m3* p_m1)
{
	TG_ASSERT(p_m0 && p_m1);

	m3 result = { 0 };

	result.m00 = p_m0->m00 * p_m1->m00 + p_m0->m01 * p_m1->m10 + p_m0->m02 * p_m1->m20;
	result.m10 = p_m0->m10 * p_m1->m00 + p_m0->m11 * p_m1->m10 + p_m0->m12 * p_m1->m20;
	result.m20 = p_m0->m20 * p_m1->m00 + p_m0->m21 * p_m1->m10 + p_m0->m22 * p_m1->m20;

	result.m01 = p_m0->m00 * p_m1->m01 + p_m0->m01 * p_m1->m11 + p_m0->m02 * p_m1->m21;
	result.m11 = p_m0->m10 * p_m1->m01 + p_m0->m11 * p_m1->m11 + p_m0->m12 * p_m1->m21;
	result.m21 = p_m0->m20 * p_m1->m01 + p_m0->m21 * p_m1->m11 + p_m0->m22 * p_m1->m21;

	result.m02 = p_m0->m00 * p_m1->m02 + p_m0->m01 * p_m1->m12 + p_m0->m02 * p_m1->m22;
	result.m12 = p_m0->m10 * p_m1->m02 + p_m0->m11 * p_m1->m12 + p_m0->m12 * p_m1->m22;
	result.m22 = p_m0->m20 * p_m1->m02 + p_m0->m21 * p_m1->m12 + p_m0->m22 * p_m1->m22;

	return result;
}

v3 tgm_m3_multiply_v3(const m3* p_m, const v3* p_v)
{
	TG_ASSERT(p_m && p_v);

	v3 result = { 0 };

	result.x = p_v->x * p_m->m00 + p_v->y * p_m->m01 + p_v->z * p_m->m02;
	result.y = p_v->x * p_m->m10 + p_v->y * p_m->m11 + p_v->z * p_m->m12;
	result.z = p_v->x * p_m->m20 + p_v->y * p_m->m21 + p_v->z * p_m->m22;

	return result;
}

m3 tgm_m3_orthographic(f32 left, f32 right, f32 bottom, f32 top)
{
	TG_ASSERT(right != left && top != bottom);

	m3 result = { 0 };

	result.m00 = 2.0f / (right - left);
	result.m10 = 0.0f;
	result.m20 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = -2.0f / (top - bottom);
	result.m21 = 0.0f;

	result.m02 = -(right + left) / (right - left);
	result.m12 = -(bottom + top) / (bottom - top);
	result.m22 = 1.0f;

	return result;
}

m3 tgm_m3_transposed(const m3* p_m)
{
	TG_ASSERT(p_m);

	m3 result = { 0 };

	result.m00 = p_m->m00;
	result.m10 = p_m->m01;
	result.m20 = p_m->m01;

	result.m01 = p_m->m10;
	result.m11 = p_m->m11;
	result.m21 = p_m->m12;

	result.m02 = p_m->m20;
	result.m12 = p_m->m21;
	result.m22 = p_m->m22;

	return result;
}



m4 tgm_m4_angle_axis(f32 angle_in_radians, const v3* p_axis)
{
	TG_ASSERT(p_axis);

	m4 result = { 0 };

	const f32 c = tgm_f32_cos(angle_in_radians);
	const f32 s = tgm_f32_sin(angle_in_radians);
	const f32 omc = 1.0f - c;
	const f32 l = tgm_v3_magnitude(p_axis);
	TG_ASSERT(l);
	const f32 x = p_axis->x / l;
	const f32 y = p_axis->y / l;
	const f32 z = p_axis->z / l;

	result.m00 = x * x * omc + c;
	result.m10 = y * x * omc + z * s;
	result.m20 = z * x * omc - y * s;
	result.m30 = 0.0f;

	result.m01 = x * y * omc - z * s;
	result.m11 = y * y * omc + c;
	result.m21 = z * y * omc + x * s;
	result.m31 = 0.0f;

	result.m02 = x * z * omc + y * s;
	result.m12 = y * z * omc - x * s;
	result.m22 = z * z * omc + c;
	result.m32 = 0.0f;

	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	return result;
}

f32 tgm_m4_determinant(const m4* p_m)
{
	TG_ASSERT(p_m);

	const f32 result =
		p_m->m03 * p_m->m12 * p_m->m21 * p_m->m30 - p_m->m02 * p_m->m03 * p_m->m21 * p_m->m30 -
		p_m->m03 * p_m->m11 * p_m->m22 * p_m->m30 + p_m->m01 * p_m->m03 * p_m->m22 * p_m->m30 +
		p_m->m02 * p_m->m11 * p_m->m23 * p_m->m30 - p_m->m01 * p_m->m02 * p_m->m23 * p_m->m30 -
		p_m->m03 * p_m->m12 * p_m->m20 * p_m->m31 + p_m->m02 * p_m->m03 * p_m->m20 * p_m->m31 +
		p_m->m03 * p_m->m10 * p_m->m22 * p_m->m31 - p_m->m00 * p_m->m03 * p_m->m22 * p_m->m31 -
		p_m->m02 * p_m->m10 * p_m->m23 * p_m->m31 + p_m->m00 * p_m->m02 * p_m->m23 * p_m->m31 +
		p_m->m03 * p_m->m11 * p_m->m20 * p_m->m32 - p_m->m01 * p_m->m03 * p_m->m20 * p_m->m32 -
		p_m->m03 * p_m->m10 * p_m->m21 * p_m->m32 + p_m->m00 * p_m->m03 * p_m->m21 * p_m->m32 +
		p_m->m01 * p_m->m10 * p_m->m23 * p_m->m32 - p_m->m00 * p_m->m01 * p_m->m23 * p_m->m32 -
		p_m->m02 * p_m->m11 * p_m->m20 * p_m->m33 + p_m->m01 * p_m->m02 * p_m->m20 * p_m->m33 +
		p_m->m02 * p_m->m10 * p_m->m21 * p_m->m33 - p_m->m00 * p_m->m02 * p_m->m21 * p_m->m33 -
		p_m->m01 * p_m->m10 * p_m->m22 * p_m->m33 + p_m->m00 * p_m->m01 * p_m->m22 * p_m->m33;

	return result;
}

m4 tgm_m4_euler(f32 pitch_in_radians, f32 yaw_in_radians, f32 roll_in_radians)
{
	const m4 x = tgm_m4_rotate_x(pitch_in_radians);
	const m4 y = tgm_m4_rotate_y(yaw_in_radians);
	const m4 z = tgm_m4_rotate_z(roll_in_radians);
	const m4 yx = tgm_m4_multiply_m4(&y, &x);
	return yx;
	const m4 zyx = tgm_m4_multiply_m4(&z, &yx);

	return zyx;
}

m4 tgm_m4_identity()
{
	m4 result = { 0 };

	result.m00 = 1.0f;
	result.m10 = 0.0f;
	result.m20 = 0.0f;
	result.m30 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = 1.0f;
	result.m21 = 0.0f;
	result.m31 = 0.0f;

	result.m02 = 0.0f;
	result.m12 = 0.0f;
	result.m22 = 1.0f;
	result.m32 = 0.0f;

	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	return result;
}

m4 tgm_m4_inverse(const m4* p_m)
{
	m4 result = { 0 };

	const f32 m2323 = p_m->m22 * p_m->m33 - p_m->m23 * p_m->m32;
	const f32 m1323 = p_m->m21 * p_m->m33 - p_m->m23 * p_m->m31;
	const f32 m1223 = p_m->m21 * p_m->m32 - p_m->m22 * p_m->m31;
	const f32 m0323 = p_m->m20 * p_m->m33 - p_m->m23 * p_m->m30;
	const f32 m0223 = p_m->m20 * p_m->m32 - p_m->m22 * p_m->m30;
	const f32 m0123 = p_m->m20 * p_m->m31 - p_m->m21 * p_m->m30;
	const f32 m2313 = p_m->m12 * p_m->m33 - p_m->m13 * p_m->m32;
	const f32 m1313 = p_m->m11 * p_m->m33 - p_m->m13 * p_m->m31;
	const f32 m1213 = p_m->m11 * p_m->m32 - p_m->m12 * p_m->m31;
	const f32 m2312 = p_m->m12 * p_m->m23 - p_m->m13 * p_m->m22;
	const f32 m1312 = p_m->m11 * p_m->m23 - p_m->m13 * p_m->m21;
	const f32 m1212 = p_m->m11 * p_m->m22 - p_m->m12 * p_m->m21;
	const f32 m0313 = p_m->m10 * p_m->m33 - p_m->m13 * p_m->m30;
	const f32 m0213 = p_m->m10 * p_m->m32 - p_m->m12 * p_m->m30;
	const f32 m0312 = p_m->m10 * p_m->m23 - p_m->m13 * p_m->m20;
	const f32 m0212 = p_m->m10 * p_m->m22 - p_m->m12 * p_m->m20;
	const f32 m0113 = p_m->m10 * p_m->m31 - p_m->m11 * p_m->m30;
	const f32 m0112 = p_m->m10 * p_m->m21 - p_m->m11 * p_m->m20;

	const f32 det = 1.0f / (
		p_m->m00 * (p_m->m11 * m2323 - p_m->m12 * m1323 + p_m->m13 * m1223) -
		p_m->m01 * (p_m->m10 * m2323 - p_m->m12 * m0323 + p_m->m13 * m0223) +
		p_m->m02 * (p_m->m10 * m1323 - p_m->m11 * m0323 + p_m->m13 * m0123) -
		p_m->m03 * (p_m->m10 * m1223 - p_m->m11 * m0223 + p_m->m12 * m0123));

	result.m00 = det * (p_m->m11 * m2323 - p_m->m12 * m1323 + p_m->m13 * m1223);
	result.m01 = det * -(p_m->m01 * m2323 - p_m->m02 * m1323 + p_m->m03 * m1223);
	result.m02 = det * (p_m->m01 * m2313 - p_m->m02 * m1313 + p_m->m03 * m1213);
	result.m03 = det * -(p_m->m01 * m2312 - p_m->m02 * m1312 + p_m->m03 * m1212);
	result.m10 = det * -(p_m->m10 * m2323 - p_m->m12 * m0323 + p_m->m13 * m0223);
	result.m11 = det * (p_m->m00 * m2323 - p_m->m02 * m0323 + p_m->m03 * m0223);
	result.m12 = det * -(p_m->m00 * m2313 - p_m->m02 * m0313 + p_m->m03 * m0213);
	result.m13 = det * (p_m->m00 * m2312 - p_m->m02 * m0312 + p_m->m03 * m0212);
	result.m20 = det * (p_m->m10 * m1323 - p_m->m11 * m0323 + p_m->m13 * m0123);
	result.m21 = det * -(p_m->m00 * m1323 - p_m->m01 * m0323 + p_m->m03 * m0123);
	result.m22 = det * (p_m->m00 * m1313 - p_m->m01 * m0313 + p_m->m03 * m0113);
	result.m23 = det * -(p_m->m00 * m1312 - p_m->m01 * m0312 + p_m->m03 * m0112);
	result.m30 = det * -(p_m->m10 * m1223 - p_m->m11 * m0223 + p_m->m12 * m0123);
	result.m31 = det * (p_m->m00 * m1223 - p_m->m01 * m0223 + p_m->m02 * m0123);
	result.m32 = det * -(p_m->m00 * m1213 - p_m->m01 * m0213 + p_m->m02 * m0113);
	result.m33 = det * (p_m->m00 * m1212 - p_m->m01 * m0212 + p_m->m02 * m0112);

	return result;
}

m4 tgm_m4_look_at(const v3* p_from, const v3* p_to, const v3* p_up)
{
	TG_ASSERT(p_from && p_to && p_up && !tgm_v3_equal(p_from, p_to) && !tgm_v3_equal(p_from, p_up) && !tgm_v3_equal(p_to, p_up));

	m4 result = { 0 };
	v3 temp = { 0 };

	temp = tgm_v3_subtract_v3(p_to, p_from);
	const v3 f_negated = tgm_v3_normalized(&temp);

	temp = tgm_v3_normalized(p_up);
	temp = tgm_v3_cross(&f_negated, &temp);
	const v3 r = tgm_v3_normalized(&temp);

	temp = tgm_v3_cross(&r, &f_negated);
	const v3 u = tgm_v3_normalized(&temp);

	const v3 f = tgm_v3_negated(&f_negated);

	v3 from_negated = tgm_v3_negated(p_from);

	result.m00 = r.x;
	result.m10 = u.x;
	result.m20 = f.x;
	result.m30 = 0.0f;

	result.m01 = r.y;
	result.m11 = u.y;
	result.m21 = f.y;
	result.m31 = 0.0f;

	result.m02 = r.z;
	result.m12 = u.z;
	result.m22 = f.z;
	result.m32 = 0.0f;

	result.m03 = tgm_v3_dot(&r, &from_negated);
	result.m13 = tgm_v3_dot(&u, &from_negated);
	result.m23 = tgm_v3_dot(&f, &from_negated);
	result.m33 = 1.0f;

	return result;
}

m4 tgm_m4_multiply_m4(const m4* p_m0, const m4* p_m1)
{
	TG_ASSERT(p_m0 && p_m1);

	m4 result = { 0 };

	result.m00 = p_m0->m00 * p_m1->m00 + p_m0->m01 * p_m1->m10 + p_m0->m02 * p_m1->m20 + p_m0->m03 * p_m1->m30;
	result.m10 = p_m0->m10 * p_m1->m00 + p_m0->m11 * p_m1->m10 + p_m0->m12 * p_m1->m20 + p_m0->m13 * p_m1->m30;
	result.m20 = p_m0->m20 * p_m1->m00 + p_m0->m21 * p_m1->m10 + p_m0->m22 * p_m1->m20 + p_m0->m23 * p_m1->m30;
	result.m30 = p_m0->m30 * p_m1->m00 + p_m0->m31 * p_m1->m10 + p_m0->m32 * p_m1->m20 + p_m0->m33 * p_m1->m30;

	result.m01 = p_m0->m00 * p_m1->m01 + p_m0->m01 * p_m1->m11 + p_m0->m02 * p_m1->m21 + p_m0->m03 * p_m1->m31;
	result.m11 = p_m0->m10 * p_m1->m01 + p_m0->m11 * p_m1->m11 + p_m0->m12 * p_m1->m21 + p_m0->m13 * p_m1->m31;
	result.m21 = p_m0->m20 * p_m1->m01 + p_m0->m21 * p_m1->m11 + p_m0->m22 * p_m1->m21 + p_m0->m23 * p_m1->m31;
	result.m31 = p_m0->m30 * p_m1->m01 + p_m0->m31 * p_m1->m11 + p_m0->m32 * p_m1->m21 + p_m0->m33 * p_m1->m31;

	result.m02 = p_m0->m00 * p_m1->m02 + p_m0->m01 * p_m1->m12 + p_m0->m02 * p_m1->m22 + p_m0->m03 * p_m1->m32;
	result.m12 = p_m0->m10 * p_m1->m02 + p_m0->m11 * p_m1->m12 + p_m0->m12 * p_m1->m22 + p_m0->m13 * p_m1->m32;
	result.m22 = p_m0->m20 * p_m1->m02 + p_m0->m21 * p_m1->m12 + p_m0->m22 * p_m1->m22 + p_m0->m23 * p_m1->m32;
	result.m32 = p_m0->m30 * p_m1->m02 + p_m0->m31 * p_m1->m12 + p_m0->m32 * p_m1->m22 + p_m0->m33 * p_m1->m32;

	result.m03 = p_m0->m00 * p_m1->m03 + p_m0->m01 * p_m1->m13 + p_m0->m02 * p_m1->m23 + p_m0->m03 * p_m1->m33;
	result.m13 = p_m0->m10 * p_m1->m03 + p_m0->m11 * p_m1->m13 + p_m0->m12 * p_m1->m23 + p_m0->m13 * p_m1->m33;
	result.m23 = p_m0->m20 * p_m1->m03 + p_m0->m21 * p_m1->m13 + p_m0->m22 * p_m1->m23 + p_m0->m23 * p_m1->m33;
	result.m33 = p_m0->m30 * p_m1->m03 + p_m0->m31 * p_m1->m13 + p_m0->m32 * p_m1->m23 + p_m0->m33 * p_m1->m33;

	return result;
}

v4 tgm_m4_multiply_v4(const m4* p_m, const v4* p_v)
{
	TG_ASSERT(p_m && p_v);

	v4 result = { 0 };

	result.x = p_v->x * p_m->m00 + p_v->y * p_m->m01 + p_v->z * p_m->m02 + p_v->w * p_m->m03;
	result.y = p_v->x * p_m->m10 + p_v->y * p_m->m11 + p_v->z * p_m->m12 + p_v->w * p_m->m13;
	result.z = p_v->x * p_m->m20 + p_v->y * p_m->m21 + p_v->z * p_m->m22 + p_v->w * p_m->m23;
	result.w = p_v->x * p_m->m30 + p_v->y * p_m->m31 + p_v->z * p_m->m32 + p_v->w * p_m->m33;

	return result;
}

m4 tgm_m4_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near)
{
	TG_ASSERT(left != right && top != bottom && near != far);

	m4 result = { 0 };

	result.m00 = 2.0f / (right - left);
	result.m10 = 0.0f;
	result.m20 = 0.0f;
	result.m30 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = -2.0f / (top - bottom);
	result.m21 = 0.0f;
	result.m31 = 0.0f;

	result.m02 = 0.0f;
	result.m12 = 0.0f;
	result.m22 = -1.0f / (near - far);
	result.m32 = 0.0f;

	result.m03 = -(right + left) / (right - left);
	result.m13 = -(bottom + top) / (bottom - top);
	result.m23 = near / (near - far);
	result.m33 = 1.0f;

	return result;
}

m4 tgm_m4_perspective(f32 fov_y_in_radians, f32 aspect, f32 near, f32 far)
{
	m4 result = { 0 };

	const f32 tan_half_fov_y = tgm_f32_tan(fov_y_in_radians / 2.0f);

	const f32 a = -far / (far - near);
	const f32 b = -(2.0f * far * near) / (near - far);

	result.m00 = 1.0f / (aspect * tan_half_fov_y);
	result.m10 = 0.0f;
	result.m20 = 0.0f;
	result.m30 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = -1.0f / tan_half_fov_y;
	result.m21 = 0.0f;
	result.m31 = 0.0f;

	result.m02 = 0.0f;
	result.m12 = 0.0f;
	result.m22 = a;
	result.m32 = -1.0f;

	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = b;
	result.m33 = 0.0f;

	return result;
}

m4 tgm_m4_rotate_x(f32 angle_in_radians)
{
	m4 result = { 0 };

	result.m00 = 1.0f;
	result.m10 = 0.0f;
	result.m20 = 0.0f;
	result.m30 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = tgm_f32_cos(angle_in_radians);
	result.m21 = tgm_f32_sin(angle_in_radians);
	result.m31 = 0.0f;

	result.m02 = 0.0f;
	result.m12 = -tgm_f32_sin(angle_in_radians);
	result.m22 = tgm_f32_cos(angle_in_radians);
	result.m32 = 0.0f;

	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	return result;
}

m4 tgm_m4_rotate_y(f32 angle_in_radians)
{
	m4 result = { 0 };

	result.m00 = tgm_f32_cos(angle_in_radians);
	result.m10 = 0.0f;
	result.m20 = -tgm_f32_sin(angle_in_radians);
	result.m30 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = 1.0f;
	result.m21 = 0.0f;
	result.m31 = 0.0f;

	result.m02 = tgm_f32_sin(angle_in_radians);
	result.m12 = 0.0f;
	result.m22 = tgm_f32_cos(angle_in_radians);
	result.m32 = 0.0f;

	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	return result;
}

m4 tgm_m4_rotate_z(f32 angle_in_radians)
{
	m4 result = { 0 };

	result.m00 = tgm_f32_cos(angle_in_radians);
	result.m10 = tgm_f32_sin(angle_in_radians);
	result.m20 = 0.0f;
	result.m30 = 0.0f;

	result.m01 = -tgm_f32_sin(angle_in_radians);
	result.m11 = tgm_f32_cos(angle_in_radians);
	result.m21 = 0.0f;
	result.m31 = 0.0f;

	result.m02 = 0.0f;
	result.m12 = 0.0f;
	result.m22 = 1.0f;
	result.m32 = 0.0f;

	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	return result;
}

m4 tgm_m4_translate(const v3* p_v)
{
	TG_ASSERT(p_v);

	m4 result = { 0 };

	result.m00 = 1.0f;
	result.m10 = 0.0f;
	result.m20 = 0.0f;
	result.m30 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = 1.0f;
	result.m21 = 0.0f;
	result.m31 = 0.0f;

	result.m02 = 0.0f;
	result.m12 = 0.0f;
	result.m22 = 1.0f;
	result.m32 = 0.0f;

	result.m03 = p_v->x;
	result.m13 = p_v->y;
	result.m23 = p_v->z;
	result.m33 = 1.0f;

	return result;
}

m4 tgm_m4_transposed(const m4* p_m)
{
	TG_ASSERT(p_m);

	m4 result = { 0 };

	result.m00 = p_m->m00;
	result.m10 = p_m->m01;
	result.m20 = p_m->m02;
	result.m30 = p_m->m03;

	result.m01 = p_m->m10;
	result.m11 = p_m->m11;
	result.m21 = p_m->m12;
	result.m31 = p_m->m13;

	result.m02 = p_m->m20;
	result.m12 = p_m->m21;
	result.m22 = p_m->m22;
	result.m32 = p_m->m23;

	result.m03 = p_m->m30;
	result.m13 = p_m->m31;
	result.m23 = p_m->m32;
	result.m33 = p_m->m33;

	return result;
}