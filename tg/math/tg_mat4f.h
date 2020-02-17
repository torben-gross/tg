#ifndef TG_MAT4F
#define TG_MAT4F

#include "tg_math_defines.h"
#include "tg_vec3f.h"
#include "tg_vec4f.h"

#ifdef TG_MATH_USE_ROW_MAJOR
#define TG_MAT4F_AT(m, row, col) m->data[TG_MAT4F_ROWS * row + col]
#elif defined(TG_MATH_USE_COLUMN_MAJOR)
#define TG_MAT4F_AT(m, row, col) m->data[row + TG_MAT4F_COLUMNS * col]
#endif

typedef struct tg_mat4f
{
	union
	{
		struct
		{
#ifdef TG_MATH_USE_ROW_MAJOR
			tg_math_float_t m00;
			tg_math_float_t m01;
			tg_math_float_t m02;
			tg_math_float_t m03;
			tg_math_float_t m10;
			tg_math_float_t m11;
			tg_math_float_t m12;
			tg_math_float_t m13;
			tg_math_float_t m20;
			tg_math_float_t m21;
			tg_math_float_t m22;
			tg_math_float_t m23;
			tg_math_float_t m30;
			tg_math_float_t m31;
			tg_math_float_t m32;
			tg_math_float_t m33;
#elif defined(TG_MATH_USE_COLUMN_MAJOR)
			tg_math_float_t m00;
			tg_math_float_t m10;
			tg_math_float_t m20;
			tg_math_float_t m30;
			tg_math_float_t m01;
			tg_math_float_t m11;
			tg_math_float_t m21;
			tg_math_float_t m31;
			tg_math_float_t m02;
			tg_math_float_t m12;
			tg_math_float_t m22;
			tg_math_float_t m32;
			tg_math_float_t m03;
			tg_math_float_t m13;
			tg_math_float_t m23;
			tg_math_float_t m33;
#endif
		};
		struct
		{
			tg_math_float_t data[TG_MAT4F_SIZE];
		};
	};
} tg_mat4f;

tg_mat4f* tg_mat4f_angle_axis(tg_mat4f* result, tg_math_float_t angle_in_radians, tg_vec3f* axis);
tg_mat4f* tg_mat4f_identity(tg_mat4f* result);
tg_mat4f* tg_mat4f_look_at(tg_mat4f* result, tg_vec3f* from, tg_vec3f* to, tg_vec3f* up);
tg_mat4f* tg_mat4f_multiply(tg_mat4f* result, tg_mat4f* m0, tg_mat4f* m1);
tg_vec4f* tg_mat4f_multiply_v4(tg_vec4f* result, tg_mat4f* m, tg_vec4f* v);
tg_mat4f* tg_mat4f_orthographic(tg_mat4f* result, tg_math_float_t left, tg_math_float_t right, tg_math_float_t bottom, tg_math_float_t top, tg_math_float_t near, tg_math_float_t far);
tg_mat4f* tg_mat4f_perspective(tg_mat4f* result, tg_math_float_t fov_in_radians, tg_math_float_t aspect, tg_math_float_t near, tg_math_float_t far);
tg_mat4f* tg_mat4f_transpose(tg_mat4f* result, tg_mat4f* m);

#endif
