#include "math/tg_math.h"

#include "memory/tg_memory_allocator.h"

#ifdef _M_X64
#include <immintrin.h>
#else
#include <math.h>
#endif

#include <string.h>



/*------------------------------------------------------------+
| Random                                                      |
+------------------------------------------------------------*/

typedef struct tgm_random_lcg
{
	u32    state;
} tgm_random_lcg;



void tgm_random_lcg_create(u32 seed, tgm_random_lcg_h* p_random_lcg_h)
{
	*p_random_lcg_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(**p_random_lcg_h));
	(**p_random_lcg_h).state = seed;
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



typedef struct tgm_random_xorshift
{
	u32    state;
} tgm_random_xorshift;



void tgm_random_xorshift_create(u32 seed, tgm_random_xorshift_h* p_random_xorshift_h)
{
	TG_ASSERT(seed);

	*p_random_xorshift_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(**p_random_xorshift_h));
	(**p_random_xorshift_h).state = seed;
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

f32 tgm_f32_cos(f32 v)
{
#ifdef _M_X64
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_cos_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
#else
	const f32 result = cosf(v);
#endif
	return result;
}

f32 tgm_f32_floor(f32 v)
{
#ifdef _M_X64
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_floor_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
#else
	const f32 result = floorf(v);
#endif
	return result;
}

f32 tgm_f32_log2(f32 v)
{
#ifdef _M_X64
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_log2_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
#else
	const f32 result = logf(v);
#endif
	return result;
}

f32 tgm_f32_sin(f32 v)
{
#ifdef _M_X64
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_sin_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
#else
	const f32 result = sinf(v);
#endif
	return result;
}

f32 tgm_f32_sqrt(f32 v)
{
#ifdef _M_X64
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_sqrt_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
#else
	const f32 result = sqrtf(v);
#endif
	return result;
}

f32 tgm_f32_tan(f32 v)
{
#ifdef _M_X64
	const __m128 simd_v = _mm_set_ss(v);
	const __m128 simd_result = _mm_tan_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
#else
	const f32 result = tanf(v);
#endif
	return result;
}



f64 tgm_f64_pow(f64 base, f64 exponent)
{
#ifdef _M_X64
	const __m128d simd_base = _mm_set_sd(base);
	const __m128d simd_exponent = _mm_set_sd(exponent);
	const __m128d simd_result = _mm_pow_pd(simd_base, simd_exponent);
	const f64 result = simd_result.m128d_f64[0];
#else
	const f64 result = pow(base, exponent);
#endif
	return result;
}



