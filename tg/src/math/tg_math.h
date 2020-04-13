/*
Matrices are layed out in column major.

Projection matrices are currently layed out for following clipping setup:
	Left: -1, right: 1, bottom: 1, top: -1, near: 0, far: 1

TODO: Support other clipping setups via macros!
*/

#ifndef TG_MATH
#define TG_MATH

#include "tg_common.h"

#ifdef TG_CPU_x64
#include <immintrin.h>
#endif

#define TGM_PI                      3.14159265358979323846
#define TGM_TO_DEGREES(radians)     (radians * (360.0f / ((f32)TGM_PI * 2.0f)))
#define TGM_TO_RADIANS(degrees)     (degrees * (((f32)TGM_PI * 2.0f) / 360.0f))



/*
+-       -+
| m00 m01 |
| m10 m11 |
+-       -+
*/
typedef struct m2
{
	union
	{
		struct
		{
			f32    m00;
			f32    m10;

			f32    m01;
			f32    m11;
		};
		f32        data[4];

#ifdef TG_CPU_x64
		__m64      simd_m64[2];
		__m128     simd_m128;
#endif

	};
} m2;

/*
+-           -+
| m00 m01 m02 |
| m10 m11 m12 |
| m20 m21 m22 |
+-           -+
*/
typedef struct m3
{
	union
	{
		struct
		{
			f32    m00;
			f32    m10;
			f32    m20;

			f32    m01;
			f32    m11;
			f32    m21;

			f32    m02;
			f32    m12;
			f32    m22;
		};
		f32        data[9];

#ifdef TG_CPU_x64 // TODO: ?
#endif

	};
} m3;

/*
+-               -+
| m00 m01 m02 m03 |
| m10 m11 m12 m13 |
| m20 m21 m22 m23 |
| m30 m31 m32 m33 |
+-               -+
*/
typedef struct m4
{
	union
	{
		struct
		{
			f32    m00;
			f32    m10;
			f32    m20;
			f32    m30;

			f32    m01;
			f32    m11;
			f32    m21;
			f32    m31;
			
			f32    m02;
			f32    m12;
			f32    m22;
			f32    m32;
			
			f32    m03;
			f32    m13;
			f32    m23;
			f32    m33;
		};
		f32        data[16];

#ifdef TG_CPU_x64
		__m64      simd_m64[8];
		__m128     simd_m128[4];
		__m256     simd_m256[2];
		__m512     simd_m512;
#endif

	};
} m4;

typedef struct v2
{
	union
	{
		struct
		{
			f32    x;
			f32    y;
		};
		f32        data[2];

#ifdef TG_CPU_x64
		__m64      simd_m64;
#endif

	};
} v2;

typedef struct v3
{
	union
	{
		struct
		{
			f32    x;
			f32    y;
			f32    z;
		};
		f32        data[3];

#ifdef TG_CPU_x64 // TODO: ?
#endif

	};
} v3;

typedef struct v4
{
	union
	{
		struct
		{
			f32    x;
			f32    y;
			f32    z;
			f32    w;
		};
		f32        data[4];

#ifdef TG_CPU_x64
		__m64      simd_m64[2];
		__m128     simd_m128;
#endif

	};
} v4;



/*------------------------------------------------------------+
| Random                                                      |
+------------------------------------------------------------*/

TG_DECLARE_HANDLE(tgm_random_lcg);

tgm_random_lcg_h         tgm_random_lcg_create(u32 seed);
f32                      tgm_random_lcg_next_f32(tgm_random_lcg_h random_lcg_h);
u32                      tgm_random_lcg_next_ui32(tgm_random_lcg_h random_lcg_h);
void                     tgm_random_lcg_destroy(tgm_random_lcg_h random_lcg_h);



TG_DECLARE_HANDLE(tgm_random_xorshift); // TODO: Unity uses Marsaglia's Xorshift 128, this is the basic variation.

tgm_random_xorshift_h    tgm_random_xorshift_create(u32 seed);
f32                      tgm_random_xorshift_next_f32(tgm_random_xorshift_h random_xorshift_h);
u32                      tgm_random_xorshift_next_ui32(tgm_random_xorshift_h random_xorshift_h);
void                     tgm_random_xorshift_destroy(tgm_random_xorshift_h random_xorshift_h);



/*------------------------------------------------------------+
| Intrinsics                                                  |
+------------------------------------------------------------*/

f32    tgm_f32_arccos(f32 v);
f32    tgm_f32_arccosh(f32 v);
f32    tgm_f32_arcsin(f32 v);
f32    tgm_f32_arcsinh(f32 v);
f32    tgm_f32_arctan(f32 v);
f32    tgm_f32_arctanh(f32 v);
f32    tgm_f32_cos(f32 v);
f32    tgm_f32_cosh(f32 v);
f32    tgm_f32_floor(f32 v);
f32    tgm_f32_log2(f32 v);
f32    tgm_f32_sin(f32 v);
f32    tgm_f32_sinh(f32 v);
f32    tgm_f32_sqrt(f32 v);
f32    tgm_f32_tan(f32 v);
f32    tgm_f32_tanh(f32 v);

f64    tgm_f64_pow(f64 base, f64 exponent);

i32    tgm_i32_abs(i32 v);
f32    tgm_i32_log10(i32 v);
i32    tgm_i32_pow(i32 base, i32 exponent);

