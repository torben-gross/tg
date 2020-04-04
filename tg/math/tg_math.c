#include "tg_math.h"

#include <math.h>
#include <string.h>



f32 tgm_f32_log2(f32 v)
{
	return logf(v);
}



f64 tgm_f64_pow(f64 base, f64 exponent)
{
	return pow(base, exponent);
}



i32 tgm_i32_abs(i32 v)
{
	TG_ASSERT(v != I32_MIN);

	return v >= 0 ? v : -v;
}

ui32 tgm_i32_digits(i32 v)
{
	if (v == I32_MIN)
	{
		v += 1;
	}
	return v == 0 ? 1 : tgm_i32_floor(tgm_i32_log10(tgm_i32_abs(v))) + 1;
}

tgm_i32_floor(i32 v)
{
	return (i32)floor((f64)v);
}

i32 tgm_i32_log10(i32 v)
{
	return (i32)log10((f64)v);
}

i32 tgm_i32_pow(i32 base, i32 exponent)
{
	return (i32)pow((f64)base, (f64)exponent);
}



ui32 tgm_ui32_digits(ui32 v)
{
	return v == 0 ? 1 : tgm_ui32_floor(tgm_ui32_log10(v)) + 1;
}

ui32 tgm_ui32_floor(ui32 v)
{
	return (ui32)floor((f64)v);
}

ui32 tgm_ui32_log10(ui32 v)
{
	return (ui32)log10((f64)v);
}

ui32 tgm_ui32_pow(ui32 base, ui32 exponent)
{
	return (ui32)pow((f64)base, (f64)exponent);
}



f32 tgm_f32_clamp(f32 v, f32 low, f32 high)
{
	return tgm_f32_max(low, tgm_f32_min(high, v));
}

f32 tgm_f32_max(f32 v0, f32 v1)
{
	return v0 > v1 ? v0 : v1;
}

f32 tgm_f32_min(f32 v0, f32 v1)
{
	return v0 < v1 ? v0 : v1;
}

i32 tgm_i32_clamp(i32 v, i32 low, i32 high)
{
	return tgm_i32_max(low, tgm_i32_min(high, v));
}

i32 tgm_i32_max(i32 v0, i32 v1)
{
	return v0 > v1 ? v0 : v1;
}

i32 tgm_i32_min(i32 v0, i32 v1)
{
	return v0 < v1 ? v0 : v1;
}

ui32 tgm_ui32_clamp(ui32 v, ui32 low, ui32 high)
{
	return tgm_ui32_max(low, tgm_ui32_min(high, v));
}

ui32 tgm_ui32_max(ui32 v0, ui32 v1)
{
	return v0 > v1 ? v0 : v1;
}

ui32 tgm_ui32_min(ui32 v0, ui32 v1)
{
	return v0 < v1 ? v0 : v1;
}



tgm_vec2f* tgm_v2f_subtract_v2f(tgm_vec2f* p_result, tgm_vec2f* p_v0, tgm_vec2f* p_v1)
{
	TG_ASSERT(p_result && p_v0 && p_v1);

	p_result->x = p_v0->x - p_v1->x;
	p_result->y = p_v0->y - p_v1->y;

	return p_result;
}



tgm_vec3f* tgm_v3f_add_v3f(tgm_vec3f* p_result, tgm_vec3f* p_v0, tgm_vec3f* p_v1)
{
	TG_ASSERT(p_result && p_v0 && p_v1);

	p_result->x = p_v0->x + p_v1->x;
	p_result->y = p_v0->y + p_v1->y;
	p_result->z = p_v0->z + p_v1->z;

	return p_result;
}

tgm_vec3f* tgm_v3f_add_f(tgm_vec3f* p_result, tgm_vec3f* p_v, f32 f)
{
	TG_ASSERT(p_result && p_v);

	p_result->x = p_v->x + f;
	p_result->y = p_v->y + f;
	p_result->z = p_v->z + f;

	return p_result;
}

tgm_vec3f* tgm_v3f_cross(tgm_vec3f* p_result, tgm_vec3f* p_v0, tgm_vec3f* p_v1)
{
	TG_ASSERT(p_result && p_v0 && p_v1);

	tgm_vec3f result = { 0 };

	result.x = p_v0->y * p_v1->z - p_v0->z * p_v1->y;
	result.y = p_v0->z * p_v1->x - p_v0->x * p_v1->z;
	result.z = p_v0->x * p_v1->y - p_v0->y * p_v1->x;

	*p_result = result;
	return p_result;
}

