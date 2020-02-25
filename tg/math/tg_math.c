#include "tg_math.h"

#include <math.h>

float32 tgm_f32_clamp(float32 v, float32 low, float32 high)
{
	return tgm_f32_max(low, tgm_f32_min(high, v));
}

float32 tgm_f32_max(float32 v0, float32 v1)
{
	return v0 > v1 ? v0 : v1;
}

float32 tgm_f32_min(float32 v0, float32 v1)
{
	return v0 < v1 ? v0 : v1;
}

uint32 tgm_ui32_clamp(uint32 v, uint32 low, uint32 high)
{
	return tgm_ui32_max(low, tgm_ui32_min(high, v));
}

uint32 tgm_ui32_max(uint32 v0, uint32 v1)
{
	return v0 > v1 ? v0 : v1;
}

uint32 tgm_ui32_min(uint32 v0, uint32 v1)
{
	return v0 < v1 ? v0 : v1;
}



tgm_vec3f* tgm_v3f_add_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1)
{
	result->x = v0->x + v1->x;
	result->y = v0->y + v1->y;
	result->z = v0->z + v1->z;
	return result;
}

tgm_vec3f* tgm_v3f_add_f(tgm_vec3f* result, tgm_vec3f* v0, float32 f)
{
	result->x = v0->x + f;
	result->y = v0->y + f;
	result->z = v0->z + f;
	return result;
}

tgm_vec3f* tgm_v3f_cross(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1)
{
	result->x = v0->y * v1->z - v0->z * v1->y;
	result->y = v0->z * v1->x - v0->x * v1->z;
	result->z = v0->x * v1->y - v0->y * v1->x;
	return result;
}

tgm_vec3f* tgm_v3f_divide_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1)
{
	result->x = v0->x / v1->x;
	result->y = v0->y / v1->y;
	result->z = v0->z / v1->z;
	return result;
}

tgm_vec3f* tgm_v3f_divide_f(tgm_vec3f* result, tgm_vec3f* v0, float32 f)
{
	result->x = v0->x / f;
	result->y = v0->y / f;
	result->z = v0->z / f;
	return result;
}

float32 tgm_v3f_dot(const tgm_vec3f* v0, const tgm_vec3f* v1)
{
	return v0->x * v1->x + v0->y * v1->y + v0->z * v1->z;
}

float32 tgm_v3f_magnitude(const tgm_vec3f* v)
{
	return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

float32 tgm_v3f_magnitude_squared(const tgm_vec3f* v)
{
	return v->x * v->x + v->y * v->y + v->z * v->z;
}

tgm_vec3f* tgm_v3f_multiply_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1)
{
	result->x = v0->x * v1->x;
	result->y = v0->y * v1->y;
	result->z = v0->z * v1->z;
	return result;
}

tgm_vec3f* tgm_v3f_multiply_f(tgm_vec3f* result, tgm_vec3f* v0, float32 f)
{
	result->x = v0->x * f;
	result->y = v0->y * f;
	result->z = v0->z * f;
	return result;
}

tgm_vec3f* tgm_v3f_negative(tgm_vec3f* result, tgm_vec3f* v0)
{
	result->x = -v0->x;
	result->y = -v0->y;
	result->z = -v0->z;
	return result;
}

tgm_vec3f* tgm_v3f_normalize(tgm_vec3f* result, tgm_vec3f* v0)
{
	const float32 magnitude = tgm_v3f_magnitude(v0);
	result->x = v0->x / magnitude;
	result->y = v0->y / magnitude;
	result->z = v0->z / magnitude;
	return result;
}

tgm_vec3f* tgm_v3f_subtract_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1)
{
	result->x = v0->x - v1->x;
	result->y = v0->y - v1->y;
	result->z = v0->z - v1->z;
	return result;
}

tgm_vec3f* tgm_v3f_subtract_f(tgm_vec3f* result, tgm_vec3f* v0, float32 f)
{
	result->x = v0->x - f;
	result->y = v0->y - f;
	result->z = v0->z - f;
	return result;
}



