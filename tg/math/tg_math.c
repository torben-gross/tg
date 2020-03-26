#include "tg_math.h"

#include <math.h>

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



tgm_vec3f* tgm_v3f_add_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1)
{
	TG_ASSERT(result && v0 && v1);

	result->x = v0->x + v1->x;
	result->y = v0->y + v1->y;
	result->z = v0->z + v1->z;

	return result;
}

tgm_vec3f* tgm_v3f_add_f(tgm_vec3f* result, tgm_vec3f* v, f32 f)
{
	TG_ASSERT(result && v);

	result->x = v->x + f;
	result->y = v->y + f;
	result->z = v->z + f;

	return result;
}

tgm_vec3f* tgm_v3f_cross(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1)
{
	TG_ASSERT(result && v0 && v1);

	tgm_vec3f res = { 0 };

	res.x = v0->y * v1->z - v0->z * v1->y;
	res.y = v0->z * v1->x - v0->x * v1->z;
	res.z = v0->x * v1->y - v0->y * v1->x;

	*result = res;
	return result;
}

tgm_vec3f* tgm_v3f_divide_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1)
{
	TG_ASSERT(result && v0 && v1 && v1->x && v1->y && v1->z);

	result->x = v0->x / v1->x;
	result->y = v0->y / v1->y;
	result->z = v0->z / v1->z;

	return result;
}

tgm_vec3f* tgm_v3f_divide_f(tgm_vec3f* result, tgm_vec3f* v, f32 f)
{
	TG_ASSERT(result && v && f);

	result->x = v->x / f;
	result->y = v->y / f;
	result->z = v->z / f;

	return result;
}

f32 tgm_v3f_dot(const tgm_vec3f* v0, const tgm_vec3f* v1)
{
	TG_ASSERT(v0 && v1);

	return v0->x * v1->x + v0->y * v1->y + v0->z * v1->z;
}

bool tgm_v3f_equal(tgm_vec3f* v0, tgm_vec3f* v1)
{
	TG_ASSERT(v0 && v1);

	return v0 == v1 || v0->x == v1->x && v0->y == v1->y && v0->z == v1->z;
}

f32 tgm_v3f_magnitude(const tgm_vec3f* v0)
{
	TG_ASSERT(v0);

	return sqrtf(v0->x * v0->x + v0->y * v0->y + v0->z * v0->z);
}

f32 tgm_v3f_magnitude_squared(const tgm_vec3f* v)
{
	TG_ASSERT(v);

	return v->x * v->x + v->y * v->y + v->z * v->z;
}

tgm_vec3f* tgm_v3f_multiply_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1)
{
	TG_ASSERT(result && v0 && v1);

	result->x = v0->x * v1->x;
	result->y = v0->y * v1->y;
	result->z = v0->z * v1->z;

	return result;
}

tgm_vec3f* tgm_v3f_multiply_f(tgm_vec3f* result, tgm_vec3f* v, f32 f)
{
	TG_ASSERT(result && v);

	result->x = v->x * f;
	result->y = v->y * f;
	result->z = v->z * f;

	return result;
}

tgm_vec3f* tgm_v3f_negate(tgm_vec3f* result, tgm_vec3f* v)
{
	TG_ASSERT(result && v);

	result->x = -v->x;
	result->y = -v->y;
	result->z = -v->z;

	return result;
}

tgm_vec3f* tgm_v3f_normalize(tgm_vec3f* result, tgm_vec3f* v)
{
	TG_ASSERT(result && v);

	const f32 magnitude = tgm_v3f_magnitude(v);
	TG_ASSERT(magnitude);

	result->x = v->x / magnitude;
	result->y = v->y / magnitude;
	result->z = v->z / magnitude;

	return result;
}

tgm_vec3f* tgm_v3f_subtract_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1)
{
	TG_ASSERT(result && v0 && v1);

	result->x = v0->x - v1->x;
	result->y = v0->y - v1->y;
	result->z = v0->z - v1->z;

	return result;
}