tgm_vec3f* tgm_v3f_divide_v3f(tgm_vec3f* p_result, tgm_vec3f* p_v0, tgm_vec3f* p_v1)
{
	TG_ASSERT(p_result && p_v0 && p_v1 && p_v1->x && p_v1->y && p_v1->z);

	p_result->x = p_v0->x / p_v1->x;
	p_result->y = p_v0->y / p_v1->y;
	p_result->z = p_v0->z / p_v1->z;

	return p_result;
}

tgm_vec3f* tgm_v3f_divide_f(tgm_vec3f* p_result, tgm_vec3f* p_v, f32 f)
{
	TG_ASSERT(p_result && p_v && f);

	p_result->x = p_v->x / f;
	p_result->y = p_v->y / f;
	p_result->z = p_v->z / f;

	return p_result;
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

	return sqrtf(p_v0->x * p_v0->x + p_v0->y * p_v0->y + p_v0->z * p_v0->z);
}

f32 tgm_v3f_magnitude_squared(const tgm_vec3f* p_v)
{
	TG_ASSERT(p_v);

	return p_v->x * p_v->x + p_v->y * p_v->y + p_v->z * p_v->z;
}

tgm_vec3f* tgm_v3f_multiply_v3f(tgm_vec3f* p_result, tgm_vec3f* p_v0, tgm_vec3f* p_v1)
{
	TG_ASSERT(p_result && p_v0 && p_v1);

	p_result->x = p_v0->x * p_v1->x;
	p_result->y = p_v0->y * p_v1->y;
	p_result->z = p_v0->z * p_v1->z;

	return p_result;
}

tgm_vec3f* tgm_v3f_multiply_f(tgm_vec3f* p_result, tgm_vec3f* p_v, f32 f)
{
	TG_ASSERT(p_result && p_v);

	p_result->x = p_v->x * f;
	p_result->y = p_v->y * f;
	p_result->z = p_v->z * f;

	return p_result;
}

tgm_vec3f* tgm_v3f_negate(tgm_vec3f* p_result, tgm_vec3f* p_v)
{
	TG_ASSERT(p_result && p_v);

	p_result->x = -p_v->x;
	p_result->y = -p_v->y;
	p_result->z = -p_v->z;

	return p_result;
}

tgm_vec3f* tgm_v3f_normalize(tgm_vec3f* p_result, tgm_vec3f* p_v)
{
	TG_ASSERT(p_result && p_v);

	const f32 magnitude = tgm_v3f_magnitude(p_v);
	TG_ASSERT(magnitude);

	p_result->x = p_v->x / magnitude;
	p_result->y = p_v->y / magnitude;
	p_result->z = p_v->z / magnitude;

	return p_result;
}

tgm_vec3f* tgm_v3f_subtract_v3f(tgm_vec3f* p_result, tgm_vec3f* p_v0, tgm_vec3f* p_v1)
{
	TG_ASSERT(p_result && p_v0 && p_v1);

	p_result->x = p_v0->x - p_v1->x;
	p_result->y = p_v0->y - p_v1->y;
	p_result->z = p_v0->z - p_v1->z;

	return p_result;
}

tgm_vec3f* tgm_v3f_subtract_f(tgm_vec3f* p_result, tgm_vec3f* p_v, f32 f)
{
	TG_ASSERT(p_result && p_v);

	p_result->x = p_v->x - f;
	p_result->y = p_v->y - f;
	p_result->z = p_v->z - f;

	return p_result;
}

tgm_vec4f* tgm_v3f_to_v4f(tgm_vec4f* p_result, const tgm_vec3f* p_v, f32 w)
{
	TG_ASSERT(p_result && p_v);

	p_result->x = p_v->x;
	p_result->y = p_v->y;
	p_result->z = p_v->z;
	p_result->w = w;

	return p_result;
}



