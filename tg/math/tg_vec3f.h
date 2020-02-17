#ifndef TG_VEC3F
#define TG_VEC3F

#include "tg_math_defines.h"

typedef struct tg_vec3f
{
	union
	{
		struct
		{
			tg_math_float_t x, y, z;
		};
		struct
		{
			tg_math_float_t data[3];
		};
	};
} tg_vec3f;

tg_vec3f* tg_vec3f_add(tg_vec3f* result, tg_vec3f* v0, tg_vec3f* v1);
tg_vec3f* tg_vec3f_add_f(tg_vec3f* result, tg_vec3f* v0, tg_math_float_t f);
tg_vec3f* tg_vec3f_cross(tg_vec3f* result, tg_vec3f* v0, tg_vec3f* v1);
tg_vec3f* tg_vec3f_divide(tg_vec3f* result, tg_vec3f* v0, tg_vec3f* v1);
tg_vec3f* tg_vec3f_divide_f(tg_vec3f* result, tg_vec3f* v0, tg_math_float_t f);
tg_math_float_t tg_vec3f_dot(const tg_vec3f* v0, const tg_vec3f* v1);
tg_math_float_t tg_vec3f_magnitude(const tg_vec3f* result);
tg_math_float_t tg_vec3f_magnitude_squared(const tg_vec3f* result);
tg_vec3f* tg_vec3f_multiply(tg_vec3f* result, tg_vec3f* v0, tg_vec3f* v1);
tg_vec3f* tg_vec3f_multiply_f(tg_vec3f* result, tg_vec3f* v0, tg_math_float_t f);
tg_vec3f* tg_vec3f_negative(tg_vec3f* result, tg_vec3f* v0);
tg_vec3f* tg_vec3f_normalize(tg_vec3f* result, tg_vec3f* v0);
tg_vec3f* tg_vec3f_subtract(tg_vec3f* result, tg_vec3f* v0, tg_vec3f* v1);
tg_vec3f* tg_vec3f_subtract_f(tg_vec3f* result, tg_vec3f* v0, tg_math_float_t f);

#endif
