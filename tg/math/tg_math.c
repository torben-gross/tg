#include "tg/math/tg_math.h"

#ifdef _M_X64
#include <immintrin.h>
#else
#include <math.h>
#endif

#include <string.h>



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
	TG_ASSERT(v != I32_MIN);

	const i32 result = v >= 0 ? v : -v;
	return result;
}

ui32 tgm_i32_digits(i32 v)
{
	if (v == I32_MIN)
	{
		v += 1;
	}
	const ui32 result = v == 0 ? 1 : (ui32)tgm_f32_floor(tgm_ui32_log10((ui32)tgm_i32_abs(v))) + 1;
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



ui32 tgm_ui32_digits(ui32 v)
{
	const ui32 result = v == 0 ? 1 : (ui32)tgm_f32_floor(tgm_ui32_log10(v)) + 1;
	return result;
}

ui32 tgm_ui32_floor(ui32 v)
{
#ifdef _M_X64
	const __m128 simd_v = _mm_set_ss((f32)v);
	const __m128 simd_result = _mm_floor_ps(simd_v);
	const ui32 result = (ui32)simd_result.m128_f32[0];
#else
	const ui32 result = (ui32)floor((f64)v);
#endif
	return result;
}

f32 tgm_ui32_log10(ui32 v)
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

ui32 tgm_ui32_pow(ui32 base, ui32 exponent)
{
#ifdef _M_X64
	const __m128 simd_base = _mm_set_ss((f32)base);
	const __m128 simd_exponent = _mm_set_ss((f32)exponent);
	const __m128 simd_result = _mm_pow_ps(simd_base, simd_exponent);
	const ui32 result = (ui32)simd_result.m128_f32[0];
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

ui32 tgm_ui32_clamp(ui32 v, ui32 low, ui32 high)
{
	const ui32 result = tgm_ui32_max(low, tgm_ui32_min(high, v));
	return result;
}

ui32 tgm_ui32_max(ui32 v0, ui32 v1)
{
	const ui32 result = v0 > v1 ? v0 : v1;
	return result;
}

ui32 tgm_ui32_min(ui32 v0, ui32 v1)
{
	const ui32 result = v0 < v1 ? v0 : v1;
	return result;
}



/*------------------------------------------------------------+
| Vectors                                                     |
+------------------------------------------------------------*/

tgm_vec2f tgm_v2f_subtract_v2f(const tgm_vec2f* p_v0, const tgm_vec2f* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	tgm_vec2f result = { 0 };

	result.x = p_v0->x - p_v1->x;
	result.y = p_v0->y - p_v1->y;

	return result;
}



tgm_vec3f tgm_v3f_add_v3f(const tgm_vec3f* p_v0, const tgm_vec3f* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	tgm_vec3f result = { 0 };

	result.x = p_v0->x + p_v1->x;
	result.y = p_v0->y + p_v1->y;
	result.z = p_v0->z + p_v1->z;

	return result;
}

tgm_vec3f tgm_v3f_add_f(const tgm_vec3f* p_v, f32 f)
{
	TG_ASSERT(p_v);

	tgm_vec3f result = { 0 };

	result.x = p_v->x + f;
	result.y = p_v->y + f;
	result.z = p_v->z + f;

	return result;
}

tgm_vec3f tgm_v3f_cross(const tgm_vec3f* p_v0, const tgm_vec3f* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	tgm_vec3f result = { 0 };

	result.x = p_v0->y * p_v1->z - p_v0->z * p_v1->y;
	result.y = p_v0->z * p_v1->x - p_v0->x * p_v1->z;
	result.z = p_v0->x * p_v1->y - p_v0->y * p_v1->x;

	return result;
}

tgm_vec3f tgm_v3f_divide_v3f(const tgm_vec3f* p_v0, const tgm_vec3f* p_v1)
{
	TG_ASSERT(p_v0 && p_v1 && p_v1->x && p_v1->y && p_v1->z);

	tgm_vec3f result = { 0 };

	result.x = p_v0->x / p_v1->x;
	result.y = p_v0->y / p_v1->y;
	result.z = p_v0->z / p_v1->z;

	return result;
}

tgm_vec3f tgm_v3f_divide_f(const tgm_vec3f* p_v, f32 f)
{
	TG_ASSERT(p_v && f);

	tgm_vec3f result = { 0 };

	result.x = p_v->x / f;
	result.y = p_v->y / f;
	result.z = p_v->z / f;

	return result;
}

f32 tgm_v3f_dot(const tgm_vec3f* p_v0, const tgm_vec3f* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	return p_v0->x * p_v1->x + p_v0->y * p_v1->y + p_v0->z * p_v1->z;
}

b32 tgm_v3f_equal(const tgm_vec3f* p_v0, const tgm_vec3f* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	return p_v0 == p_v1 || memcmp(p_v0, p_v1, sizeof(tgm_vec3f)) == 0;
}

f32 tgm_v3f_magnitude(const tgm_vec3f* p_v0)
{
	TG_ASSERT(p_v0);

	return tgm_f32_sqrt(p_v0->x * p_v0->x + p_v0->y * p_v0->y + p_v0->z * p_v0->z);
}

f32 tgm_v3f_magnitude_squared(const tgm_vec3f* p_v)
{
	TG_ASSERT(p_v);

	return p_v->x * p_v->x + p_v->y * p_v->y + p_v->z * p_v->z;
}

tgm_vec3f tgm_v3f_multiply_v3f(const tgm_vec3f* p_v0, const tgm_vec3f* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	tgm_vec3f result = { 0 };

	result.x = p_v0->x * p_v1->x;
	result.y = p_v0->y * p_v1->y;
	result.z = p_v0->z * p_v1->z;

	return result;
}

tgm_vec3f tgm_v3f_multiply_f(const tgm_vec3f* p_v, f32 f)
{
	TG_ASSERT(p_v);

	tgm_vec3f result = { 0 };

	result.x = p_v->x * f;
	result.y = p_v->y * f;
	result.z = p_v->z * f;

	return result;
}

tgm_vec3f tgm_v3f_negated(const tgm_vec3f* p_v)
{
	TG_ASSERT(p_v);

	tgm_vec3f result = { 0 };

	result.x = -p_v->x;
	result.y = -p_v->y;
	result.z = -p_v->z;

	return result;
}

tgm_vec3f tgm_v3f_normalized(const tgm_vec3f* p_v)
{
	TG_ASSERT(p_v);

	tgm_vec3f result = { 0 };

	const f32 magnitude = tgm_v3f_magnitude(p_v);
	TG_ASSERT(magnitude);

	result.x = p_v->x / magnitude;
	result.y = p_v->y / magnitude;
	result.z = p_v->z / magnitude;

	return result;
}

tgm_vec3f tgm_v3f_subtract_v3f(const tgm_vec3f* p_v0, const tgm_vec3f* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	tgm_vec3f result = { 0 };

	result.x = p_v0->x - p_v1->x;
	result.y = p_v0->y - p_v1->y;
	result.z = p_v0->z - p_v1->z;

	return result;
}

tgm_vec3f tgm_v3f_subtract_f(const tgm_vec3f* p_v, f32 f)
{
	TG_ASSERT(p_v);

	tgm_vec3f result = { 0 };

	result.x = p_v->x - f;
	result.y = p_v->y - f;
	result.z = p_v->z - f;

	return result;
}

tgm_vec4f tgm_v3f_to_v4f(const tgm_vec3f* p_v, f32 w)
{
	TG_ASSERT(p_v);

	tgm_vec4f result = { 0 };

	result.x = p_v->x;
	result.y = p_v->y;
	result.z = p_v->z;
	result.w = w;

	return result;
}



tgm_vec4f tgm_v4f_negated(const tgm_vec4f* p_v)
{
	TG_ASSERT(p_v);

	tgm_vec4f result = { 0 };

	result.x = -p_v->x;
	result.y = -p_v->y;
	result.z = -p_v->z;
	result.w = -p_v->w;

	return result;
}

tgm_vec3f tgm_v4f_to_v3f(const tgm_vec4f* p_v)
{
	TG_ASSERT(p_v);

	tgm_vec3f result = { 0 };

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

tgm_mat2f tgm_m2f_identity()
{
	tgm_mat2f result = { 0 };

	result.m00 = 1.0f;
	result.m10 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = 1.0f;

	return result;
}

tgm_mat2f tgm_m2f_multiply_m2f(const tgm_mat2f* p_m0, const tgm_mat2f* p_m1)
{
	TG_ASSERT(p_m0 && p_m1);

	tgm_mat2f result = { 0 };

	result.m00 = p_m0->m00 * p_m1->m00 + p_m0->m01 * p_m1->m10;
	result.m10 = p_m0->m10 * p_m1->m00 + p_m0->m11 * p_m1->m10;

	result.m01 = p_m0->m00 * p_m1->m01 + p_m0->m01 * p_m1->m11;
	result.m11 = p_m0->m10 * p_m1->m01 + p_m0->m11 * p_m1->m11;

	return result;
}

tgm_vec2f tgm_m2f_multiply_v2f(const tgm_mat2f* p_m, const tgm_vec2f* p_v)
{
	TG_ASSERT(p_m && p_v);

	tgm_vec2f result = { 0 };

	result.x = p_v->x * p_m->m00 + p_v->y * p_m->m01;
	result.y = p_v->x * p_m->m10 + p_v->y * p_m->m11;

	return result;
}

tgm_mat2f tgm_m2f_transposed(const tgm_mat2f* p_m)
{
	TG_ASSERT(p_m);

	tgm_mat2f result = { 0 };

	result.m00 = p_m->m00;
	result.m10 = p_m->m01;
	result.m01 = p_m->m10;
	result.m11 = p_m->m11;

	return result;
}



tgm_mat3f tgm_m3f_identity()
{
	tgm_mat3f result = { 0 };

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

tgm_mat3f tgm_m3f_multiply_m3f(const tgm_mat3f* p_m0, const tgm_mat3f* p_m1)
{
	TG_ASSERT(p_m0 && p_m1);

	tgm_mat3f result = { 0 };

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

tgm_vec3f tgm_m3f_multiply_v3f(const tgm_mat3f* p_m, const tgm_vec3f* p_v)
{
	TG_ASSERT(p_m && p_v);

	tgm_vec3f result = { 0 };

	result.x = p_v->x * p_m->m00 + p_v->y * p_m->m01 + p_v->z * p_m->m02;
	result.y = p_v->x * p_m->m10 + p_v->y * p_m->m11 + p_v->z * p_m->m12;
	result.z = p_v->x * p_m->m20 + p_v->y * p_m->m21 + p_v->z * p_m->m22;

	return result;
}

tgm_mat3f tgm_m3f_orthographic(f32 left, f32 right, f32 bottom, f32 top)
{
	TG_ASSERT(right != left && top != bottom);

	tgm_mat3f result = { 0 };

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

tgm_mat3f tgm_m3f_transposed(const tgm_mat3f* p_m)
{
	TG_ASSERT(p_m);

	tgm_mat3f result = { 0 };

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



tgm_mat4f tgm_m4f_angle_axis(f32 angle_in_radians, const tgm_vec3f* p_axis)
{
	TG_ASSERT(p_axis);

	tgm_mat4f result = { 0 };

	const f32 c = tgm_f32_cos(angle_in_radians);
	const f32 s = tgm_f32_sin(angle_in_radians);
	const f32 omc = 1.0f - c;
	const f32 l = tgm_v3f_magnitude(p_axis);
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

tgm_mat4f tgm_m4f_euler(f32 pitch_in_radians, f32 yaw_in_radians, f32 roll_in_radians)
{
	tgm_mat4f result = { 0 };

	tgm_mat4f x = tgm_m4f_rotate_x(pitch_in_radians);
	tgm_mat4f y = tgm_m4f_rotate_y(yaw_in_radians);
	tgm_mat4f z = tgm_m4f_rotate_z(roll_in_radians);

	result = tgm_m4f_multiply_m4f(&y, &z);
	result = tgm_m4f_multiply_m4f(&x, &result);

	return result;
}

tgm_mat4f tgm_m4f_identity()
{
	tgm_mat4f result = { 0 };

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

tgm_mat4f tgm_m4f_look_at(const tgm_vec3f* p_from, const tgm_vec3f* p_to, const tgm_vec3f* p_up)
{
	TG_ASSERT(p_from && p_to && p_up && !tgm_v3f_equal(p_from, p_to) && !tgm_v3f_equal(p_from, p_up) && !tgm_v3f_equal(p_to, p_up));

	tgm_mat4f result = { 0 };
	tgm_vec3f temp = { 0 };
	
	temp = tgm_v3f_subtract_v3f(p_to, p_from);
	const tgm_vec3f f_negated = tgm_v3f_normalized(&temp);

	temp = tgm_v3f_normalized(p_up);
	temp = tgm_v3f_cross(&f_negated, &temp);
	const tgm_vec3f r = tgm_v3f_normalized(&temp);

	temp = tgm_v3f_cross(&r, &f_negated);
	const tgm_vec3f u = tgm_v3f_normalized(&temp);

	const tgm_vec3f f = tgm_v3f_negated(&f_negated);

	tgm_vec3f from_negated = tgm_v3f_negated(p_from);
	
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

	result.m03 = tgm_v3f_dot(&r, &from_negated);
	result.m13 = tgm_v3f_dot(&u, &from_negated);
	result.m23 = tgm_v3f_dot(&f, &from_negated);
	result.m33 = 1.0f;

	return result;
}

tgm_mat4f tgm_m4f_multiply_m4f(const tgm_mat4f* p_m0, const tgm_mat4f* p_m1)
{
	TG_ASSERT(p_m0 && p_m1);

	tgm_mat4f result = { 0 };

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

tgm_vec4f tgm_m4f_multiply_v4f(const tgm_mat4f* p_m, const tgm_vec4f* p_v)
{
	TG_ASSERT(p_m && p_v);

	tgm_vec4f result = { 0 };

	result.x = p_v->x * p_m->m00 + p_v->y * p_m->m01 + p_v->z * p_m->m02 + p_v->w * p_m->m03;
	result.y = p_v->x * p_m->m10 + p_v->y * p_m->m11 + p_v->z * p_m->m12 + p_v->w * p_m->m13;
	result.z = p_v->x * p_m->m20 + p_v->y * p_m->m21 + p_v->z * p_m->m22 + p_v->w * p_m->m23;
	result.w = p_v->x * p_m->m30 + p_v->y * p_m->m31 + p_v->z * p_m->m32 + p_v->w * p_m->m33;

	return result;
}

tgm_mat4f tgm_m4f_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near)
{
	TG_ASSERT(left != right && top != bottom && near != far);

	tgm_mat4f result = { 0 };

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

tgm_mat4f tgm_m4f_perspective(f32 fov_y_in_radians, f32 aspect, f32 near, f32 far)
{
	tgm_mat4f result = { 0 };

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

tgm_mat4f tgm_m4f_rotate_x(f32 angle_in_radians)
{
	tgm_mat4f result = { 0 };

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

tgm_mat4f tgm_m4f_rotate_y(f32 angle_in_radians)
{
	tgm_mat4f result = { 0 };

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

tgm_mat4f tgm_m4f_rotate_z(f32 angle_in_radians)
{
	tgm_mat4f result = { 0 };

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

tgm_mat4f tgm_m4f_translate(const tgm_vec3f* p_v)
{
	TG_ASSERT(p_v);

	tgm_mat4f result = { 0 };

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

tgm_mat4f tgm_m4f_transposed(const tgm_mat4f* p_m)
{
	TG_ASSERT(p_m);

	tgm_mat4f result = { 0 };

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