tgm_vec4f* tgm_v4f_negate(tgm_vec4f* p_result, tgm_vec4f* p_v)
{
	TG_ASSERT(p_result && p_v);

	p_result->x = -p_v->x;
	p_result->y = -p_v->y;
	p_result->z = -p_v->z;

	return p_result;
}

tgm_vec3f* tgm_v4f_to_v3f(tgm_vec3f* p_result, const tgm_vec4f* p_v)
{
	TG_ASSERT(p_result && p_v);

	if (p_v->w == 0.0f)
	{
		p_result->x = p_v->x;
		p_result->y = p_v->y;
		p_result->z = p_v->z;
	}
	else
	{
		p_result->x = p_v->x / p_v->w;
		p_result->y = p_v->y / p_v->w;
		p_result->z = p_v->z / p_v->w;
	}
	return p_result;
}



tgm_mat2f* tgm_m2f_identity(tgm_mat2f* p_result)
{
	TG_ASSERT(p_result);

	p_result->m00 = 1.0f;
	p_result->m10 = 0.0f;

	p_result->m01 = 0.0f;
	p_result->m11 = 1.0f;

	return p_result;
}

tgm_mat2f* tgm_m2f_multiply_m2f(tgm_mat2f* p_result, tgm_mat2f* p_m0, tgm_mat2f* p_m1)
{
	TG_ASSERT(p_result && p_m0 && p_m1);

	tgm_mat2f result = { 0 };

	result.m00 = p_m0->m00 * p_m1->m00 + p_m0->m01 * p_m1->m10;
	result.m10 = p_m0->m10 * p_m1->m00 + p_m0->m11 * p_m1->m10;

	result.m01 = p_m0->m00 * p_m1->m01 + p_m0->m01 * p_m1->m11;
	result.m11 = p_m0->m10 * p_m1->m01 + p_m0->m11 * p_m1->m11;

	*p_result = result;
	return p_result;
}

tgm_vec2f* tgm_m2f_multiply_v2f(tgm_vec2f* p_result, const tgm_mat2f* p_m, tgm_vec2f* p_v)
{
	TG_ASSERT(p_result && p_m && p_v);

	tgm_vec2f result = { 0 };

	result.x = p_v->x * p_m->m00 + p_v->y * p_m->m01;
	result.y = p_v->x * p_m->m10 + p_v->y * p_m->m11;

	*p_result = result;
	return p_result;
}

tgm_mat2f* tgm_m2f_transpose(tgm_mat2f* p_result, tgm_mat2f* p_m)
{
	TG_ASSERT(p_result && p_m);

	tgm_mat2f result = { 0 };

	result.m00 = p_m->m00;
	result.m10 = p_m->m01;
	result.m01 = p_m->m10;
	result.m11 = p_m->m11;

	*p_result = result;
	return p_result;
}



tgm_mat3f* tgm_m3f_identity(tgm_mat3f* p_result)
{
	TG_ASSERT(p_result);

	p_result->m00 = 1.0f;
	p_result->m10 = 0.0f;
	p_result->m20 = 0.0f;

	p_result->m01 = 0.0f;
	p_result->m11 = 1.0f;
	p_result->m21 = 0.0f;

	p_result->m02 = 0.0f;
	p_result->m12 = 0.0f;
	p_result->m22 = 1.0f;

	return p_result;
}

tgm_mat3f* tgm_m3f_multiply_m3f(tgm_mat3f* p_result, tgm_mat3f* p_m0, tgm_mat3f* p_m1)
{
	TG_ASSERT(p_result && p_m0 && p_m1);

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

	*p_result = result;
	return p_result;
}

tgm_vec3f* tgm_m3f_multiply_v3f(tgm_vec3f* p_result, const tgm_mat3f* p_m, tgm_vec3f* p_v)
{
	TG_ASSERT(p_result && p_m && p_v);

	tgm_vec3f result = { 0 };

	result.x = p_v->x * p_m->m00 + p_v->y * p_m->m01 + p_v->z * p_m->m02;
	result.y = p_v->x * p_m->m10 + p_v->y * p_m->m11 + p_v->z * p_m->m12;
	result.z = p_v->x * p_m->m20 + p_v->y * p_m->m21 + p_v->z * p_m->m22;

	*p_result = result;
	return p_result;
}

