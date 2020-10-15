#ifndef TG_MATH
#define TG_MATH

#include "tg_common.h"



#define TG_PI                                      3.14159274f
#define TG_GOLDEN_RATIO                            1.61803401f
#define TG_CEIL_TO_MULTIPLE(value, multiple_of)    ((((value) + (multiple_of) - 1) / (multiple_of)) * (multiple_of))
#define TG_MAX(v0, v1)                             ((v0) > (v1) ? (v0) : (v1))
#define TG_MIN(v0, v1)                             ((v0) < (v1) ? (v0) : (v1))
#define TG_TO_DEGREES(radians)                     (radians * (360.0f / (TG_PI * 2.0f)))
#define TG_TO_RADIANS(degrees)                     (degrees * ((TG_PI * 2.0f) / 360.0f))

#define V2(f)                                      ((v2) { (f), (f) })
#define V2I(i)                                     ((v2i) { (i), (i) })
#define V3(f)                                      ((v3) { (f), (f), (f) })
#define V3I(i)                                     ((v3i) { (i), (i), (i) })
#define V4(f)                                      ((v4) { (f), (f), (f), (f) })



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
	};
} v2;

typedef struct v2i
{
	union
	{
		struct
		{
			i32    x;
			i32    y;
		};
		i32        p_data[2];
	};
} v2i;

typedef struct v3
{
	union
	{
		struct
		{
			f32            x;
			union
			{
				struct
				{
					f32    y;
					f32    z;
				};
				v2         yz;
			};
		};
		v2                 xy;
		f32                p_data[3];
	};
} v3;

typedef struct v3i
{
	union
	{
		struct
		{
			i32            x;
			union
			{
				struct
				{
					i32    y;
					i32    z;
				};
				v2i        yz;
			};
		};
		v2i                xy;
		i32                p_data[3];
	};
} v3i;