u32    tgm_ui32_floor(u32 v);
f32    tgm_ui32_log10(u32 v);
u32    tgm_ui32_pow(u32 base, u32 exponent);



/*------------------------------------------------------------+
| Functional                                                  |
+------------------------------------------------------------*/

f32    tgm_f32_clamp(f32 v, f32 low, f32 high);
f32    tgm_f32_max(f32 v0, f32 v1);
f32    tgm_f32_min(f32 v0, f32 v1);

i32    tgm_i32_clamp(i32 v, i32 low, i32 high);
u32    tgm_i32_digits(i32 v);
i32    tgm_i32_max(i32 v0, i32 v1);
i32    tgm_i32_min(i32 v0, i32 v1);

u32    tgm_u32_clamp(u32 v, u32 low, u32 high);
u32    tgm_u32_digits(u32 v);
u32    tgm_u32_max(u32 v0, u32 v1);
u32    tgm_u32_min(u32 v0, u32 v1);

u64    tgm_u64_max(u64 v0, u64 v1);
u64    tgm_u64_min(u64 v0, u64 v1);



/*------------------------------------------------------------+
| Vectors                                                     |
+------------------------------------------------------------*/

v2     tgm_v2_subtract_v2(const v2* p_v0, const v2* p_v1);

v3     tgm_v3_add_v3(const v3* p_v0, const v3* p_v1);
v3     tgm_v3_add_f(const v3* p_v, f32 f);
v3     tgm_v3_cross(const v3* p_v0, const v3* p_v1);
v3     tgm_v3_divide_v3(const v3* p_v0, const v3* p_v1);
v3     tgm_v3_divide_f(const v3* p_v, f32 f);
f32    tgm_v3_dot(const v3* p_v0, const v3* p_v1);
b32    tgm_v3_equal(const v3* p_v0, const v3* p_v1);
f32    tgm_v3_magnitude(const v3* p_v);
f32    tgm_v3_magnitude_squared(const v3* p_v);
v3     tgm_v3_multiply_v3(const v3* p_v0, const v3* p_v1);
v3     tgm_v3_multiply_f(const v3* p_v, f32 f);
v3     tgm_v3_negated(const v3* p_v);
v3     tgm_v3_normalized(const v3* p_v);
v3     tgm_v3_subtract_v3(const v3* p_v0, const v3* p_v1);
v3     tgm_v3_subtract_f(const v3* p_v, f32 f);
v4     tgm_v3_to_v4(const v3* p_v, f32 w);

v4     tgm_v4_add_v4(const v4* p_v0, const v4* p_v1);
v4     tgm_v4_add_f(const v4* p_v, f32 f);
v4     tgm_v4_divide_v4(const v4* p_v0, const v4* p_v1);
v4     tgm_v4_divide_f(const v4* p_v, f32 f);
f32    tgm_v4_dot(const v4* p_v0, const v4* p_v1);
b32    tgm_v4_equal(const v4* p_v0, const v4* p_v1);
f32    tgm_v4_magnitude(const v4* p_v);
f32    tgm_v4_magnitude_squared(const v4* p_v);
v4     tgm_v4_multiply_v4(const v4* p_v0, const v4* p_v1);
v4     tgm_v4_multiply_f(const v4* p_v, f32 f);
v4     tgm_v4_negated(const v4* p_v);
v4     tgm_v4_normalized(const v4* p_v);
v4     tgm_v4_subtract_v4(const v4* p_v0, const v4* p_v1);
v4     tgm_v4_subtract_f(const v4* p_v, f32 f);
v3     tgm_v4_to_v3(const v4* p_v);



/*------------------------------------------------------------+
| Matrices                                                    |
+------------------------------------------------------------*/

m2     tgm_m2_identity();
m2     tgm_m2_multiply_m2(const m2* p_m0, const m2* p_m1);
v2     tgm_m2_multiply_v2(const m2* p_m, const v2* p_v);
m2     tgm_m2_transposed(const m2* p_m);

m3     tgm_m3_identity();
m3     tgm_m3_multiply_m3(const m3* p_m0, const m3* p_m1);
v3     tgm_m3_multiply_v3(const m3* p_m, const v3* p_v);
m3     tgm_m3_orthographic(f32 left, f32 right, f32 bottom, f32 top);
m3     tgm_m3_transposed(const m3* p_m);

m4     tgm_m4_angle_axis(f32 angle_in_radians, const v3* p_axis);
f32    tgm_m4_determinant(const m4* p_m);
m4     tgm_m4_euler(f32 pitch_in_radians, f32 yaw_in_radians, f32 roll_in_radians);
m4     tgm_m4_identity();
m4     tgm_m4_inverse(const m4* p_m);
m4     tgm_m4_look_at(const v3* p_from, const v3* p_to, const v3* p_up);
m4     tgm_m4_multiply_m4(const m4* p_m0, const m4* p_m1);
v4     tgm_m4_multiply_v4(const m4* p_m, const v4* p_v);
m4     tgm_m4_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near);
m4     tgm_m4_perspective(f32 fov_y_in_radians, f32 aspect, f32 near, f32 far);
m4     tgm_m4_rotate_x(f32 angle_in_radians);
m4     tgm_m4_rotate_y(f32 angle_in_radians);
m4     tgm_m4_rotate_z(f32 angle_in_radians);
m4     tgm_m4_translate(const v3* p_v);
m4     tgm_m4_transposed(const m4* p_m);

#endif