tgm_vec3f* tgm_v3f_subtract_f(tgm_vec3f* result, tgm_vec3f* v, f32 f)
{
	TG_ASSERT(result && v);

	result->x = v->x - f;
	result->y = v->y - f;
	result->z = v->z - f;

	return result;
}

tgm_vec4f* tgm_v3f_to_v4f(tgm_vec4f* result, const tgm_vec3f* v, f32 w)
{
	TG_ASSERT(result && v);

	result->x = v->x;
	result->y = v->y;
	result->z = v->z;
	result->w = w;

	return result;
}



tgm_vec4f* tgm_v4f_negate(tgm_vec4f* result, tgm_vec4f* v)
{
	TG_ASSERT(result && v);

	result->x = -v->x;
	result->y = -v->y;
	result->z = -v->z;

	return result;
}

tgm_vec3f* tgm_v4f_to_v3f(tgm_vec3f* result, const tgm_vec4f* v)
{
	TG_ASSERT(result && v);

	if (v->w == 0.0f)
	{
		result->x = v->x;
		result->y = v->y;
		result->z = v->z;
	}
	else
	{
		result->x = v->x / v->w;
		result->y = v->y / v->w;
		result->z = v->z / v->w;
	}
	return result;
}



tgm_mat2f* tgm_m2f_identity(tgm_mat2f* result)
{
	TG_ASSERT(result);

	result->m00 = 1.0f;
	result->m10 = 0.0f;

	result->m01 = 0.0f;
	result->m11 = 1.0f;

	return result;
}

tgm_mat2f* tgm_m2f_multiply_m2f(tgm_mat2f* result, tgm_mat2f* m0, tgm_mat2f* m1)
{
	TG_ASSERT(result && m0 && m1);

	tgm_mat2f res = { 0 };

	res.m00 = m0->m00 * m1->m00 + m0->m01 * m1->m10;
	res.m10 = m0->m10 * m1->m00 + m0->m11 * m1->m10;

	res.m01 = m0->m00 * m1->m01 + m0->m01 * m1->m11;
	res.m11 = m0->m10 * m1->m01 + m0->m11 * m1->m11;

	*result = res;
	return result;
}

tgm_vec2f* tgm_m2f_multiply_v2f(tgm_vec2f* result, const tgm_mat2f* m, tgm_vec2f* v)
{
	TG_ASSERT(result && m && v);

	tgm_vec2f res = { 0 };

	res.x = v->x * m->m00 + v->y * m->m01;
	res.y = v->x * m->m10 + v->y * m->m11;

	*result = res;
	return result;
}

tgm_mat2f* tgm_m2f_transpose(tgm_mat2f* result, tgm_mat2f* m)
{
	TG_ASSERT(result && m);

	tgm_mat2f res = { 0 };

	res.m00 = m->m00;
	res.m10 = m->m01;
	res.m01 = m->m10;
	res.m11 = m->m11;

	*result = res;
	return result;
}



tgm_mat3f* tgm_m3f_identity(tgm_mat3f* result)
{
	TG_ASSERT(result);

	result->m00 = 1.0f;
	result->m10 = 0.0f;
	result->m20 = 0.0f;

	result->m01 = 0.0f;
	result->m11 = 1.0f;
	result->m21 = 0.0f;

	result->m02 = 0.0f;
	result->m12 = 0.0f;
	result->m22 = 1.0f;

	return result;
}

