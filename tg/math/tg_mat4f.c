#include "tg_mat4f.h"

#include <math.h>

tg_mat4f* tg_mat4f_angle_axis(tg_mat4f* result, tg_math_float_t angle_in_radians, tg_vec3f* axis)
{
	const tg_math_float_t c = (tg_math_float_t)cos((double)angle_in_radians);
	const tg_math_float_t s = (tg_math_float_t)sin((double)angle_in_radians);
	const tg_math_float_t omc = TG_MATH_FLOAT(1.0) - c;
	const tg_math_float_t x = axis->x;
	const tg_math_float_t y = axis->y;
	const tg_math_float_t z = axis->z;
	const tg_math_float_t xx = x * x;
	const tg_math_float_t xy = x * y;
	const tg_math_float_t xz = x * z;
	const tg_math_float_t yy = y * y;
	const tg_math_float_t yz = y * z;
	const tg_math_float_t zz = z * z;
	const tg_math_float_t l = xx + yy + zz;
	const tg_math_float_t sqrt_l = (tg_math_float_t)sqrt((double)l);
	TG_MAT4F_AT(result, 0, 0) = (xx + (yy + zz) * c) / l;
	TG_MAT4F_AT(result, 0, 1) = (xy * omc + z * sqrt_l * s) / l;
	TG_MAT4F_AT(result, 0, 2) = (xz * omc - y * sqrt_l * s) / l;
	TG_MAT4F_AT(result, 0, 3) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 1, 0) = (xy * omc - z * sqrt_l * s) / l;
	TG_MAT4F_AT(result, 1, 1) = (yy + (xx + zz) * c) / l;
	TG_MAT4F_AT(result, 1, 2) = (yz * omc + x * sqrt_l * s) / l;
	TG_MAT4F_AT(result, 1, 3) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 2, 0) = (xz * omc + y * sqrt_l * s) / l;
	TG_MAT4F_AT(result, 2, 1) = (yz * omc - x * sqrt_l * s) / l;
	TG_MAT4F_AT(result, 2, 2) = (zz + (xx + yy) * c) / l;
	TG_MAT4F_AT(result, 2, 3) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 3, 0) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 3, 1) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 3, 2) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 3, 3) = TG_MATH_FLOAT(1.0);
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

tg_mat4f* tg_mat4f_identity(tg_mat4f* result)
{
	TG_MAT4F_AT(result, 0, 0) = TG_MATH_FLOAT(1.0);
	TG_MAT4F_AT(result, 0, 1) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 0, 2) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 0, 3) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 1, 0) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 1, 1) = TG_MATH_FLOAT(1.0);
	TG_MAT4F_AT(result, 1, 2) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 1, 3) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 2, 0) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 2, 1) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 2, 2) = TG_MATH_FLOAT(1.0);
	TG_MAT4F_AT(result, 2, 3) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 3, 0) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 3, 1) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 3, 2) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 3, 3) = TG_MATH_FLOAT(1.0);
	return result;
}

tg_mat4f* tg_mat4f_look_at(tg_mat4f* result, tg_vec3f* from, tg_vec3f* to, tg_vec3f* up)
{
	tg_vec3f temp = { 0 };
	tg_vec3f f = { 0 };
	tg_vec3f r = { 0 };
	tg_vec3f u = { 0 };
	tg_vec3f from_negative = { 0 };
	tg_vec3f_normalize(&f, tg_vec3f_subtract(&f, to, from));
	tg_vec3f_normalize(&r, tg_vec3f_cross(&r, &f, tg_vec3f_normalize(&temp, up)));
	tg_vec3f_normalize(&u, tg_vec3f_cross(&u, &r, &f));
	tg_vec3f_negative(&from_negative, from);
	TG_MAT4F_AT(result, 0, 0) = r.x;
	TG_MAT4F_AT(result, 0, 1) = r.y;
	TG_MAT4F_AT(result, 0, 2) = r.z;
	TG_MAT4F_AT(result, 0, 3) = tg_vec3f_dot(&r, &from_negative);
	TG_MAT4F_AT(result, 1, 0) = u.x;
	TG_MAT4F_AT(result, 1, 1) = u.y;
	TG_MAT4F_AT(result, 1, 2) = u.z;
	TG_MAT4F_AT(result, 1, 3) = tg_vec3f_dot(&u, &from_negative);
	TG_MAT4F_AT(result, 2, 0) = f.x;
	TG_MAT4F_AT(result, 2, 1) = f.y;
	TG_MAT4F_AT(result, 2, 2) = f.z;
	TG_MAT4F_AT(result, 2, 3) = tg_vec3f_dot(&f, &from_negative);
	TG_MAT4F_AT(result, 3, 0) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 3, 1) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 3, 2) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 3, 3) = TG_MATH_FLOAT(1.0);
	return result;
}