tgm_mat2f* tgm_m2f_identity(tgm_mat2f* result)
{
	result->m00 = 1.0f;
	result->m10 = 0.0f;

	result->m01 = 0.0f;
	result->m11 = 1.0f;

	return result;
}

tgm_mat2f* tgm_m2f_multiply_m2f(tgm_mat2f* result, tgm_mat2f* m0, tgm_mat2f* m1)
{
	result->m00 = m0->m00 * m1->m00 + m0->m01 * m1->m10;
	result->m01 = m0->m00 * m1->m01 + m0->m01 * m1->m11;

	result->m10 = m0->m10 * m1->m00 + m0->m11 * m1->m10;
	result->m11 = m0->m10 * m1->m01 + m0->m11 * m1->m11;

	return result;
}

tgm_vec2f* tgm_m2f_multiply_v2f(tgm_vec2f* result, tgm_mat2f* m, tgm_vec2f* v)
{
	result->x = v->x * m->m00 + v->y * m->m01;
	result->y = v->x * m->m10 + v->y * m->m11;

	return result;
}

tgm_mat2f* tgm_m2f_transpose(tgm_mat2f* result, tgm_mat2f* m)
{
	float32 temp;

	result->m00 = m->m00;
	result->m11 = m->m11;

	temp = m->m01;
	result->m01 = m->m10;
	result->m10 = temp;

	return result;
}



tgm_mat3f* tgm_m3f_identity(tgm_mat3f* result)
{
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
	result->m00 = m0->m00 * m1->m00 + m0->m01 * m1->m10 + m0->m02 * m1->m20;
	result->m01 = m0->m00 * m1->m01 + m0->m01 * m1->m11 + m0->m02 * m1->m21;
	result->m02 = m0->m00 * m1->m02 + m0->m01 * m1->m12 + m0->m02 * m1->m22;
	
	result->m10 = m0->m10 * m1->m00 + m0->m11 * m1->m10 + m0->m12 * m1->m20;
	result->m11 = m0->m10 * m1->m01 + m0->m11 * m1->m11 + m0->m12 * m1->m21;
	result->m12 = m0->m10 * m1->m02 + m0->m11 * m1->m12 + m0->m12 * m1->m22;
	
	result->m20 = m0->m20 * m1->m00 + m0->m21 * m1->m10 + m0->m22 * m1->m20;
	result->m21 = m0->m20 * m1->m01 + m0->m21 * m1->m11 + m0->m22 * m1->m21;
	result->m22 = m0->m20 * m1->m02 + m0->m21 * m1->m12 + m0->m22 * m1->m22;

	return result;
}

tgm_vec3f* tgm_m3f_multiply_v3f(tgm_vec3f* result, tgm_mat3f* m, tgm_vec3f* v)
{
	result->x = v->x * m->m00 + v->y * m->m01 + v->z * m->m02;
	result->y = v->x * m->m10 + v->y * m->m11 + v->z * m->m12;
	result->z = v->x * m->m20 + v->y * m->m21 + v->z * m->m22;

	return result;
}

tgm_mat3f* tgm_m3f_orthographic(tgm_mat3f* result, float32 left, float32 right, float32 bottom, float32 top)
{
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
	float32 temp;

	result->m00 = m->m00;
	result->m11 = m->m11;
	result->m22 = m->m22;

	temp = m->m01;
	result->m01 = m->m10;
	result->m10 = temp;

	temp = m->m02;
	result->m02 = m->m20;
	result->m20 = temp;

	temp = m->m12;
	result->m12 = m->m21;
	result->m21 = temp;

	return result;
}