tgm_mat3f* tgm_m3f_multiply_m3f(tgm_mat3f* result, tgm_mat3f* m0, tgm_mat3f* m1)
{
	TG_ASSERT(result && m0 && m1);

	tgm_mat3f res = { 0 };

	res.m00 = m0->m00 * m1->m00 + m0->m01 * m1->m10 + m0->m02 * m1->m20;
	res.m10 = m0->m10 * m1->m00 + m0->m11 * m1->m10 + m0->m12 * m1->m20;
	res.m20 = m0->m20 * m1->m00 + m0->m21 * m1->m10 + m0->m22 * m1->m20;
	
	res.m01 = m0->m00 * m1->m01 + m0->m01 * m1->m11 + m0->m02 * m1->m21;
	res.m11 = m0->m10 * m1->m01 + m0->m11 * m1->m11 + m0->m12 * m1->m21;
	res.m21 = m0->m20 * m1->m01 + m0->m21 * m1->m11 + m0->m22 * m1->m21;
	
	res.m02 = m0->m00 * m1->m02 + m0->m01 * m1->m12 + m0->m02 * m1->m22;
	res.m12 = m0->m10 * m1->m02 + m0->m11 * m1->m12 + m0->m12 * m1->m22;
	res.m22 = m0->m20 * m1->m02 + m0->m21 * m1->m12 + m0->m22 * m1->m22;

	*result = res;
	return result;
}

tgm_vec3f* tgm_m3f_multiply_v3f(tgm_vec3f* result, const tgm_mat3f* m, tgm_vec3f* v)
{
	TG_ASSERT(result && m && v);

	tgm_vec3f res = { 0 };

	res.x = v->x * m->m00 + v->y * m->m01 + v->z * m->m02;
	res.y = v->x * m->m10 + v->y * m->m11 + v->z * m->m12;
	res.z = v->x * m->m20 + v->y * m->m21 + v->z * m->m22;

	*result = res;
	return result;
}

tgm_mat3f* tgm_m3f_orthographic(tgm_mat3f* result, f32 left, f32 right, f32 bottom, f32 top)
{
	TG_ASSERT(result && right != left && top != bottom);

	result->m00 = 2.0f / (right - left);
	result->m10 = 0.0f;
	result->m20 = 0.0f;

	result->m01 = 0.0f;
	result->m11 = -2.0f / (top - bottom);
	result->m21 = 0.0f;

	result->m02 = -(right + left) / (right - left);
	result->m12 = -(bottom + top) / (bottom - top);
	result->m22 = 1.0f;

	return result;
}

tgm_mat3f* tgm_m3f_transpose(tgm_mat3f* result, tgm_mat3f* m)
{
	TG_ASSERT(result && m);

	tgm_mat3f res = { 0 };

	res.m00 = m->m00;
	res.m10 = m->m01;
	res.m20 = m->m01;

	res.m01 = m->m10;
	res.m11 = m->m11;
	res.m21 = m->m12;

	res.m02 = m->m20;
	res.m12 = m->m21;
	res.m22 = m->m22;

	*result = res;
	return result;
}



tgm_mat4f* tgm_m4f_angle_axis(tgm_mat4f* result, f32 angle_in_radians, const tgm_vec3f* axis)
{
	TG_ASSERT(result && axis);

	const f32 c = (f32)cos((f64)angle_in_radians);
	const f32 s = (f32)sin((f64)angle_in_radians);
	const f32 omc = 1.0f - c;
	const f32 l = tgm_v3f_magnitude(axis);
	TG_ASSERT(l);
	const f32 x = axis->x / l;
	const f32 y = axis->y / l;
	const f32 z = axis->z / l;

	result->m00 = x * x * omc + c;
	result->m10 = y * x * omc + z * s;
	result->m20 = z * x * omc - y * s;
	result->m30 = 0.0f;

	result->m01 = x * y * omc - z * s;
	result->m11 = y * y * omc + c;
	result->m21 = z * y * omc + x * s;
	result->m31 = 0.0f;

	result->m02 = x * z * omc + y * s;
	result->m12 = y * z * omc - x * s;
	result->m22 = z * z * omc + c;
	result->m32 = 0.0f;

	result->m03 = 0.0f;
	result->m13 = 0.0f;
	result->m23 = 0.0f;
	result->m33 = 1.0f;
	
	return result;
}

tgm_mat4f* tgm_m4f_euler(tgm_mat4f* result, f32 pitch_in_radians, f32 yaw_in_radians, f32 roll_in_radians)
{
	TG_ASSERT(result);

	tgm_mat4f x = *tgm_m4f_rotate_x(&x, pitch_in_radians);
	tgm_mat4f y = *tgm_m4f_rotate_y(&y, yaw_in_radians);
	tgm_mat4f z = *tgm_m4f_rotate_z(&z, roll_in_radians);

	return tgm_m4f_multiply_m4f(result, &x, tgm_m4f_multiply_m4f(result, &y, &z));
}

