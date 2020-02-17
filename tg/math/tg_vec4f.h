#ifndef TG_VEC4F
#define TG_VEC4F

#include "tg_math_defines.h"

typedef struct tg_vec4f
{
	union
	{
		struct
		{
			tg_math_float_t x, y, z, w;
		};
		struct
		{
			tg_math_float_t data[4];
		};
	};
} tg_vec4f;

#endif
