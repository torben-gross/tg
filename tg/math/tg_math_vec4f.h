#ifndef TG_MATH_VEC4F
#define TG_MATH_VEC4F

typedef struct tg_vec4f
{
	union
	{
		struct
		{
			float x, y, z, w;
		};
		struct
		{
			float data[4];
		};
	};
} tg_vec4f;

#endif