tgm_mat4f* tgm_m4f_identity(tgm_mat4f* result)
{
	TG_ASSERT(result);

	result->m00 = 1.0f;
	result->m10 = 0.0f;
	result->m20 = 0.0f;
	result->m30 = 0.0f;

	result->m01 = 0.0f;
	result->m11 = 1.0f;
	result->m21 = 0.0f;
	result->m31 = 0.0f;

	result->m02 = 0.0f;
	result->m12 = 0.0f;
	result->m22 = 1.0f;
	result->m32 = 0.0f;
	
	result->m03 = 0.0f;
	result->m13 = 0.0f;
	result->m23 = 0.0f;
	result->m33 = 1.0f;

	return result;
}

tgm_mat4f* tgm_m4f_look_at(tgm_mat4f* result, tgm_vec3f* from, tgm_vec3f* to, tgm_vec3f* up)
{
	TG_ASSERT(result && from && to && up && !tgm_v3f_equal(from, to) && !tgm_v3f_equal(from, up) && !tgm_v3f_equal(to, up));

	tgm_vec3f f = *tgm_v3f_normalize(&f, tgm_v3f_subtract_v3f(&f, to, from));
	tgm_vec3f r = *tgm_v3f_normalize(&r, tgm_v3f_cross(&r, &f, tgm_v3f_normalize(up, up)));
	tgm_vec3f u = *tgm_v3f_normalize(&u, tgm_v3f_cross(&u, &r, &f));
	tgm_v3f_negate(&f, &f);
	tgm_vec3f from_negated = *tgm_v3f_negate(&from_negated, from);
	
	result->m00 = r.x;
	result->m10 = u.x;
	result->m20 = f.x;
	result->m30 = 0.0f;

	result->m01 = r.y;
	result->m11 = u.y;
	result->m21 = f.y;
	result->m31 = 0.0f;

	result->m02 = r.z;
	result->m12 = u.z;
	result->m22 = f.z;
	result->m32 = 0.0f;

	result->m03 = tgm_v3f_dot(&r, &from_negated);
	result->m13 = tgm_v3f_dot(&u, &from_negated);
	result->m23 = tgm_v3f_dot(&f, &from_negated);
	result->m33 = 1.0f;

	return result;
}

tgm_mat4f* tgm_m4f_multiply_m4f(tgm_mat4f* result, tgm_mat4f* m0, tgm_mat4f* m1)
{
	TG_ASSERT(result && m0 && m1);

	tgm_mat4f res = { 0 };

	res.m00 = m0->m00 * m1->m00 + m0->m01 * m1->m10 + m0->m02 * m1->m20 + m0->m03 * m1->m30;
	res.m10 = m0->m10 * m1->m00 + m0->m11 * m1->m10 + m0->m12 * m1->m20 + m0->m13 * m1->m30;
	res.m20 = m0->m20 * m1->m00 + m0->m21 * m1->m10 + m0->m22 * m1->m20 + m0->m23 * m1->m30;
	res.m30 = m0->m30 * m1->m00 + m0->m31 * m1->m10 + m0->m32 * m1->m20 + m0->m33 * m1->m30;

	res.m01 = m0->m00 * m1->m01 + m0->m01 * m1->m11 + m0->m02 * m1->m21 + m0->m03 * m1->m31;
	res.m11 = m0->m10 * m1->m01 + m0->m11 * m1->m11 + m0->m12 * m1->m21 + m0->m13 * m1->m31;
	res.m21 = m0->m20 * m1->m01 + m0->m21 * m1->m11 + m0->m22 * m1->m21 + m0->m23 * m1->m31;
	res.m31 = m0->m30 * m1->m01 + m0->m31 * m1->m11 + m0->m32 * m1->m21 + m0->m33 * m1->m31;

	res.m02 = m0->m00 * m1->m02 + m0->m01 * m1->m12 + m0->m02 * m1->m22 + m0->m03 * m1->m32;
	res.m12 = m0->m10 * m1->m02 + m0->m11 * m1->m12 + m0->m12 * m1->m22 + m0->m13 * m1->m32;
	res.m22 = m0->m20 * m1->m02 + m0->m21 * m1->m12 + m0->m22 * m1->m22 + m0->m23 * m1->m32;
	res.m32 = m0->m30 * m1->m02 + m0->m31 * m1->m12 + m0->m32 * m1->m22 + m0->m33 * m1->m32;

	res.m03 = m0->m00 * m1->m03 + m0->m01 * m1->m13 + m0->m02 * m1->m23 + m0->m03 * m1->m33;
	res.m13 = m0->m10 * m1->m03 + m0->m11 * m1->m13 + m0->m12 * m1->m23 + m0->m13 * m1->m33;
	res.m23 = m0->m20 * m1->m03 + m0->m21 * m1->m13 + m0->m22 * m1->m23 + m0->m23 * m1->m33;
	res.m33 = m0->m30 * m1->m03 + m0->m31 * m1->m13 + m0->m32 * m1->m23 + m0->m33 * m1->m33;

	*result = res;
	return result;
}

