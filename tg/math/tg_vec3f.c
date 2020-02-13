#include "tg_vec3f.h"

#include <math.h>

tg_vec3f* tg_vec3f_add(tg_vec3f* result, tg_vec3f* v0, tg_vec3f* v1)
{
	result->x = v0->x + v1->x;
	result->y = v0->y + v1->y;
	result->z = v0->z + v1->z;
	return result;
}

tg_vec3f* tg_vec3f_add_f(tg_vec3f* result, tg_vec3f* v0, float f)
{
	result->x = v0->x + f;
	result->y = v0->y + f;
	result->z = v0->z + f;
	return result;
}

tg_vec3f* tg_vec3f_cross(tg_vec3f* result, tg_vec3f* v0, tg_vec3f* v1)
{
	result->x = v0->y * v1->z - v0->z * v1->y;
	result->y = v0->z * v1->x - v0->x * v1->z;
	result->z = v0->x * v1->y - v0->y * v1->x;
	return result;
}

tg_vec3f* tg_vec3f_divide(tg_vec3f* result, tg_vec3f* v0, tg_vec3f* v1)
{
	result->x = v0->x / v1->x;
	result->y = v0->y / v1->y;
	result->z = v0->z / v1->z;
	return result;
}

tg_vec3f* tg_vec3f_divide_f(tg_vec3f* result, tg_vec3f* v0, float f)
{
	result->x = v0->x / f;
	result->y = v0->y / f;
	result->z = v0->z / f;
	return result;
}

float tg_vec3f_dot(const tg_vec3f* v0, const tg_vec3f* v1)
{
	return v0->x * v1->x + v0->y * v1->y + v0->z * v1->z;
}

float tg_vec3f_magnitude(const tg_vec3f* v0)
{
	return sqrtf(v0->x * v0->x + v0->y * v0->y + v0->z * v0->z);
}

float tg_vec3f_magnitude_squared(const tg_vec3f* v0)
{
	return v0->x * v0->x + v0->y * v0->y + v0->z * v0->z;
}

tg_vec3f* tg_vec3f_multiply(tg_vec3f* result, tg_vec3f* v0, tg_vec3f* v1)
{
	result->x = v0->x * v1->x;
	result->y = v0->y * v1->y;
	result->z = v0->z * v1->z;
	return result;
}

tg_vec3f* tg_vec3f_multiply_f(tg_vec3f* result, tg_vec3f* v0, float f)
{
	result->x = v0->x * f;
	result->y = v0->y * f;
	result->z = v0->z * f;
	return result;
}

tg_vec3f* tg_vec3f_negative(tg_vec3f* result, tg_vec3f* v0)
{
	result->x = -v0->x;
	result->y = -v0->y;
	result->z = -v0->z;
	return result;
}

tg_vec3f* tg_vec3f_normalize(tg_vec3f* result, tg_vec3f* v0)
{
	const float magnitude = tg_vec3f_magnitude(v0);
	result->x = v0->x / magnitude;
	result->y = v0->y / magnitude;
	result->z = v0->z / magnitude;
	return result;
}

tg_vec3f* tg_vec3f_subtract(tg_vec3f* result, tg_vec3f* v0, tg_vec3f* v1)
{
	result->x = v0->x - v1->x;
	result->y = v0->y - v1->y;
	result->z = v0->z - v1->z;
	return result;
}

tg_vec3f* tg_vec3f_subtract_f(tg_vec3f* result, tg_vec3f* v0, float f)
{
	result->x = v0->x - f;
	result->y = v0->y - f;
	result->z = v0->z - f;
	return result;
}