tg_mat4f* tg_mat4f_multiply(tg_mat4f* result, tg_mat4f* m0, tg_mat4f* m1)
{
	TG_MAT4F_AT(result, 0, 0) = TG_MAT4F_AT(m0, 0, 0) * TG_MAT4F_AT(m1, 0, 0) + TG_MAT4F_AT(m0, 0, 1) * TG_MAT4F_AT(m1, 1, 0) + TG_MAT4F_AT(m0, 0, 2) * TG_MAT4F_AT(m1, 2, 0) + TG_MAT4F_AT(m0, 0, 3) * TG_MAT4F_AT(m1, 3, 0);
	TG_MAT4F_AT(result, 0, 1) = TG_MAT4F_AT(m0, 0, 0) * TG_MAT4F_AT(m1, 0, 1) + TG_MAT4F_AT(m0, 0, 1) * TG_MAT4F_AT(m1, 1, 1) + TG_MAT4F_AT(m0, 0, 2) * TG_MAT4F_AT(m1, 2, 1) + TG_MAT4F_AT(m0, 0, 3) * TG_MAT4F_AT(m1, 3, 1);
	TG_MAT4F_AT(result, 0, 2) = TG_MAT4F_AT(m0, 0, 0) * TG_MAT4F_AT(m1, 0, 2) + TG_MAT4F_AT(m0, 0, 1) * TG_MAT4F_AT(m1, 1, 2) + TG_MAT4F_AT(m0, 0, 2) * TG_MAT4F_AT(m1, 2, 2) + TG_MAT4F_AT(m0, 0, 3) * TG_MAT4F_AT(m1, 3, 2);
	TG_MAT4F_AT(result, 0, 3) = TG_MAT4F_AT(m0, 0, 0) * TG_MAT4F_AT(m1, 0, 3) + TG_MAT4F_AT(m0, 0, 1) * TG_MAT4F_AT(m1, 1, 3) + TG_MAT4F_AT(m0, 0, 2) * TG_MAT4F_AT(m1, 2, 3) + TG_MAT4F_AT(m0, 0, 3) * TG_MAT4F_AT(m1, 3, 3);
	TG_MAT4F_AT(result, 1, 0) = TG_MAT4F_AT(m0, 1, 0) * TG_MAT4F_AT(m1, 0, 0) + TG_MAT4F_AT(m0, 1, 1) * TG_MAT4F_AT(m1, 1, 0) + TG_MAT4F_AT(m0, 1, 2) * TG_MAT4F_AT(m1, 2, 0) + TG_MAT4F_AT(m0, 1, 3) * TG_MAT4F_AT(m1, 3, 0);
	TG_MAT4F_AT(result, 1, 1) = TG_MAT4F_AT(m0, 1, 0) * TG_MAT4F_AT(m1, 0, 1) + TG_MAT4F_AT(m0, 1, 1) * TG_MAT4F_AT(m1, 1, 1) + TG_MAT4F_AT(m0, 1, 2) * TG_MAT4F_AT(m1, 2, 1) + TG_MAT4F_AT(m0, 1, 3) * TG_MAT4F_AT(m1, 3, 1);
	TG_MAT4F_AT(result, 1, 2) = TG_MAT4F_AT(m0, 1, 0) * TG_MAT4F_AT(m1, 0, 2) + TG_MAT4F_AT(m0, 1, 1) * TG_MAT4F_AT(m1, 1, 2) + TG_MAT4F_AT(m0, 1, 2) * TG_MAT4F_AT(m1, 2, 2) + TG_MAT4F_AT(m0, 1, 3) * TG_MAT4F_AT(m1, 3, 2);
	TG_MAT4F_AT(result, 1, 3) = TG_MAT4F_AT(m0, 1, 0) * TG_MAT4F_AT(m1, 0, 3) + TG_MAT4F_AT(m0, 1, 1) * TG_MAT4F_AT(m1, 1, 3) + TG_MAT4F_AT(m0, 1, 2) * TG_MAT4F_AT(m1, 2, 3) + TG_MAT4F_AT(m0, 1, 3) * TG_MAT4F_AT(m1, 3, 3);
	TG_MAT4F_AT(result, 2, 0) = TG_MAT4F_AT(m0, 2, 0) * TG_MAT4F_AT(m1, 0, 0) + TG_MAT4F_AT(m0, 2, 1) * TG_MAT4F_AT(m1, 1, 0) + TG_MAT4F_AT(m0, 2, 2) * TG_MAT4F_AT(m1, 2, 0) + TG_MAT4F_AT(m0, 2, 3) * TG_MAT4F_AT(m1, 3, 0);
	TG_MAT4F_AT(result, 2, 1) = TG_MAT4F_AT(m0, 2, 0) * TG_MAT4F_AT(m1, 0, 1) + TG_MAT4F_AT(m0, 2, 1) * TG_MAT4F_AT(m1, 1, 1) + TG_MAT4F_AT(m0, 2, 2) * TG_MAT4F_AT(m1, 2, 1) + TG_MAT4F_AT(m0, 2, 3) * TG_MAT4F_AT(m1, 3, 1);
	TG_MAT4F_AT(result, 2, 2) = TG_MAT4F_AT(m0, 2, 0) * TG_MAT4F_AT(m1, 0, 2) + TG_MAT4F_AT(m0, 2, 1) * TG_MAT4F_AT(m1, 1, 2) + TG_MAT4F_AT(m0, 2, 2) * TG_MAT4F_AT(m1, 2, 2) + TG_MAT4F_AT(m0, 2, 3) * TG_MAT4F_AT(m1, 3, 2);
	TG_MAT4F_AT(result, 2, 3) = TG_MAT4F_AT(m0, 2, 0) * TG_MAT4F_AT(m1, 0, 3) + TG_MAT4F_AT(m0, 2, 1) * TG_MAT4F_AT(m1, 1, 3) + TG_MAT4F_AT(m0, 2, 2) * TG_MAT4F_AT(m1, 2, 3) + TG_MAT4F_AT(m0, 2, 3) * TG_MAT4F_AT(m1, 3, 3);
	TG_MAT4F_AT(result, 3, 0) = TG_MAT4F_AT(m0, 3, 0) * TG_MAT4F_AT(m1, 0, 0) + TG_MAT4F_AT(m0, 3, 1) * TG_MAT4F_AT(m1, 1, 0) + TG_MAT4F_AT(m0, 3, 2) * TG_MAT4F_AT(m1, 2, 0) + TG_MAT4F_AT(m0, 3, 3) * TG_MAT4F_AT(m1, 3, 0);
	TG_MAT4F_AT(result, 3, 1) = TG_MAT4F_AT(m0, 3, 0) * TG_MAT4F_AT(m1, 0, 1) + TG_MAT4F_AT(m0, 3, 1) * TG_MAT4F_AT(m1, 1, 1) + TG_MAT4F_AT(m0, 3, 2) * TG_MAT4F_AT(m1, 2, 1) + TG_MAT4F_AT(m0, 3, 3) * TG_MAT4F_AT(m1, 3, 1);
	TG_MAT4F_AT(result, 3, 2) = TG_MAT4F_AT(m0, 3, 0) * TG_MAT4F_AT(m1, 0, 2) + TG_MAT4F_AT(m0, 3, 1) * TG_MAT4F_AT(m1, 1, 2) + TG_MAT4F_AT(m0, 3, 2) * TG_MAT4F_AT(m1, 2, 2) + TG_MAT4F_AT(m0, 3, 3) * TG_MAT4F_AT(m1, 3, 2);
	TG_MAT4F_AT(result, 3, 3) = TG_MAT4F_AT(m0, 3, 0) * TG_MAT4F_AT(m1, 0, 3) + TG_MAT4F_AT(m0, 3, 1) * TG_MAT4F_AT(m1, 1, 3) + TG_MAT4F_AT(m0, 3, 2) * TG_MAT4F_AT(m1, 2, 3) + TG_MAT4F_AT(m0, 3, 3) * TG_MAT4F_AT(m1, 3, 3);
	return result;
}