tgm_vec4f* tgm_m4f_multiply_v4f(tgm_vec4f* result, const tgm_mat4f* m, tgm_vec4f* v)
{
	TG_ASSERT(result && m && v);

	tgm_vec4f res = { 0 };

	res.x = v->x * m->m00 + v->y * m->m01 + v->z * m->m02 + v->w * m->m03;
	res.y = v->x * m->m10 + v->y * m->m11 + v->z * m->m12 + v->w * m->m13;
	res.z = v->x * m->m20 + v->y * m->m21 + v->z * m->m22 + v->w * m->m23;
	res.w = v->x * m->m30 + v->y * m->m31 + v->z * m->m32 + v->w * m->m33;

	*result = res;
	return result;
}

// NOTE: vulkan uses left = -1.0, right = 1.0, bottom = 1.0, top = -1.0, near = 0.0 and far = 1.0
tgm_mat4f* tgm_m4f_orthographic(tgm_mat4f* result, f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near)
{
	TG_ASSERT(result && left != right && top != bottom && near != far);

	result->m00 = 2.0f / (right - left);
	result->m10 = 0.0f;
	result->m20 = 0.0f;
	result->m30 = 0.0f;

	result->m01 = 0.0f;
	result->m11 = -2.0f / (top - bottom);
	result->m21 = 0.0f;
	result->m31 = 0.0f;

	result->m02 = 0.0f;
	result->m12 = 0.0f;
	result->m22 = -1.0f / (near - far);
	result->m32 = 0.0f;

	result->m03 = -(right + left) / (right - left);
	result->m13 = -(bottom + top) / (bottom - top);
	result->m23 = near / (near - far);
	result->m33 = 1.0f;

	return result;
}

tgm_mat4f* tgm_m4f_perspective(tgm_mat4f* result, f32 fov_y_in_radians, f32 aspect, f32 near, f32 far)
{
	TG_ASSERT(result);

	const f32 tan_half_fov_y = (f32)tan((f64)fov_y_in_radians / 2.0f);

	const f32 a = -far / (far - near);
	const f32 b = -(2.0f * far * near) / (near - far);

	result->m00 = 1.0f / (aspect * tan_half_fov_y);
	result->m10 = 0.0f;
	result->m20 = 0.0f;
	result->m30 = 0.0f;

	result->m01 = 0.0f;
	result->m11 = -1.0f / tan_half_fov_y;
	result->m21 = 0.0f;
	result->m31 = 0.0f;

	result->m02 = 0.0f;
	result->m12 = 0.0f;
	result->m22 = a;
	result->m32 = -1.0f;

	result->m03 = 0.0f;
	result->m13 = 0.0f;
	result->m23 = b;
	result->m33 = 0.0f;

	return result;
}

