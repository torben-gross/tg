#ifndef TG_MATH_VEC2F
#define TG_MATH_VEC2F

typedef struct tg_vec2f
{
	union
	{
		struct
		{
			float x, y;
		};
		struct
		{
			float data[2];
		};
	};
} tg_vec2f;

#endif