tg_vec4f* tg_mat4f_multiply_v4(tg_vec4f* result, tg_mat4f* m, tg_vec4f* v)
{
	result->x = TG_MAT4F_AT(m, 0, 0) * v->x + TG_MAT4F_AT(m, 0, 1) * v->x + TG_MAT4F_AT(m, 0, 2) * v->x + TG_MAT4F_AT(m, 0, 3) * v->x;
	result->y = TG_MAT4F_AT(m, 1, 0) * v->y + TG_MAT4F_AT(m, 1, 1) * v->y + TG_MAT4F_AT(m, 1, 2) * v->y + TG_MAT4F_AT(m, 1, 3) * v->y;
	result->z = TG_MAT4F_AT(m, 2, 0) * v->z + TG_MAT4F_AT(m, 2, 1) * v->z + TG_MAT4F_AT(m, 2, 2) * v->z + TG_MAT4F_AT(m, 2, 3) * v->z;
	result->w = TG_MAT4F_AT(m, 3, 0) * v->w + TG_MAT4F_AT(m, 3, 1) * v->w + TG_MAT4F_AT(m, 3, 2) * v->w + TG_MAT4F_AT(m, 3, 3) * v->w;
	return result;
}

tg_mat4f* tg_mat4f_orthographic(tg_mat4f* result, tg_math_float_t left, tg_math_float_t right, tg_math_float_t bottom, tg_math_float_t top, tg_math_float_t near, tg_math_float_t far)
{
	TG_MAT4F_AT(result, 0, 0) = TG_MATH_FLOAT(2.0) / (right - left);
	TG_MAT4F_AT(result, 0, 1) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 0, 2) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 0, 3) = -(right + left) / (right - left);
	TG_MAT4F_AT(result, 1, 0) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 1, 1) = TG_MATH_FLOAT(2.0) / (bottom - top);
	TG_MAT4F_AT(result, 1, 2) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 1, 3) = -(bottom + top) / (bottom - top);
	TG_MAT4F_AT(result, 2, 0) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 2, 1) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 2, 2) = TG_MATH_FLOAT(-1.0) / (near - far);
	TG_MAT4F_AT(result, 2, 3) = near / (near - far);
	TG_MAT4F_AT(result, 3, 0) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 3, 1) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 3, 2) = TG_MATH_FLOAT(0.0);
	TG_MAT4F_AT(result, 3, 3) = TG_MATH_FLOAT(1.0);
	return result;
}

