#ifndef TG_MATH
#define TG_MATH

#include "tg/tg_common.h"

#define TGM_PI 3.14159265358979323846
#define TGM_TO_DEGREES(radians) (radians * (360.0f / ((f32)TGM_PI * 2.0f)))
#define TGM_TO_RADIANS(degrees) (degrees * (((f32)TGM_PI * 2.0f) / 360.0f))

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
			f32 m00;
			f32 m10;
			f32 m20;
			f32 m30;

			f32 m01;
			f32 m11;
			f32 m21;
			f32 m31;
			
			f32 m02;
			f32 m12;
			f32 m22;
			f32 m32;
			
			f32 m03;
			f32 m13;
			f32 m23;
			f32 m33;
		};
		struct
		{
			f32 data[TGM_MAT4F_ELEMENT_COUNT];
		};
	};
} tgm_mat4f;

typedef struct tgm_mat3f
{
	union
	{
		struct
		{
			f32 m00;
			f32 m10;
			f32 m20;

			f32 m01;
			f32 m11;
			f32 m21;
			
			f32 m02;
			f32 m12;
			f32 m22;
		};
		struct
		{
			f32 data[TGM_MAT3F_ELEMENT_COUNT];
		};
	};
} tgm_mat3f;

typedef struct tgm_mat2f
{
	union
	{
		struct
		{
			f32 m00;
			f32 m10;

			f32 m01;
			f32 m11;
		};
		struct
		{
			f32 data[TGM_MAT2F_ELEMENT_COUNT];
		};
	};
} tgm_mat2f;

typedef struct tgm_vec4f
{
	union
	{
		struct
		{
			f32 x, y, z, w;
		};
		struct
		{
			f32 data[TGM_VEC4F_ELEMENT_COUNT];
		};
	};
} tgm_vec4f;

typedef struct tgm_vec3f
{
	union
	{
		struct
		{
			f32 x, y, z;
		};
		struct
		{
			f32 data[TGM_VEC3F_ELEMENT_COUNT];
		};
	};
} tgm_vec3f;

typedef struct tgm_vec2f
{
	union
	{
		struct
		{
			f32 x, y;
		};
		struct
		{
			f32 data[TGM_VEC2F_ELEMENT_COUNT];
		};
	};
} tgm_vec2f;

// functional
f32 tgm_f32_clamp(f32 v, f32 low, f32 high);
f32 tgm_f32_max(f32 v0, f32 v1);
f32 tgm_f32_min(f32 v0, f32 v1);

ui32 tgm_ui32_clamp(ui32 v, ui32 low, ui32 high);
ui32 tgm_ui32_max(ui32 v0, ui32 v1);
ui32 tgm_ui32_min(ui32 v0, ui32 v1);

// vectors
tgm_vec3f* tgm_v3f_add_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1);
tgm_vec3f* tgm_v3f_add_f(tgm_vec3f* result, tgm_vec3f* v0, f32 f);
tgm_vec3f* tgm_v3f_cross(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1);
tgm_vec3f* tgm_v3f_divide_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1);
tgm_vec3f* tgm_v3f_divide_f(tgm_vec3f* result, tgm_vec3f* v0, f32 f);
f32 tgm_v3f_dot(const tgm_vec3f* v0, const tgm_vec3f* v1);
f32 tgm_v3f_magnitude(const tgm_vec3f* v);
f32 tgm_v3f_magnitude_squared(const tgm_vec3f* v);
tgm_vec3f* tgm_v3f_multiply_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1);
tgm_vec3f* tgm_v3f_multiply_f(tgm_vec3f* result, tgm_vec3f* v0, f32 f);
tgm_vec3f* tgm_v3f_negate(tgm_vec3f* result, tgm_vec3f* v0);
tgm_vec3f* tgm_v3f_normalize(tgm_vec3f* result, tgm_vec3f* v0);
tgm_vec3f* tgm_v3f_subtract_v3f(tgm_vec3f* result, tgm_vec3f* v0, tgm_vec3f* v1);
tgm_vec3f* tgm_v3f_subtract_f(tgm_vec3f* result, tgm_vec3f* v0, f32 f);

// matrices
tgm_mat2f* tgm_m2f_identity(tgm_mat2f* result);
tgm_mat2f* tgm_m2f_multiply_m2f(tgm_mat2f* result, tgm_mat2f* m0, tgm_mat2f* m1);
tgm_vec2f* tgm_m2f_multiply_v2f(tgm_vec2f* result, tgm_mat2f* m, tgm_vec2f* v);
tgm_mat2f* tgm_m2f_transpose(tgm_mat2f* result, tgm_mat2f* m);

tgm_mat3f* tgm_m3f_identity(tgm_mat3f* result);
tgm_mat3f* tgm_m3f_multiply_m3f(tgm_mat3f* result, tgm_mat3f* m0, tgm_mat3f* m1);
tgm_vec3f* tgm_m3f_multiply_v3f(tgm_vec3f* result, tgm_mat3f* m, tgm_vec3f* v);
tgm_mat3f* tgm_m3f_orthographic(tgm_mat3f* result, f32 left, f32 right, f32 bottom, f32 top);
tgm_mat3f* tgm_m3f_transpose(tgm_mat3f* result, tgm_mat3f* m);

tgm_mat4f* tgm_m4f_angle_axis(tgm_mat4f* result, f32 angle_in_radians, tgm_vec3f* axis);
tgm_mat4f* tgm_m4f_identity(tgm_mat4f* result);
tgm_mat4f* tgm_m4f_look_at(tgm_mat4f* result, tgm_vec3f* from, tgm_vec3f* to, tgm_vec3f* up);
tgm_mat4f* tgm_m4f_multiply_m4f(tgm_mat4f* result, tgm_mat4f* m0, tgm_mat4f* m1);
tgm_vec4f* tgm_m4f_multiply_v4f(tgm_vec4f* result, tgm_mat4f* m, tgm_vec4f* v);
tgm_mat4f* tgm_m4f_orthographic(tgm_mat4f* result, f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near);
tgm_mat4f* tgm_m4f_perspective(tgm_mat4f* result, f32 fov_y_in_radians, f32 aspect, f32 near, f32 far);
tgm_mat4f* tgm_m4f_translate(tgm_mat4f* result, tgm_vec3f* v);
tgm_mat4f* tgm_m4f_transpose(tgm_mat4f* result, tgm_mat4f* m);

#endif