tgm_mat3f* tgm_m3f_orthographic(tgm_mat3f* p_result, f32 left, f32 right, f32 bottom, f32 top)
{
	TG_ASSERT(p_result && right != left && top != bottom);

	p_result->m00 = 2.0f / (right - left);
	p_result->m10 = 0.0f;
	p_result->m20 = 0.0f;

	p_result->m01 = 0.0f;
	p_result->m11 = -2.0f / (top - bottom);
	p_result->m21 = 0.0f;

	p_result->m02 = -(right + left) / (right - left);
	p_result->m12 = -(bottom + top) / (bottom - top);
	p_result->m22 = 1.0f;

	return p_result;
}

tgm_mat3f* tgm_m3f_transpose(tgm_mat3f* p_result, tgm_mat3f* p_m)
{
	TG_ASSERT(p_result && p_m);

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

	*p_result = result;
	return p_result;
}



tgm_mat4f* tgm_m4f_angle_axis(tgm_mat4f* p_result, f32 angle_in_radians, const tgm_vec3f* p_axis)
{
	TG_ASSERT(p_result && p_axis);

	const f32 c = (f32)cos((f64)angle_in_radians);
	const f32 s = (f32)sin((f64)angle_in_radians);
	const f32 omc = 1.0f - c;
	const f32 l = tgm_v3f_magnitude(p_axis);
	TG_ASSERT(l);
	const f32 x = p_axis->x / l;
	const f32 y = p_axis->y / l;
	const f32 z = p_axis->z / l;

	p_result->m00 = x * x * omc + c;
	p_result->m10 = y * x * omc + z * s;
	p_result->m20 = z * x * omc - y * s;
	p_result->m30 = 0.0f;

	p_result->m01 = x * y * omc - z * s;
	p_result->m11 = y * y * omc + c;
	p_result->m21 = z * y * omc + x * s;
	p_result->m31 = 0.0f;

	p_result->m02 = x * z * omc + y * s;
	p_result->m12 = y * z * omc - x * s;
	p_result->m22 = z * z * omc + c;
	p_result->m32 = 0.0f;

	p_result->m03 = 0.0f;
	p_result->m13 = 0.0f;
	p_result->m23 = 0.0f;
	p_result->m33 = 1.0f;
	
	return p_result;
}

tgm_mat4f* tgm_m4f_euler(tgm_mat4f* p_result, f32 pitch_in_radians, f32 yaw_in_radians, f32 roll_in_radians)
{
	TG_ASSERT(p_result);

	tgm_mat4f x = *tgm_m4f_rotate_x(&x, pitch_in_radians);
	tgm_mat4f y = *tgm_m4f_rotate_y(&y, yaw_in_radians);
	tgm_mat4f z = *tgm_m4f_rotate_z(&z, roll_in_radians);

	return tgm_m4f_multiply_m4f(p_result, &x, tgm_m4f_multiply_m4f(p_result, &y, &z));
}

tgm_mat4f* tgm_m4f_identity(tgm_mat4f* p_result)
{
	TG_ASSERT(p_result);

	p_result->m00 = 1.0f;
	p_result->m10 = 0.0f;
	p_result->m20 = 0.0f;
	p_result->m30 = 0.0f;

	p_result->m01 = 0.0f;
	p_result->m11 = 1.0f;
	p_result->m21 = 0.0f;
	p_result->m31 = 0.0f;

	p_result->m02 = 0.0f;
	p_result->m12 = 0.0f;
	p_result->m22 = 1.0f;
	p_result->m32 = 0.0f;
	
	p_result->m03 = 0.0f;
	p_result->m13 = 0.0f;
	p_result->m23 = 0.0f;
	p_result->m33 = 1.0f;

	return p_result;
}