tg_mat4f* tg_mat4f_perspective(tg_mat4f* result, tg_math_float_t fov_y_in_radians, tg_math_float_t aspect, tg_math_float_t near, tg_math_float_t far)
{
	const tg_math_float_t f = TG_MATH_FLOAT(1.0) / (tg_math_float_t)tan(0.5 * (double)fov_y_in_radians);
	TG_MAT4F_AT(result, 0, 0) = f / aspect;
	TG_MAT4F_AT(result, 0, 1) = 0.0f;
	TG_MAT4F_AT(result, 0, 2) = 0.0f;
	TG_MAT4F_AT(result, 0, 3) = 0.0f;
	TG_MAT4F_AT(result, 1, 0) = 0.0f;
	TG_MAT4F_AT(result, 1, 1) = -f;
	TG_MAT4F_AT(result, 1, 2) = 0.0f;
	TG_MAT4F_AT(result, 1, 3) = 0.0f;
	TG_MAT4F_AT(result, 2, 0) = 0.0f;
	TG_MAT4F_AT(result, 2, 1) = 0.0f;
	TG_MAT4F_AT(result, 2, 2) = -far / (near - far);
	TG_MAT4F_AT(result, 2, 3) = (near * far) / (near - far);
	TG_MAT4F_AT(result, 3, 0) = 0.0f;
	TG_MAT4F_AT(result, 3, 1) = 0.0f;
	TG_MAT4F_AT(result, 3, 2) = -1.0f;
	TG_MAT4F_AT(result, 3, 3) = 0.0f;

	const tg_math_float_t tan_half_fov_y = (tg_math_float_t)tan((double)fov_y_in_radians / TG_MATH_FLOAT(2.0));
	TG_MAT4F_AT(result, 0, 0) = 1.0f / (aspect * tan_half_fov_y);
	TG_MAT4F_AT(result, 0, 1) = 0.0f;
	TG_MAT4F_AT(result, 0, 2) = 0.0f;
	TG_MAT4F_AT(result, 0, 3) = 0.0f;
	TG_MAT4F_AT(result, 1, 0) = 0.0f;
	TG_MAT4F_AT(result, 1, 1) = 1.0f / tan_half_fov_y;
	TG_MAT4F_AT(result, 1, 2) = 0.0f;
	TG_MAT4F_AT(result, 1, 3) = 0.0f;
	TG_MAT4F_AT(result, 2, 0) = 0.0f;
	TG_MAT4F_AT(result, 2, 1) = 0.0f;
	TG_MAT4F_AT(result, 2, 2) = far / (near - far);
	TG_MAT4F_AT(result, 2, 3) = 1.0f;
	TG_MAT4F_AT(result, 3, 0) = 0.0f;
	TG_MAT4F_AT(result, 3, 1) = 0.0f;
	TG_MAT4F_AT(result, 3, 2) = -(far * near) / (far - near);
	TG_MAT4F_AT(result, 3, 3) = 0.0f;

	TG_MAT4F_AT(result, 0, 0) = 1.0f / (aspect * tan_half_fov_y);
	TG_MAT4F_AT(result, 0, 1) = 0.0f;
	TG_MAT4F_AT(result, 0, 2) = 0.0f;
	TG_MAT4F_AT(result, 0, 3) = 0.0f;
	TG_MAT4F_AT(result, 1, 0) = 0.0f;
	TG_MAT4F_AT(result, 1, 1) = 1.0f / tan_half_fov_y;
	TG_MAT4F_AT(result, 1, 2) = 0.0f;
	TG_MAT4F_AT(result, 1, 3) = 0.0f;
	TG_MAT4F_AT(result, 2, 0) = 0.0f;
	TG_MAT4F_AT(result, 2, 1) = 0.0f;
	TG_MAT4F_AT(result, 2, 2) = near / (far - near);
	TG_MAT4F_AT(result, 2, 3) = -1.0f;
	TG_MAT4F_AT(result, 3, 0) = 0.0f;
	TG_MAT4F_AT(result, 3, 1) = 0.0f;
	TG_MAT4F_AT(result, 3, 2) = (far * near) / (far - near);
	TG_MAT4F_AT(result, 3, 3) = 0.0f;
	return result;
}