tgm_mat4f* tgm_m4f_angle_axis(tgm_mat4f* result, float32 angle_in_radians, tgm_vec3f* axis)
{
	const float32 c = (float32)cos((double)angle_in_radians);
	const float32 s = (float32)sin((double)angle_in_radians);
	const float32 omc = 1.0f - c;
	const float32 x = axis->x;
	const float32 y = axis->y;
	const float32 z = axis->z;
	const float32 xx = x * x;
	const float32 xy = x * y;
	const float32 xz = x * z;
	const float32 yy = y * y;
	const float32 yz = y * z;
	const float32 zz = z * z;
	const float32 l = xx + yy + zz;
	const float32 sqrt_l = (float32)sqrt((double)l);

	result->m00 = (xx + (yy + zz) * c) / l;
	result->m01 = (xy * omc + z * sqrt_l * s) / l;
	result->m02 = (xz * omc - y * sqrt_l * s) / l;
	result->m03 = 0.0f;
	result->m10 = (xy * omc - z * sqrt_l * s) / l;
	result->m11 = (yy + (xx + zz) * c) / l;
	result->m12 = (yz * omc + x * sqrt_l * s) / l;
	result->m13 = 0.0f;
	result->m20 = (xz * omc + y * sqrt_l * s) / l;
	result->m21 = (yz * omc - x * sqrt_l * s) / l;
	result->m22 = (zz + (xx + yy) * c) / l;
	result->m23 = 0.0f;
	result->m30 = 0.0f;
	result->m31 = 0.0f;
	result->m32 = 0.0f;
	result->m33 = 1.0f;
	
	return result;

	// TODO: test performance of both variants
	//const float c = (float)cos((double)angle_in_radians);
	//const float s = (float)sin((double)angle_in_radians);
	//const float omc = 1.0f - c;
	//tg_vec3f unit_axis;
	//tg_vec3f_normalize(&unit_axis, axis);
	//const float x = unit_axis.data[0];
	//const float y = unit_axis.data[1];
	//const float z = unit_axis.data[2];
	//result->data[0] = x * x * omc + c;
	//result->data[1] = x * y * omc - z * s;
	//result->data[2] = x * z * omc + y * s;
	//result->data[3] = 0.0f;
	//result->data[4] = y * x * omc + z * s;
	//result->data[5] = y * y * omc + c;
	//result->data[6] = y * z * omc - x * s;
	//result->data[7] = 0.0f;
	//result->data[8] = z * x * omc - y * s;
	//result->data[9] = z * y * omc + x * s;
	//result->data[10] = z * z * omc + c;
	//result->data[11] = 0.0f;
	//result->data[12] = 0.0f;
	//result->data[13] = 0.0f;
	//result->data[14] = 0.0f;
	//result->data[15] = 1.0f;
	//return result;
}

tgm_mat4f* tgm_m4f_identity(tgm_mat4f* result)
{
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
	tgm_vec3f f = *tgm_v3f_normalize(&f, tgm_v3f_subtract_v3f(&f, to, from));
	tgm_vec3f r = *tgm_v3f_normalize(&r, tgm_v3f_cross(&r, &f, tgm_v3f_normalize(&f, up)));
	tgm_vec3f u = *tgm_v3f_normalize(&u, tgm_v3f_cross(&u, &r, &f));
	tgm_vec3f from_negative = *tgm_v3f_negative(&from_negative, from);
	
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

	result->m03 = tgm_v3f_dot(&r, &from_negative);
	result->m13 = tgm_v3f_dot(&u, &from_negative);
	result->m23 = tgm_v3f_dot(&f, &from_negative);
	result->m33 = 1.0f;

	return result;
}

