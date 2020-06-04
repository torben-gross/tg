/*
Noise uses Simplex Noise.
Random uses Marsaglia's Xorshift.

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

#define TGM_PI                     3.14159265358979323846
#define TGM_TO_DEGREES(radians)    (radians * (360.0f / ((f32)TGM_PI * 2.0f)))
#define TGM_TO_RADIANS(degrees)    (degrees * (((f32)TGM_PI * 2.0f) / 360.0f))
#define TGM_SQRT(v)                tgm_f32_sqrt((f32)(v))



typedef struct v2
{
	union
	{
		struct
		{
			f32    x;
			f32    y;
		};
		f32        p_data[2];

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
		f32        p_data[3];

#ifdef TG_CPU_x64 // TODO: ?
#endif

	};
} v3;

typedef struct v3i
{
	union
	{
		struct
		{
			i32    x;
			i32    y;
			i32    z;
		};
		i32        p_data[3];
	};
} v3i;

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
		f32        p_data[4];

#ifdef TG_CPU_x64
		__m64      simd_m64[2];
		__m128     simd_m128;
#endif

	};
} v4;

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
		f32        p_data[4];

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
		f32        p_data[9];

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
		f32        p_data[16];

#ifdef TG_CPU_x64
		__m64      simd_m64[8];
		__m128     simd_m128[4];
		__m256     simd_m256[2];
		__m512     simd_m512;
#endif

	};
} m4;

typedef struct tg_random
{
	u32    state;
} tg_random;



/*------------------------------------------------------------+
| Noise                                                       |
+------------------------------------------------------------*/

f32    tgm_noise(f32 x, f32 y, f32 z);



/*------------------------------------------------------------+
| Random                                                      |
+------------------------------------------------------------*/

// TODO: Unity uses Marsaglia's Xorshift 128, this is the basic variation.
void    tgm_random_init(tg_random* p_random, u32 seed);
f32     tgm_random_next_f32(tg_random* p_random);
u32     tgm_random_next_u32(tg_random* p_random);



/*------------------------------------------------------------+
| Intrinsics                                                  |
+------------------------------------------------------------*/

f32    tgm_f32_acos(f32 v);
f32    tgm_f32_acosh(f32 v);
f32    tgm_f32_asin(f32 v);
f32    tgm_f32_asinh(f32 v);
f32    tgm_f32_atan(f32 v);
f32    tgm_f32_atanh(f32 v);
f32    tgm_f32_ceil(f32 v);
f32    tgm_f32_cos(f32 v);
f32    tgm_f32_cosh(f32 v);
f32    tgm_f32_floor(f32 v);
f32    tgm_f32_log10(f32 v);
f32    tgm_f32_log2(f32 v);
f32    tgm_f32_pow(f32 base, f32 exponent);
f32    tgm_f32_sin(f32 v);
f32    tgm_f32_sinh(f32 v);
f32    tgm_f32_sqrt(f32 v);
f32    tgm_f32_tan(f32 v);
f32    tgm_f32_tanh(f32 v);

f64    tgm_f64_pow(f64 base, f64 exponent);

f32    tgm_i32_log10(i32 v);
f32    tgm_i32_log2(i32 v);
i32    tgm_i32_pow(i32 base, i32 exponent);

f32    tgm_u32_log10(u32 v);
f32    tgm_u32_log2(u32 v);
u32    tgm_u32_pow(u32 base, u32 exponent);



/*------------------------------------------------------------+
| Functional                                                  |
+------------------------------------------------------------*/

f32    tgm_f32_abs(f32 v);
f32    tgm_f32_blerp(f32 v00, f32 v01, f32 v10, f32 v11, f32 tx, f32 ty);
f32    tgm_f32_clamp(f32 v, f32 low, f32 high);
f32    tgm_f32_lerp(f32 v0, f32 v1, f32 t);
f32    tgm_f32_max(f32 v0, f32 v1);
f32    tgm_f32_min(f32 v0, f32 v1);
i32    tgm_f32_round_to_i32(f32 v);
f32    tgm_f32_tlerp(f32 v000, f32 v001, f32 v010, f32 v011, f32 v100, f32 v101, f32 v110, f32 v111, f32 tx, f32 ty, f32 tz);

i32    tgm_i32_abs(i32 v);
i32    tgm_i32_clamp(i32 v, i32 low, i32 high);
u32    tgm_i32_digits(i32 v);
b32    tgm_i32_is_power_of_two(i32 v);
i32    tgm_i32_max(i32 v0, i32 v1);
i32    tgm_i32_min(i32 v0, i32 v1);

u8     tgm_u8_max(u8 v0, u8 v1);
u8     tgm_u8_min(u8 v0, u8 v1);

u32    tgm_u32_clamp(u32 v, u32 low, u32 high);
u32    tgm_u32_digits(u32 v);
u32    tgm_u32_incmod(u32 v, u32 mod);
b32    tgm_u32_is_power_of_two(u32 v);
u32    tgm_u32_max(u32 v0, u32 v1);
u32    tgm_u32_min(u32 v0, u32 v1);