tgm_mat4f* tgm_m4f_look_at(tgm_mat4f* p_result, tgm_vec3f* p_from, tgm_vec3f* p_to, tgm_vec3f* p_up)
{
	TG_ASSERT(p_result && p_from && p_to && p_up && !tgm_v3f_equal(p_from, p_to) && !tgm_v3f_equal(p_from, p_up) && !tgm_v3f_equal(p_to, p_up));

	tgm_vec3f f = *tgm_v3f_normalize(&f, tgm_v3f_subtract_v3f(&f, p_to, p_from));
	tgm_vec3f r = *tgm_v3f_normalize(&r, tgm_v3f_cross(&r, &f, tgm_v3f_normalize(p_up, p_up)));
	tgm_vec3f u = *tgm_v3f_normalize(&u, tgm_v3f_cross(&u, &r, &f));
	tgm_v3f_negate(&f, &f);
	tgm_vec3f from_negated = *tgm_v3f_negate(&from_negated, p_from);
	
	p_result->m00 = r.x;
	p_result->m10 = u.x;
	p_result->m20 = f.x;
	p_result->m30 = 0.0f;

	p_result->m01 = r.y;
	p_result->m11 = u.y;
	p_result->m21 = f.y;
	p_result->m31 = 0.0f;

	p_result->m02 = r.z;
	p_result->m12 = u.z;
	p_result->m22 = f.z;
	p_result->m32 = 0.0f;

	p_result->m03 = tgm_v3f_dot(&r, &from_negated);
	p_result->m13 = tgm_v3f_dot(&u, &from_negated);
	p_result->m23 = tgm_v3f_dot(&f, &from_negated);
	p_result->m33 = 1.0f;

	return p_result;
}

tgm_mat4f* tgm_m4f_multiply_m4f(tgm_mat4f* p_result, tgm_mat4f* p_m0, tgm_mat4f* p_m1)
{
	TG_ASSERT(p_result && p_m0 && p_m1);

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

	*p_result = result;
	return p_result;
}

tgm_vec4f* tgm_m4f_multiply_v4f(tgm_vec4f* p_result, const tgm_mat4f* p_m, tgm_vec4f* p_v)
{
	TG_ASSERT(p_result && p_m && p_v);

	tgm_vec4f result = { 0 };

	result.x = p_v->x * p_m->m00 + p_v->y * p_m->m01 + p_v->z * p_m->m02 + p_v->w * p_m->m03;
	result.y = p_v->x * p_m->m10 + p_v->y * p_m->m11 + p_v->z * p_m->m12 + p_v->w * p_m->m13;
	result.z = p_v->x * p_m->m20 + p_v->y * p_m->m21 + p_v->z * p_m->m22 + p_v->w * p_m->m23;
	result.w = p_v->x * p_m->m30 + p_v->y * p_m->m31 + p_v->z * p_m->m32 + p_v->w * p_m->m33;

	*p_result = result;
	return p_result;
}

// NOTE: vulkan uses left = -1.0, right = 1.0, bottom = 1.0, top = -1.0, near = 0.0 and far = 1.0
tgm_mat4f* tgm_m4f_orthographic(tgm_mat4f* p_result, f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near)
{
	TG_ASSERT(p_result && left != right && top != bottom && near != far);

	p_result->m00 = 2.0f / (right - left);
	p_result->m10 = 0.0f;
	p_result->m20 = 0.0f;
	p_result->m30 = 0.0f;

	p_result->m01 = 0.0f;
	p_result->m11 = -2.0f / (top - bottom);
	p_result->m21 = 0.0f;
	p_result->m31 = 0.0f;

	p_result->m02 = 0.0f;
	p_result->m12 = 0.0f;
	p_result->m22 = -1.0f / (near - far);
	p_result->m32 = 0.0f;

	p_result->m03 = -(right + left) / (right - left);
	p_result->m13 = -(bottom + top) / (bottom - top);
	p_result->m23 = near / (near - far);
	p_result->m33 = 1.0f;

	return p_result;
}

tgm_mat4f* tgm_m4f_perspective(tgm_mat4f* p_result, f32 fov_y_in_radians, f32 aspect, f32 near, f32 far)
{
	TG_ASSERT(p_result);

	const f32 tan_half_fov_y = (f32)tan((f64)fov_y_in_radians / 2.0f);

	const f32 a = -far / (far - near);
	const f32 b = -(2.0f * far * near) / (near - far);

	p_result->m00 = 1.0f / (aspect * tan_half_fov_y);
	p_result->m10 = 0.0f;
	p_result->m20 = 0.0f;
	p_result->m30 = 0.0f;

	p_result->m01 = 0.0f;
	p_result->m11 = -1.0f / tan_half_fov_y;
	p_result->m21 = 0.0f;
	p_result->m31 = 0.0f;

	p_result->m02 = 0.0f;
	p_result->m12 = 0.0f;
	p_result->m22 = a;
	p_result->m32 = -1.0f;

	p_result->m03 = 0.0f;
	p_result->m13 = 0.0f;
	p_result->m23 = b;
	p_result->m33 = 0.0f;

	return p_result;
}