tg_mat4f* tg_mat4f_transpose(tg_mat4f* result, tg_mat4f* m)
{
	TG_MAT4F_AT(result, 0, 0) = TG_MAT4F_AT(m, 0, 0);
	TG_MAT4F_AT(result, 0, 1) = TG_MAT4F_AT(m, 1, 0);
	TG_MAT4F_AT(result, 0, 2) = TG_MAT4F_AT(m, 2, 0);
	TG_MAT4F_AT(result, 0, 3) = TG_MAT4F_AT(m, 3, 0);
	TG_MAT4F_AT(result, 1, 0) = TG_MAT4F_AT(m, 0, 1);
	TG_MAT4F_AT(result, 1, 1) = TG_MAT4F_AT(m, 1, 1);
	TG_MAT4F_AT(result, 1, 2) = TG_MAT4F_AT(m, 2, 1);
	TG_MAT4F_AT(result, 1, 3) = TG_MAT4F_AT(m, 3, 1);
	TG_MAT4F_AT(result, 2, 0) = TG_MAT4F_AT(m, 0, 2);
	TG_MAT4F_AT(result, 2, 1) = TG_MAT4F_AT(m, 1, 2);
	TG_MAT4F_AT(result, 2, 2) = TG_MAT4F_AT(m, 2, 2);
	TG_MAT4F_AT(result, 2, 3) = TG_MAT4F_AT(m, 3, 2);
	TG_MAT4F_AT(result, 3, 0) = TG_MAT4F_AT(m, 0, 3);
	TG_MAT4F_AT(result, 3, 1) = TG_MAT4F_AT(m, 1, 3);
	TG_MAT4F_AT(result, 3, 2) = TG_MAT4F_AT(m, 2, 3);
	TG_MAT4F_AT(result, 3, 3) = TG_MAT4F_AT(m, 3, 3);
	return result;
}