i32 tgm_i32_abs(i32 v)
{
	TG_ASSERT(v != TG_I32_MIN);

	const i32 result = v >= 0 ? v : -v;
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

f32 tgm_i32_log10(i32 v)
{
#ifdef _M_X64
	const __m128 simd_v = _mm_set_ss((f32)v);
	const __m128 simd_result = _mm_log10_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
#else
	const f32 result = log10f((f32)v);
#endif
	return result;
}

i32 tgm_i32_pow(i32 base, i32 exponent)
{
#ifdef _M_X64
	const __m128 simd_base = _mm_set_ss((f32)base);
	const __m128 simd_exponent = _mm_set_ss((f32)exponent);
	const __m128 simd_result = _mm_pow_ps(simd_base, simd_exponent);
	const i32 result = (i32)simd_result.m128_f32[0];
#else
	const i32 result = (i32)pow((f64)base, (f64)exponent);
#endif
	return result;
}



u32 tgm_ui32_digits(u32 v)
{
	const u32 result = v == 0 ? 1 : (u32)tgm_f32_floor(tgm_ui32_log10(v)) + 1;
	return result;
}

u32 tgm_ui32_floor(u32 v)
{
#ifdef _M_X64
	const __m128 simd_v = _mm_set_ss((f32)v);
	const __m128 simd_result = _mm_floor_ps(simd_v);
	const u32 result = (u32)simd_result.m128_f32[0];
#else
	const ui32 result = (ui32)floor((f64)v);
#endif
	return result;
}

f32 tgm_ui32_log10(u32 v)
{
#ifdef _M_X64
	const __m128 simd_v = _mm_set_ss((f32)v);
	const __m128 simd_result = _mm_log10_ps(simd_v);
	const f32 result = simd_result.m128_f32[0];
#else
	const f32 result = log10f((f32)v);
#endif
	return result;
}

u32 tgm_ui32_pow(u32 base, u32 exponent)
{
#ifdef _M_X64
	const __m128 simd_base = _mm_set_ss((f32)base);
	const __m128 simd_exponent = _mm_set_ss((f32)exponent);
	const __m128 simd_result = _mm_pow_ps(simd_base, simd_exponent);
	const u32 result = (u32)simd_result.m128_f32[0];
#else
	const ui32 result = (ui32)pow((f64)base, (f64)exponent);
#endif
	return result;
}



/*------------------------------------------------------------+
| Functional                                                  |
+------------------------------------------------------------*/

f32 tgm_f32_clamp(f32 v, f32 low, f32 high)
{
	const f32 result = tgm_f32_max(low, tgm_f32_min(high, v));
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

u32 tgm_ui32_clamp(u32 v, u32 low, u32 high)
{
	const u32 result = tgm_ui32_max(low, tgm_ui32_min(high, v));
	return result;
}

u32 tgm_ui32_max(u32 v0, u32 v1)
{
	const u32 result = v0 > v1 ? v0 : v1;
	return result;
}

u32 tgm_ui32_min(u32 v0, u32 v1)
{
	const u32 result = v0 < v1 ? v0 : v1;
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

	return p_v0->x * p_v1->x + p_v0->y * p_v1->y + p_v0->z * p_v1->z;
}

b32 tgm_v3_equal(const v3* p_v0, const v3* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	return p_v0 == p_v1 || memcmp(p_v0, p_v1, sizeof(v3)) == 0;
}

f32 tgm_v3_magnitude(const v3* p_v0)
{
	TG_ASSERT(p_v0);

	return tgm_f32_sqrt(p_v0->x * p_v0->x + p_v0->y * p_v0->y + p_v0->z * p_v0->z);
}

f32 tgm_v3_magnitude_squared(const v3* p_v)
{
	TG_ASSERT(p_v);

	return p_v->x * p_v->x + p_v->y * p_v->y + p_v->z * p_v->z;
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

f32 tgm_m4_det(const m4* p_m)
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

	result.m00 = det *  (p_m->m11 * m2323 - p_m->m12 * m1323 + p_m->m13 * m1223);
	result.m01 = det * -(p_m->m01 * m2323 - p_m->m02 * m1323 + p_m->m03 * m1223);
	result.m02 = det *  (p_m->m01 * m2313 - p_m->m02 * m1313 + p_m->m03 * m1213);
	result.m03 = det * -(p_m->m01 * m2312 - p_m->m02 * m1312 + p_m->m03 * m1212);
	result.m10 = det * -(p_m->m10 * m2323 - p_m->m12 * m0323 + p_m->m13 * m0223);
	result.m11 = det *  (p_m->m00 * m2323 - p_m->m02 * m0323 + p_m->m03 * m0223);
	result.m12 = det * -(p_m->m00 * m2313 - p_m->m02 * m0313 + p_m->m03 * m0213);
	result.m13 = det *  (p_m->m00 * m2312 - p_m->m02 * m0312 + p_m->m03 * m0212);
	result.m20 = det *  (p_m->m10 * m1323 - p_m->m11 * m0323 + p_m->m13 * m0123);
	result.m21 = det * -(p_m->m00 * m1323 - p_m->m01 * m0323 + p_m->m03 * m0123);
	result.m22 = det *  (p_m->m00 * m1313 - p_m->m01 * m0313 + p_m->m03 * m0113);
	result.m23 = det * -(p_m->m00 * m1312 - p_m->m01 * m0312 + p_m->m03 * m0112);
	result.m30 = det * -(p_m->m10 * m1223 - p_m->m11 * m0223 + p_m->m12 * m0123);
	result.m31 = det *  (p_m->m00 * m1223 - p_m->m01 * m0223 + p_m->m02 * m0123);
	result.m32 = det * -(p_m->m00 * m1213 - p_m->m01 * m0213 + p_m->m02 * m0113);
	result.m33 = det *  (p_m->m00 * m1212 - p_m->m01 * m0212 + p_m->m02 * m0112);

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
