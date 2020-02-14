#include "tg_mat4f.h"

#include <math.h>

tg_mat4f* tg_mat4f_angle_axis(tg_mat4f* result, float angleInRadians, tg_vec3f* axis)
{
	const double r = angleInRadians;
	const float c = (float)cos((double)r);
	const float s = (float)sin((double)r);
	const float omc = 1.0f - c;
	tg_vec3f unit_axis;
	tg_vec3f_normalize(&unit_axis, axis);
	const float x = unit_axis.x;
	const float y = unit_axis.y;
	const float z = unit_axis.z;
	result->data[0] = x * x * omc + c;
	result->data[1] = x * y * omc - z * s;
	result->data[2] = x * z * omc + y * s;
	result->data[3] = 0.0f;
	result->data[4] = y * x * omc + z * s;
	result->data[5] = y * y * omc + c;
	result->data[6] = y * z * omc - x * s;
	result->data[7] = 0.0f;
	result->data[8] = z * x * omc - y * s;
	result->data[9] = z * y * omc + x * s;
	result->data[10] = z * z * omc + c;
	result->data[11] = 0.0f;
	result->data[12] = 0.0f;
	result->data[13] = 0.0f;
	result->data[14] = 0.0f;
	result->data[15] = 1.0f;
	return result;
}

tg_mat4f* tg_mat4f_identity(tg_mat4f* result)
{
	result->data[0] = 1.0f;
	result->data[1] = 0.0f;
	result->data[2] = 0.0f;
	result->data[3] = 0.0f;
	result->data[4] = 0.0f;
	result->data[5] = 1.0f;
	result->data[6] = 0.0f;
	result->data[7] = 0.0f;
	result->data[8] = 0.0f;
	result->data[9] = 0.0f;
	result->data[10] = 1.0f;
	result->data[11] = 0.0f;
	result->data[12] = 0.0f;
	result->data[13] = 0.0f;
	result->data[14] = 0.0f;
	result->data[15] = 1.0f;
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
	result->data[0] = r.x;
	result->data[1] = r.y;
	result->data[2] = r.z;
	result->data[3] = tg_vec3f_dot(&r, &from_negative);
	result->data[4] = u.x;
	result->data[5] = u.y;
	result->data[6] = u.z;
	result->data[7] = tg_vec3f_dot(&u, &from_negative);
	result->data[8] = f.x;
	result->data[9] = f.y;
	result->data[10] = f.z;
	result->data[11] = tg_vec3f_dot(&f, &from_negative);
	result->data[12] = 0.0f;
	result->data[13] = 0.0f;
	result->data[14] = 0.0f;
	result->data[15] = 1.0f;
	return result;
}

tg_mat4f* tg_mat4f_orthographic(tg_mat4f* result, float left, float right, float top, float bottom, float near, float far)
{
	result->data[0] = 2.0f / (right - left);
	result->data[1] = 0.0f;
	result->data[2] = 0.0f;
	result->data[3] = -(right + left) / (right - left);
	result->data[4] = 0.0f;
	result->data[5] = 2.0f / (top - bottom);
	result->data[6] = 0.0f;
	result->data[7] = -(top + bottom) / (top - bottom);
	result->data[8] = 0.0f;
	result->data[9] = 0.0f;
	result->data[10] = -2.0f / (far - near);
	result->data[11] = (far + near) / (far - near);
	result->data[12] = 0.0f;
	result->data[13] = 0.0f;
	result->data[14] = 0.0f;
	result->data[15] = 1.0f;
	return result;
}

tg_mat4f* tg_mat4f_perspective(tg_mat4f* result, float fov, float aspect, float near, float far)
{
	return result;
}

tg_mat4f* tg_mat4f_transposed(tg_mat4f* result, tg_mat4f* mat)
{
	result->data[0] = mat->data[0];
	result->data[1] = mat->data[4];
	result->data[2] = mat->data[8];
	result->data[3] = mat->data[12];
	result->data[4] = mat->data[1];
	result->data[5] = mat->data[5];
	result->data[6] = mat->data[9];
	result->data[7] = mat->data[13];
	result->data[8] = mat->data[2];
	result->data[9] = mat->data[6];
	result->data[10] = mat->data[10];
	result->data[11] = mat->data[14];
	result->data[12] = mat->data[3];
	result->data[13] = mat->data[7];
	result->data[14] = mat->data[11];
	result->data[15] = mat->data[15];
	return result;
}