tgm_mat4f* tgm_m4f_rotate_x(tgm_mat4f* p_result, f32 angle_in_radians)
{
	TG_ASSERT(p_result);

	p_result->m00 = 1.0f;
	p_result->m10 = 0.0f;
	p_result->m20 = 0.0f;
	p_result->m30 = 0.0f;

	p_result->m01 = 0.0f;
	p_result->m11 = (f32)cos((f64)angle_in_radians);
	p_result->m21 = (f32)sin((f64)angle_in_radians);
	p_result->m31 = 0.0f;

	p_result->m02 = 0.0f;
	p_result->m12 = (f32)-sin((f64)angle_in_radians);
	p_result->m22 = (f32)cos((f64)angle_in_radians);
	p_result->m32 = 0.0f;

	p_result->m03 = 0.0f;
	p_result->m13 = 0.0f;
	p_result->m23 = 0.0f;
	p_result->m33 = 1.0f;

	return p_result;
}

tgm_mat4f* tgm_m4f_rotate_y(tgm_mat4f* p_result, f32 angle_in_radians)
{
	TG_ASSERT(p_result);

	p_result->m00 = (f32)cos((f64)angle_in_radians);
	p_result->m10 = 0.0f;
	p_result->m20 = (f32)-sin((f64)angle_in_radians);
	p_result->m30 = 0.0f;

	p_result->m01 = 0.0f;
	p_result->m11 = 1.0f;
	p_result->m21 = 0.0f;
	p_result->m31 = 0.0f;

	p_result->m02 = (f32)sin((f64)angle_in_radians);
	p_result->m12 = 0.0f;
	p_result->m22 = (f32)cos((f64)angle_in_radians);
	p_result->m32 = 0.0f;

	p_result->m03 = 0.0f;
	p_result->m13 = 0.0f;
	p_result->m23 = 0.0f;
	p_result->m33 = 1.0f;

	return p_result;
}

tgm_mat4f* tgm_m4f_rotate_z(tgm_mat4f* p_result, f32 angle_in_radians)
{
	TG_ASSERT(p_result);

	p_result->m00 = (f32)cos((f64)angle_in_radians);
	p_result->m10 = (f32)sin((f64)angle_in_radians);
	p_result->m20 = 0.0f;
	p_result->m30 = 0.0f;

	p_result->m01 = (f32)-sin((f64)angle_in_radians);
	p_result->m11 = (f32)cos((f64)angle_in_radians);
	p_result->m21 = 0.0f;
	p_result->m31 = 0.0f;

	p_result->m02 = 0.0f;
	p_result->m12 = 0.0f;
	p_result->m22 = 1.0f;
	p_result->m32 = 0.0f;

	p_result->m03 = 0.0f;
	p_result->m13 = 0.0f;
	p_result->m23 = 0.0f;
	p_result->m33 = 1.0f;

	return p_result;
}

tgm_mat4f* tgm_m4f_translate(tgm_mat4f* p_result, const tgm_vec3f* p_v)
{
	TG_ASSERT(p_result && p_v);

	p_result->m00 = 1.0f;
	p_result->m10 = 0.0f;
	p_result->m20 = 0.0f;
	p_result->m30 = 0.0f;

	p_result->m01 = 0.0f;
	p_result->m11 = 1.0f;
	p_result->m21 = 0.0f;
	p_result->m31 = 0.0f;

	p_result->m02 = 0.0f;
	p_result->m12 = 0.0f;
	p_result->m22 = 1.0f;
	p_result->m32 = 0.0f;

	p_result->m03 = p_v->x;
	p_result->m13 = p_v->y;
	p_result->m23 = p_v->z;
	p_result->m33 = 1.0f;

	return p_result;
}

tgm_mat4f* tgm_m4f_transpose(tgm_mat4f* p_result, tgm_mat4f* p_m)
{
	TG_ASSERT(p_result && p_m);

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

	*p_result = result;
	return p_result;
}