typedef struct v4
{
	union
	{
		struct
		{
			f32                    x;
			union
			{
				struct
				{
					f32            y;
					union
					{
						struct
						{
							f32    z;
							f32    w;
						};
						v2         zw;
					};
				};
				v3                 yz;
				v3                 yzw;
			};
		};
		v3                         xyz;
		v2                         xy;
		f32                        p_data[4];
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
	union m2_u
	{
		struct
		{
			f32    m00;
			f32    m10;

			f32    m01;
			f32    m11;
		};
		struct
		{
			v2     col0;
			v2     col1;
		};
		f32        p_data[4];
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
	union m3_u
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
		struct
		{
			v3     col0;
			v3     col1;
			v3     col2;
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
	union m4_u
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
		struct
		{
			v4     col0;
			v4     col1;
			v4     col2;
			v4     col3;
		};
		f32        p_data[16];
	};
} m4;

typedef struct tg_random
{
	u32    state;
} tg_random;



/*------------------------------------------------------------+
| Miscellaneous                                               |
+------------------------------------------------------------*/

f32          tgm_noise(f32 x, f32 y, f32 z); // simplex noise
tg_random    tgm_random_init(u32 seed); // TODO: Unity uses Marsaglia's Xorshift 128, this is the basic variation
f32          tgm_random_next_f32(tg_random* p_random);
f32          tgm_random_next_f32_between(tg_random* p_random, f32 low, f32 high);
u32          tgm_random_next_u32(tg_random* p_random);
void         tgm_enclosing_sphere(u32 contained_point_count, const v3* p_contained_points, v3* p_center, f32* p_radius);



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

f64    tgm_f64_cos(f32 v);
f64    tgm_f64_pow(f64 base, f64 exponent);

f32    tgm_i32_log10(i32 v);
f32    tgm_i32_log2(i32 v);
i32    tgm_i32_pow(i32 base, i32 exponent);

f32    tgm_u32_log10(u32 v);
f32    tgm_u32_log2(u32 v);
u32    tgm_u32_pow(u32 base, u32 exponent);

f64    tgm_u64_log10(u64 v);
u64    tgm_u64_pow(u64 base, u64 exponent);



/*------------------------------------------------------------+
| Functional                                                  |
+------------------------------------------------------------*/

f32    tgm_f32_abs(f32 v);
f32    tgm_f32_blerp(f32 v00, f32 v01, f32 v10, f32 v11, f32 tx, f32 ty);
f32    tgm_f32_clamp(f32 v, f32 low, f32 high);
f32    tgm_f32_lerp(f32 v0, f32 v1, f32 t);
f32    tgm_f32_max(f32 v0, f32 v1);
f32    tgm_f32_min(f32 v0, f32 v1);
f32    tgm_f32_round(f32 v);
i32    tgm_f32_round_to_i32(f32 v);
f32    tgm_f32_tlerp(f32 v000, f32 v001, f32 v010, f32 v011, f32 v100, f32 v101, f32 v110, f32 v111, f32 tx, f32 ty, f32 tz);

i32    tgm_f64_floor_to_i32(f64 v);

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

u32    tgm_u64_digits(u64 v);
u64    tgm_u64_max(u64 v0, u64 v1);
u64    tgm_u64_min(u64 v0, u64 v1);



/*------------------------------------------------------------+
| Vectors                                                     |
+------------------------------------------------------------*/

v2     tgm_v2_add(v2 v0, v2 v1);
v2     tgm_v2_divf(v2 v, f32 f);
v2     tgm_v2_max(v2 v0, v2 v1);
v2     tgm_v2_mulf(v2 v, f32 f);
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
v3     tgm_v3_floor(v3 v);
v3     tgm_v3_lerp(v3 v0, v3 v1, f32 t);
f32    tgm_v3_mag(v3 v);
f32    tgm_v3_magsqr(v3 v);
v3     tgm_v3_max(v3 v0, v3 v1);
f32    tgm_v3_max_elem(v3 v);
v3     tgm_v3_min(v3 v0, v3 v1);
f32    tgm_v3_min_elem(v3 v);
v3     tgm_v3_mul(v3 v0, v3 v1);
v3     tgm_v3_mulf(v3 v, f32 f);
v3     tgm_v3_neg(v3 v);
v3     tgm_v3_normalized(v3 v);
v3     tgm_v3_normalized_not_null(v3 v, v3 alt);
v3     tgm_v3_reflect(v3 d, v3 n);
v3     tgm_v3_refract(v3 d, v3 n, f32 eta);
v3     tgm_v3_round(v3 v);
b32    tgm_v3_similar(v3 v0, v3 v1, f32 epsilon);
v3     tgm_v3_sub(v3 v0, v3 v1);
v3     tgm_v3_subf(v3 v, f32 f);
v3i    tgm_v3_to_v3i_ceil(v3 v);
v3i    tgm_v3_to_v3i_floor(v3 v);
v3i    tgm_v3_to_v3i_round(v3 v);
v4     tgm_v3_to_v4(v3 v, f32 w);

v3i    tgm_v3i_abs(v3i v);
v3i    tgm_v3i_add(v3i v0, v3i v1);
v3i    tgm_v3i_addi(v3i v0, i32 i);
v3i    tgm_v3i_div(v3i v0, v3i v1);
v3i    tgm_v3i_divi(v3i v0, i32 i);
b32    tgm_v3i_equal(v3i v0, v3i v1);
f32    tgm_v3i_mag(v3i v);
i32    tgm_v3i_magsqr(v3i v);
v3i    tgm_v3i_max(v3i v0, v3i v1);
i32    tgm_v3i_max_elem(v3i v);
v3i    tgm_v3i_min(v3i v0, v3i v1);
i32    tgm_v3i_min_elem(v3i v);
v3i    tgm_v3i_mul(v3i v0, v3i v1);
v3i    tgm_v3i_muli(v3i v0, i32 i);
v3i    tgm_v3i_sub(v3i v0, v3i v1);
v3i    tgm_v3i_subi(v3i v, i32 i);
v3     tgm_v3i_to_v3(v3i v);

v4     tgm_v4_add(v4 v0, v4 v1);
v4     tgm_v4_addf(v4 v, f32 f);
v4     tgm_v4_div(v4 v0, v4 v1);
v4     tgm_v4_divf(v4 v, f32 f);
f32    tgm_v4_dot(v4 v0, v4 v1);
b32    tgm_v4_equal(v4 v0, v4 v1);
f32    tgm_v4_mag(v4 v);
f32    tgm_v4_magsqr(v4 v);
v4     tgm_v4_max(v4 v0, v4 v1);
f32    tgm_v4_max_elem(v4 v);
v4     tgm_v4_min(v4 v0, v4 v1);
f32    tgm_v4_min_elem(v4 v);
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

m2     tgm_m2_identity(void);
m2     tgm_m2_multiply(m2 m0, m2 m1);
v2     tgm_m2_multiply_v2(m2 m, v2 v);
m2     tgm_m2_transposed(m2 m);

m3     tgm_m3_identity(void);
m3     tgm_m3_mul(m3 m0, m3 m1);
v3     tgm_m3_mulv3(m3 m, v3 v);
m3     tgm_m3_orthographic(f32 left, f32 right, f32 bottom, f32 top);
m3     tgm_m3_transposed(m3 m);

m4     tgm_m4_angle_axis(f32 angle_in_radians, v3 axis);
f32    tgm_m4_determinant(m4 m);
m4     tgm_m4_euler(f32 pitch_in_radians, f32 yaw_in_radians, f32 roll_in_radians);
m4     tgm_m4_identity(void);
m4     tgm_m4_inverse(m4 m);
m4     tgm_m4_look_at(v3 from, v3 to, v3 up);
m4     tgm_m4_mul(m4 m0, m4 m1);
v4     tgm_m4_mulv4(m4 m, v4 v);
m4     tgm_m4_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near);
m4     tgm_m4_perspective(f32 fov_y_in_radians, f32 aspect, f32 near, f32 far);
m4     tgm_m4_rotate_x(f32 angle_in_radians);
m4     tgm_m4_rotate_y(f32 angle_in_radians);
m4     tgm_m4_rotate_z(f32 angle_in_radians);
m4     tgm_m4_scale(v3 v);
m4     tgm_m4_translate(v3 v);
m4     tgm_m4_transposed(m4 m);

#endif
