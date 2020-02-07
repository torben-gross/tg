#ifndef TG_MATH_VEC3F
#define TG_MATH_VEC3F

typedef struct tg_vec3f
{
	union
	{
		struct
		{
			float x, y, z;
		};
		struct
		{
			float data[3];
		};
	};
} tg_vec3f;

#endif
