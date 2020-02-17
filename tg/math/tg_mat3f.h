#ifndef TG_MAT3F
#define TG_MAT3F

#include "tg_math_defines.h"

#ifdef TG_MATH_USE_ROW_MAJOR
#define TG_MAT3F_AT(m, row, col) m->data[TG_MAT3F_ROWS * row + col]
#elif defined(TG_MATH_USE_COLUMN_MAJOR)
#define TG_MAT3F_AT(m, row, col) m->data[row + TG_MAT3F_COLUMNS * col]
#endif

typedef struct tg_mat3f
{
	union
	{
		struct
		{
#ifdef TG_MATH_USE_ROW_MAJOR
			tg_math_float_t m00;
			tg_math_float_t m01;
			tg_math_float_t m02;
			tg_math_float_t m10;
			tg_math_float_t m11;
			tg_math_float_t m12;
			tg_math_float_t m20;
			tg_math_float_t m21;
			tg_math_float_t m22;
#elif defined(TG_MATH_USE_COLUMN_MAJOR)
			tg_math_float_t m00;
			tg_math_float_t m10;
			tg_math_float_t m20;
			tg_math_float_t m01;
			tg_math_float_t m11;
			tg_math_float_t m21;
			tg_math_float_t m02;
			tg_math_float_t m12;
			tg_math_float_t m22;
#endif
		};
		struct
		{
			tg_math_float_t data[TG_MAT3F_SIZE];
		};
	};
} tg_mat3f;

#endif