tgm_mat4f* tgm_m4f_rotate_x(tgm_mat4f* result, f32 angle_in_radians)
{
	TG_ASSERT(result);

	result->m00 = 1.0f;
	result->m10 = 0.0f;
	result->m20 = 0.0f;
	result->m30 = 0.0f;

	result->m01 = 0.0f;
	result->m11 = (f32)cos((f64)angle_in_radians);
	result->m21 = (f32)sin((f64)angle_in_radians);
	result->m31 = 0.0f;

	result->m02 = 0.0f;
	result->m12 = (f32)-sin((f64)angle_in_radians);
	result->m22 = (f32)cos((f64)angle_in_radians);
	result->m32 = 0.0f;

	result->m03 = 0.0f;
	result->m13 = 0.0f;
	result->m23 = 0.0f;
	result->m33 = 1.0f;

	return result;
}

tgm_mat4f* tgm_m4f_rotate_y(tgm_mat4f* result, f32 angle_in_radians)
{
	TG_ASSERT(result);

	result->m00 = (f32)cos((f64)angle_in_radians);
	result->m10 = 0.0f;
	result->m20 = (f32)-sin((f64)angle_in_radians);
	result->m30 = 0.0f;

	result->m01 = 0.0f;
	result->m11 = 1.0f;
	result->m21 = 0.0f;
	result->m31 = 0.0f;

	result->m02 = (f32)sin((f64)angle_in_radians);
	result->m12 = 0.0f;
	result->m22 = (f32)cos((f64)angle_in_radians);
	result->m32 = 0.0f;

	result->m03 = 0.0f;
	result->m13 = 0.0f;
	result->m23 = 0.0f;
	result->m33 = 1.0f;

	return result;
}

tgm_mat4f* tgm_m4f_rotate_z(tgm_mat4f* result, f32 angle_in_radians)
{
	TG_ASSERT(result);

	result->m00 = (f32)cos((f64)angle_in_radians);
	result->m10 = (f32)sin((f64)angle_in_radians);
	result->m20 = 0.0f;
	result->m30 = 0.0f;

	result->m01 = (f32)-sin((f64)angle_in_radians);
	result->m11 = (f32)cos((f64)angle_in_radians);
	result->m21 = 0.0f;
	result->m31 = 0.0f;

	result->m02 = 0.0f;
	result->m12 = 0.0f;
	result->m22 = 1.0f;
	result->m32 = 0.0f;

	result->m03 = 0.0f;
	result->m13 = 0.0f;
	result->m23 = 0.0f;
	result->m33 = 1.0f;

	return result;
}

tgm_mat4f* tgm_m4f_translate(tgm_mat4f* result, const tgm_vec3f* v)
{
	TG_ASSERT(result && v);

	result->m00 = 1.0f;
	result->m10 = 0.0f;
	result->m20 = 0.0f;
	result->m30 = 0.0f;

	result->m01 = 0.0f;
	result->m11 = 1.0f;
	result->m21 = 0.0f;
	result->m31 = 0.0f;

	result->m02 = 0.0f;
	result->m12 = 0.0f;
	result->m22 = 1.0f;
	result->m32 = 0.0f;

	result->m03 = v->x;
	result->m13 = v->y;
	result->m23 = v->z;
	result->m33 = 1.0f;

	return result;
}

tgm_mat4f* tgm_m4f_transpose(tgm_mat4f* result, tgm_mat4f* m)
{
	TG_ASSERT(result && m);

	tgm_mat4f res = { 0 };

	res.m00 = m->m00;
	res.m10 = m->m01;
	res.m20 = m->m02;
	res.m30 = m->m03;

	res.m01 = m->m10;
	res.m11 = m->m11;
	res.m21 = m->m12;
	res.m31 = m->m13;

	res.m02 = m->m20;
	res.m12 = m->m21;
	res.m22 = m->m22;
	res.m32 = m->m23;

	res.m03 = m->m30;
	res.m13 = m->m31;
	res.m23 = m->m32;
	res.m33 = m->m33;

	*result = res;
	return result;
}
