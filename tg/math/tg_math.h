#ifndef TG_MATH
#define TG_MATH

#include "tg/tg_common.h"

#define TGM_PI 3.14159265358979323846
#define TGM_TO_DEGREES(radians) (radians * (360.0f / ((float32)TGM_PI * 2.0f)))
#define TGM_TO_RADIANS(degrees) (degrees * (((float32)TGM_PI * 2.0f) / 360.0f))

#define TGM_MAT4F_ELEMENT_COUNT 16
#define TGM_MAT4F_ROW_COUNT      4
#define TGM_MAT4F_COLUMN_COUNT   4
#define TGM_MAT3F_ELEMENT_COUNT  9
#define TGM_MAT3F_ROW_COUNT      3
#define TGM_MAT3F_COLUMN_COUNT   3
#define TGM_MAT2F_ELEMENT_COUNT  4
#define TGM_MAT2F_ROW_COUNT      2
#define TGM_MAT2F_COLUMN_COUNT   2

#define TGM_VEC4F_ELEMENT_COUNT  4
#define TGM_VEC3F_ELEMENT_COUNT  3
#define TGM_VEC2F_ELEMENT_COUNT  2

typedef struct tgm_mat4f
{
	union
	{
		struct
		{
			float32 m00;
			float32 m10;
			float32 m20;
			float32 m30;

			float32 m01;
			float32 m11;
			float32 m21;
			float32 m31;
			
			float32 m02;
			float32 m12;
			float32 m22;
			float32 m32;
			
			float32 m03;
			float32 m13;
			float32 m23;
			float32 m33;
		};
		struct
		{
			float32 data[TGM_MAT4F_ELEMENT_COUNT];
		};
	};
} tgm_mat4f;

typedef struct tgm_mat3f
{
	union
	{
		struct
		{
			float32 m00;
			float32 m10;
			float32 m20;

			float32 m01;
			float32 m11;
			float32 m21;
			
			float32 m02;
			float32 m12;
			float32 m22;
		};
		struct
		{
			float32 data[TGM_MAT3F_ELEMENT_COUNT];
		};
	};
} tgm_mat3f;

typedef struct tgm_mat2f
{
	union
	{
		struct
		{
			float32 m00;
			float32 m10;

			float32 m01;
			float32 m11;
		};
		struct
		{
			float32 data[TGM_MAT2F_ELEMENT_COUNT];
		};
	};
} tgm_mat2f;

typedef struct tgm_vec4f
{
	union
	{
		struct
		{
			float32 x, y, z, w;
		};
		struct
		{
			float32 data[TGM_VEC4F_ELEMENT_COUNT];
		};
	};
} tgm_vec4f;

typedef struct tgm_vec3f
{
	union
	{
		struct
		{
			float32 x, y, z;
		};
		struct
		{
			float32 data[TGM_VEC3F_ELEMENT_COUNT];
		};
	};
} tgm_vec3f;

typedef struct tgm_vec2f
{
	union
	{
		struct
		{
			float32 x, y;
		};
		struct
		{
			float32 data[TGM_VEC2F_ELEMENT_COUNT];
		};
	};
} tgm_vec2f;

// functional
float32 tgm_f32_clamp(float32 v, float32 low, float32 high);
float32 tgm_f32_max(float32 v0, float32 v1);
float32 tgm_f32_min(float32 v0, float32 v1);

uint32 tgm_ui32_clamp(uint32 v, uint32 low, uint32 high);
uint32 tgm_ui32_max(uint32 v0, uint32 v1);
uint32 tgm_ui32_min(uint32 v0, uint32 v1);

// vectors
tgm_vec3f* tgm_v3f_add_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1);
tgm_vec3f* tgm_v3f_add_f(tgm_vec3f* result, tgm_vec3f* v0, float32 f);
tgm_vec3f* tgm_v3f_cross(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1);
tgm_vec3f* tgm_v3f_divide_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1);
tgm_vec3f* tgm_v3f_divide_f(tgm_vec3f* result, tgm_vec3f* v0, float32 f);
float32 tgm_v3f_dot(const tgm_vec3f* v0, const tgm_vec3f* v1);
float32 tgm_v3f_magnitude(const tgm_vec3f* v);
float32 tgm_v3f_magnitude_squared(const tgm_vec3f* v);
tgm_vec3f* tgm_v3f_multiply_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1);
tgm_vec3f* tgm_v3f_multiply_f(tgm_vec3f* result, tgm_vec3f* v0, float32 f);
tgm_vec3f* tgm_v3f_negative(tgm_vec3f* result, tgm_vec3f* v0);
tgm_vec3f* tgm_v3f_normalize(tgm_vec3f* result, tgm_vec3f* v0);
tgm_vec3f* tgm_v3f_subtract_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1);
tgm_vec3f* tgm_v3f_subtract_f(tgm_vec3f* result, tgm_vec3f* v0, float32 f);

// matrices
tgm_mat2f* tgm_m2f_identity(tgm_mat2f* result);
tgm_mat2f* tgm_m2f_multiply_m2f(tgm_mat2f* result, tgm_mat2f* m0, tgm_mat2f* m1);
tgm_vec2f* tgm_m2f_multiply_v2f(tgm_vec2f* result, tgm_mat2f* m, tgm_vec2f* v);
tgm_mat2f* tgm_m2f_transpose(tgm_mat2f* result, tgm_mat2f* m);

tgm_mat3f* tgm_m3f_identity(tgm_mat3f* result);
tgm_mat3f* tgm_m3f_multiply_m3f(tgm_mat3f* result, tgm_mat3f* m0, tgm_mat3f* m1);
tgm_vec3f* tgm_m3f_multiply_v3f(tgm_vec3f* result, tgm_mat3f* m, tgm_vec3f* v);
tgm_mat3f* tgm_m3f_orthographic(tgm_mat3f* result, float32 left, float32 right, float32 bottom, float32 top);
tgm_mat3f* tgm_m3f_transpose(tgm_mat3f* result, tgm_mat3f* m);

tgm_mat4f* tgm_m4f_angle_axis(tgm_mat4f* result, float32 angle_in_radians, tgm_vec3f* axis);
tgm_mat4f* tgm_m4f_identity(tgm_mat4f* result);
tgm_mat4f* tgm_m4f_look_at(tgm_mat4f* result, tgm_vec3f* from, tgm_vec3f* to, tgm_vec3f* up);
tgm_mat4f* tgm_m4f_multiply_m4f(tgm_mat4f* result, tgm_mat4f* m0, tgm_mat4f* m1);
tgm_vec4f* tgm_m4f_multiply_v4f(tgm_vec4f* result, tgm_mat4f* m, tgm_vec4f* v);
tgm_mat4f* tgm_m4f_orthographic(tgm_mat4f* result, float32 left, float32 right, float32 bottom, float32 top, float32 far, float32 near);
tgm_mat4f* tgm_m4f_perspective(tgm_mat4f* result, float32 fov_y_in_radians, float32 aspect, float32 near, float32 far);
tgm_mat4f* tgm_m4f_translate(tgm_mat4f* result, tgm_vec3f* v);
tgm_mat4f* tgm_m4f_transpose(tgm_mat4f* result, tgm_mat4f* m);

#endif