u64    tgm_u64_max(u64 v0, u64 v1);
u64    tgm_u64_min(u64 v0, u64 v1);



/*------------------------------------------------------------+
| Vectors                                                     |
+------------------------------------------------------------*/

v2     tgm_v2_max(v2 v0, v2 v1);
v2     tgm_v2_min(v2 v0, v2 v1);
v2     tgm_v2_sub(v2 v0, v2 v1);

v3     tgm_v3_abs(v3 v);
v3     tgm_v3_add(v3 v0, v3 v1);
v3     tgm_v3_addf(v3 v, f32 f);
v3     tgm_v3_clamp(v3 v, v3 min, v3 max);
v3     tgm_v3_cross(v3 v0, v3 v1);
v3     tgm_v3_div(v3 v0, v3 v1);
v3     tgm_v3_divf(v3 v, f32 f);
f32    tgm_v3_dot(v3 v0, v3 v1);
b32    tgm_v3_equal(v3 v0, v3 v1);
v3     tgm_v3_lerp(v3 v0, v3 v1, f32 t);
f32    tgm_v3_mag(v3 v);
f32    tgm_v3_magsqr(v3 v);
v3     tgm_v3_max(v3 v0, v3 v1);
v3     tgm_v3_min(v3 v0, v3 v1);
v3     tgm_v3_mul(v3 v0, v3 v1);
v3     tgm_v3_mulf(v3 v, f32 f);
v3     tgm_v3_neg(v3 v);
v3     tgm_v3_normalized(v3 v);
b32    tgm_v3_similar(v3 v0, v3 v1, f32 epsilon);
v3     tgm_v3_sub(v3 v0, v3 v1);
v3     tgm_v3_subf(v3 v, f32 f);
v4     tgm_v3_to_v4(v3 v, f32 w);

v3i    tgm_v3i_abs(v3i v);
b32    tgm_v3i_equal(v3i v0, v3i v1);
f32    tgm_v3i_mag(v3i v);
i32    tgm_v3i_magsqr(v3i v);
v3i    tgm_v3i_sub(v3i v0, v3i v1);
v3i    tgm_v3i_subi(v3i v, i32 i);

v4     tgm_v4_add(v4 v0, v4 v1);
v4     tgm_v4_addf(v4 v, f32 f);
v4     tgm_v4_div(v4 v0, v4 v1);
v4     tgm_v4_divf(v4 v, f32 f);
f32    tgm_v4_dot(v4 v0, v4 v1);
b32    tgm_v4_equal(v4 v0, v4 v1);
f32    tgm_v4_mag(v4 v);
f32    tgm_v4_magsqr(v4 v);
v4     tgm_v4_max(v4 v0, v4 v1);
v4     tgm_v4_min(v4 v0, v4 v1);
v4     tgm_v4_mul(v4 v0, v4 v1);
v4     tgm_v4_mulf(v4 v, f32 f);
v4     tgm_v4_neg(v4 v);
v4     tgm_v4_normalized(v4 v);
v4     tgm_v4_sub(v4 v0, v4 v1);
v4     tgm_v4_subf(v4 v, f32 f);
v3     tgm_v4_to_v3(v4 v);



/*------------------------------------------------------------+
| Matrices                                                    |
+------------------------------------------------------------*/

m2     tgm_m2_identity();
m2     tgm_m2_multiply(m2 m0, m2 m1);
v2     tgm_m2_multiply_v2(m2 m, v2 v);
m2     tgm_m2_transposed(m2 m);

m3     tgm_m3_identity();
m3     tgm_m3_mul(m3 m0, m3 m1);
v3     tgm_m3_mulv3(m3 m, v3 v);
m3     tgm_m3_orthographic(f32 left, f32 right, f32 bottom, f32 top);
m3     tgm_m3_transposed(m3 m);

m4     tgm_m4_angle_axis(f32 angle_in_radians, v3 axis);
f32    tgm_m4_determinant(m4 m);
m4     tgm_m4_euler(f32 pitch_in_radians, f32 yaw_in_radians, f32 roll_in_radians);
m4     tgm_m4_identity();
m4     tgm_m4_inverse(m4 m);
m4     tgm_m4_look_at(v3 from, v3 to, v3 up);
m4     tgm_m4_mul(m4 m0, m4 m1);
v4     tgm_m4_mulv4(m4 m, v4 v);
m4     tgm_m4_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near);
m4     tgm_m4_perspective(f32 fov_y_in_radians, f32 aspect, f32 near, f32 far);
m4     tgm_m4_rotate_x(f32 angle_in_radians);
m4     tgm_m4_rotate_y(f32 angle_in_radians);
m4     tgm_m4_rotate_z(f32 angle_in_radians);
m4     tgm_m4_translate(v3 v);
m4     tgm_m4_transposed(m4 m);

#endif