tgm_mat4f* tgm_m4f_multiply_m4f(tgm_mat4f* result, tgm_mat4f* m0, tgm_mat4f* m1)
{
	result->m00 = m0->m00 * m1->m00 + m0->m01 * m1->m10 + m0->m02 * m1->m20 + m0->m03 * m1->m30;
	result->m01 = m0->m00 * m1->m01 + m0->m01 * m1->m11 + m0->m02 * m1->m21 + m0->m03 * m1->m31;
	result->m02 = m0->m00 * m1->m02 + m0->m01 * m1->m12 + m0->m02 * m1->m22 + m0->m03 * m1->m32;
	result->m03 = m0->m00 * m1->m03 + m0->m01 * m1->m13 + m0->m02 * m1->m23 + m0->m03 * m1->m33;
	
	result->m10 = m0->m10 * m1->m00 + m0->m11 * m1->m10 + m0->m12 * m1->m20 + m0->m13 * m1->m30;
	result->m11 = m0->m10 * m1->m01 + m0->m11 * m1->m11 + m0->m12 * m1->m21 + m0->m13 * m1->m31;
	result->m12 = m0->m10 * m1->m02 + m0->m11 * m1->m12 + m0->m12 * m1->m22 + m0->m13 * m1->m32;
	result->m13 = m0->m10 * m1->m03 + m0->m11 * m1->m13 + m0->m12 * m1->m23 + m0->m13 * m1->m33;
	
	result->m20 = m0->m20 * m1->m00 + m0->m21 * m1->m10 + m0->m22 * m1->m20 + m0->m23 * m1->m30;
	result->m21 = m0->m20 * m1->m01 + m0->m21 * m1->m11 + m0->m22 * m1->m21 + m0->m23 * m1->m31;
	result->m22 = m0->m20 * m1->m02 + m0->m21 * m1->m12 + m0->m22 * m1->m22 + m0->m23 * m1->m32;
	result->m23 = m0->m20 * m1->m03 + m0->m21 * m1->m13 + m0->m22 * m1->m23 + m0->m23 * m1->m33;
	
	result->m30 = m0->m30 * m1->m00 + m0->m31 * m1->m10 + m0->m32 * m1->m20 + m0->m33 * m1->m30;
	result->m31 = m0->m30 * m1->m01 + m0->m31 * m1->m11 + m0->m32 * m1->m21 + m0->m33 * m1->m31;
	result->m32 = m0->m30 * m1->m02 + m0->m31 * m1->m12 + m0->m32 * m1->m22 + m0->m33 * m1->m32;
	result->m33 = m0->m30 * m1->m03 + m0->m31 * m1->m13 + m0->m32 * m1->m23 + m0->m33 * m1->m33;

	return result;
}

tgm_vec4f* tgm_m4f_multiply_v4f(tgm_vec4f* result, tgm_mat4f* m, tgm_vec4f* v)
{
	result->x = v->x * m->m00 + v->y * m->m01 + v->z * m->m02 + v->w * m->m03;
	result->y = v->x * m->m10 + v->y * m->m11 + v->z * m->m12 + v->w * m->m13;
	result->z = v->x * m->m20 + v->y * m->m21 + v->z * m->m22 + v->w * m->m23;
	result->w = v->x * m->m30 + v->y * m->m31 + v->z * m->m32 + v->w * m->m33;

	return result;
}

// NOTE: vulkan uses left = -1.0, right = 1.0, bottom = 1.0, top = -1.0, near = 0.0 and far = 1.0
tgm_mat4f* tgm_m4f_orthographic(tgm_mat4f* result, float32 left, float32 right, float32 bottom, float32 top, float32 far, float32 near)
{
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

tgm_mat4f* tgm_m4f_perspective(tgm_mat4f* result, float32 fov_y_in_radians, float32 aspect, float32 near, float32 far)
{
	const float32 tan_half_fov_y = (float32)tan((double)fov_y_in_radians / 2.0f);

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
	result->m22 = -far / (far - near);
	result->m32 = -(2.0f * far * near) / (near - far);

	result->m03 = 0.0f;
	result->m13 = 0.0f;
	result->m23 = -1.0f;
	result->m33 = 0.0f;

	return result;
}

tgm_mat4f* tgm_m4f_translate(tgm_mat4f* result, tgm_vec3f* v)
{
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
	float32 temp;

	result->m00 = m->m00;
	result->m11 = m->m11;
	result->m22 = m->m22;
	result->m33 = m->m33;

	temp = m->m01;
	result->m01 = m->m10;
	result->m10 = temp;

	temp = m->m02;
	result->m02 = m->m20;
	result->m20 = temp;

	temp = m->m03;
	result->m03 = m->m30;
	result->m30 = temp;

	temp = m->m12;
	result->m12 = m->m21;
	result->m21 = temp;

	temp = m->m13;
	result->m13 = m->m31;
	result->m31 = temp;

	temp = m->m23;
	result->m23 = m->m32;
	result->m32 = temp;

	return result;
}
