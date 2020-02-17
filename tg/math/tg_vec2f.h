#ifndef TG_VEC2F
#define TG_VEC2F

#include "tg_math_defines.h"

typedef struct tg_vec2f
{
	union
	{
		struct
		{
			tg_math_float_t x, y;
		};
		struct
		{
			tg_math_float_t data[2];
		};
	};
} tg_vec2f;

#endif
