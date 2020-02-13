#ifndef TG_VEC3F
#define TG_VEC3F

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

tg_vec3f* tg_vec3f_add(tg_vec3f* result, tg_vec3f* v0, tg_vec3f* v1);
tg_vec3f* tg_vec3f_add_f(tg_vec3f* result, tg_vec3f* v0, float f);
tg_vec3f* tg_vec3f_cross(tg_vec3f* result, tg_vec3f* v0, tg_vec3f* v1);
tg_vec3f* tg_vec3f_divide(tg_vec3f* result, tg_vec3f* v0, tg_vec3f* v1);
tg_vec3f* tg_vec3f_divide_f(tg_vec3f* result, tg_vec3f* v0, float f);
float tg_vec3f_dot(const tg_vec3f* v0, const tg_vec3f* v1);
float tg_vec3f_magnitude(const tg_vec3f* result);
float tg_vec3f_magnitude_squared(const tg_vec3f* result);
tg_vec3f* tg_vec3f_multiply(tg_vec3f* result, tg_vec3f* v0, tg_vec3f* v1);
tg_vec3f* tg_vec3f_multiply_f(tg_vec3f* result, tg_vec3f* v0, float f);
tg_vec3f* tg_vec3f_negative(tg_vec3f* result, tg_vec3f* v0);
tg_vec3f* tg_vec3f_normalize(tg_vec3f* result, tg_vec3f* v0);
tg_vec3f* tg_vec3f_subtract(tg_vec3f* result, tg_vec3f* v0, tg_vec3f* v1);
tg_vec3f* tg_vec3f_subtract_f(tg_vec3f* result, tg_vec3f* v0, float f);

#endif
