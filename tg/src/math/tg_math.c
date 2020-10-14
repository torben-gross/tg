#include "math/tg_math.h"



/*------------------------------------------------------------+
| Miscellaneous                                               |
+------------------------------------------------------------*/

const i8 p_gradient_table[12][3] = {
	{  1,  1,  0 }, { -1,  1,  0 }, {  1, -1,  0 }, { -1, -1,  0 },
	{  1,  0,  1 }, { -1,  0,  1 }, {  1,  0, -1 }, { -1,  0, -1 },
	{  0,  1,  1 }, {  0, -1,  1 }, {  0,  1, -1 }, {  0, -1, -1 }
};

const u8 p_double_permutation_table[512] = {
	151, 160, 137,  91,  90,  15, 131,  13,
	201,  95,  96,  53, 194, 233,   7, 225,
	140,  36, 103,  30,  69, 142,   8,  99,
	 37, 240,  21,  10,  23, 190,   6, 148,
	247, 120, 234,  75,   0,  26, 197,  62,
	 94, 252, 219, 203, 117,  35,  11,  32,
	 57, 177,  33,  88, 237, 149,  56,  87,
	174,  20, 125, 136, 171, 168,  68, 175,
	 74, 165,  71, 134, 139,  48,  27, 166,
	 77, 146, 158, 231,  83, 111, 229, 122,
	 60, 211, 133, 230, 220, 105,  92,  41,
	 55,  46, 245,  40, 244, 102, 143,  54,
	 65,  25,  63, 161,   1, 216,  80,  73,
	209,  76, 132, 187, 208,  89,  18, 169,
	200, 196, 135, 130, 116, 188, 159,  86,
	164, 100, 109, 198, 173, 186,   3,  64,
	 52, 217, 226, 250, 124, 123,   5, 202,
	 38, 147, 118, 126, 255,  82,  85, 212,
	207, 206,  59, 227,  47,  16,  58,  17,
	182, 189,  28,  42, 223, 183, 170, 213,
	119, 248, 152,   2,  44, 154, 163,  70,
	221, 153, 101, 155, 167,  43, 172,   9,
	129,  22,  39, 253,  19,  98, 108, 110,
	 79, 113, 224, 232, 178, 185, 112, 104,
	218, 246,  97, 228, 251,  34, 242, 193,
	238, 210, 144,  12, 191, 179, 162, 241,
	 81,  51, 145, 235, 249,  14, 239, 107,
	 49, 192, 214,  31, 181, 199, 106, 157,
	184,  84, 204, 176, 115, 121,  50,  45,
	127,   4, 150, 254, 138, 236, 205,  93,
	222, 114,  67,  29,  24,  72, 243, 141,
	128, 195,  78,  66, 215,  61, 156, 180,

	151, 160, 137,  91,  90,  15, 131,  13,
	201,  95,  96,  53, 194, 233,   7, 225,
	140,  36, 103,  30,  69, 142,   8,  99,
	 37, 240,  21,  10,  23, 190,   6, 148,
	247, 120, 234,  75,   0,  26, 197,  62,
	 94, 252, 219, 203, 117,  35,  11,  32,
	 57, 177,  33,  88, 237, 149,  56,  87,
	174,  20, 125, 136, 171, 168,  68, 175,
	 74, 165,  71, 134, 139,  48,  27, 166,
	 77, 146, 158, 231,  83, 111, 229, 122,
	 60, 211, 133, 230, 220, 105,  92,  41,
	 55,  46, 245,  40, 244, 102, 143,  54,
	 65,  25,  63, 161,   1, 216,  80,  73,
	209,  76, 132, 187, 208,  89,  18, 169,
	200, 196, 135, 130, 116, 188, 159,  86,
	164, 100, 109, 198, 173, 186,   3,  64,
	 52, 217, 226, 250, 124, 123,   5, 202,
	 38, 147, 118, 126, 255,  82,  85, 212,
	207, 206,  59, 227,  47,  16,  58,  17,
	182, 189,  28,  42, 223, 183, 170, 213,
	119, 248, 152,   2,  44, 154, 163,  70,
	221, 153, 101, 155, 167,  43, 172,   9,
	129,  22,  39, 253,  19,  98, 108, 110,
	 79, 113, 224, 232, 178, 185, 112, 104,
	218, 246,  97, 228, 251,  34, 242, 193,
	238, 210, 144,  12, 191, 179, 162, 241,
	 81,  51, 145, 235, 249,  14, 239, 107,
	 49, 192, 214,  31, 181, 199, 106, 157,
	184,  84, 204, 176, 115, 121,  50,  45,
	127,   4, 150, 254, 138, 236, 205,  93,
	222, 114,  67,  29,  24,  72, 243, 141,
	128, 195,  78,  66, 215,  61, 156, 180
};

#define TGM_SIMPLEX_NOISE_FASTFLOOR(x)    ((x) > 0.0f ? (i32)(x) : (i32)(x) - 1)

f32 tgm_noise(f32 x, f32 y, f32 z) // see: http://staffwww.itn.liu.se/~stegu/simplexnoise/simplexnoise.pdf
{
	const f32 s = (x + y + z) * 0.333333343f;
	const i32 i = TGM_SIMPLEX_NOISE_FASTFLOOR(x + s);
	const i32 j = TGM_SIMPLEX_NOISE_FASTFLOOR(y + s);
	const i32 k = TGM_SIMPLEX_NOISE_FASTFLOOR(z + s);

	const f32 g3 = 0.166666672f;
	const f32 t = (i + j + k) * g3;
	const f32 x0 = x - ((f32)i - t);
	const f32 y0 = y - ((f32)j - t);
	const f32 z0 = z - ((f32)k - t);

	i32 i1, j1, k1;
	i32 i2, j2, k2;
	if (x0 >= y0)
	{
		if (y0 >= z0)
		{
			i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
		}
		else if (x0 >= z0)
		{
			i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1;
		}
		else
		{
			i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1;
		}
	}
	else
	{
		if (y0 < z0)
		{
			i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1;
		}
		else if (x0 < z0)
		{
			i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1;
		}
		else
		{
			i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
		}
	}

	const f32 x1 = x0 - i1 + g3;
	const f32 y1 = y0 - j1 + g3;
	const f32 z1 = z0 - k1 + g3;
	const f32 x2 = x0 - i2 + 2.0f * g3;
	const f32 y2 = y0 - j2 + 2.0f * g3;
	const f32 z2 = z0 - k2 + 2.0f * g3;
	const f32 x3 = x0 - 1.0f + 3.0f * g3;
	const f32 y3 = y0 - 1.0f + 3.0f * g3;
	const f32 z3 = z0 - 1.0f + 3.0f * g3;

	const i32 ii = i & 255;
	const i32 jj = j & 255;
	const i32 kk = k & 255;
	const i32 gi0 = (i32)p_double_permutation_table[ii      + (i32)p_double_permutation_table[jj      + (i32)p_double_permutation_table[kk     ]]] % 12;
	const i32 gi1 = (i32)p_double_permutation_table[ii + i1 + (i32)p_double_permutation_table[jj + j1 + (i32)p_double_permutation_table[kk + k1]]] % 12;
	const i32 gi2 = (i32)p_double_permutation_table[ii + i2 + (i32)p_double_permutation_table[jj + j2 + (i32)p_double_permutation_table[kk + k2]]] % 12;
	const i32 gi3 = (i32)p_double_permutation_table[ii +  1 + (i32)p_double_permutation_table[jj +  1 + (i32)p_double_permutation_table[kk +  1]]] % 12;

	f32 n0, n1, n2, n3;

	f32 t0 = 0.5f - x0 * x0 - y0 * y0 - z0 * z0;
	if (t0 < 0.0f)
	{
		n0 = 0.0f;
	}
	else
	{
		t0 *= t0;
		const f32 dot = (f32)p_gradient_table[gi0][0] * x0 + (f32)p_gradient_table[gi0][1] * y0 + (f32)p_gradient_table[gi0][2] * z0;
		n0 = t0 * t0 * dot;
	}

	f32 t1 = 0.5f - x1 * x1 - y1 * y1 - z1 * z1;
	if (t1 < 0.0f)
	{
		n1 = 0.0f;
	}
	else
	{
		t1 *= t1;
		const f32 dot = (f32)p_gradient_table[gi1][0] * x1 + (f32)p_gradient_table[gi1][1] * y1 + (f32)p_gradient_table[gi1][2] * z1;
		n1 = t1 * t1 * dot;
	}

	f32 t2 = 0.5f - x2 * x2 - y2 * y2 - z2 * z2;
	if (t2 < 0.0f)
	{
		n2 = 0.0f;
	}
	else
	{
		t2 *= t2;
		const f32 dot = (f32)p_gradient_table[gi2][0] * x2 + (f32)p_gradient_table[gi2][1] * y2 + (f32)p_gradient_table[gi2][2] * z2;
		n2 = t2 * t2 * dot;
	}

	f32 t3 = 0.5f - x3 * x3 - y3 * y3 - z3 * z3;
	if (t3 < 0.0f)
	{
		n3 = 0.0f;
	}
	else
	{
		t3 *= t3;
		const f32 dot = (f32)p_gradient_table[gi3][0] * x3 + (f32)p_gradient_table[gi3][1] * y3 + (f32)p_gradient_table[gi3][2] * z3;
		n3 = t3 * t3 * dot;
	}

	const f32 result = 32.0f * (n0 + n1 + n2 + n3);
	return result;
}

#undef TGM_SIMPLEX_NOISE_FASTFLOOR

tg_random tgm_random_init(u32 seed)
{
	TG_ASSERT(seed != 0);

	tg_random result = { seed };
	return result;
}

f32 tgm_random_next_f32(tg_random* p_random)
{
	TG_ASSERT(p_random);

	const f32 result = (f32)tgm_random_next_u32(p_random) / (f32)TG_U32_MAX;
	return result;
}

f32 tgm_random_next_f32_between(tg_random* p_random, f32 low, f32 high)
{
	TG_ASSERT(p_random && low <= high);

	const f32 result = tgm_random_next_f32(p_random) * (high - low) + low;
	return result;
}

u32 tgm_random_next_u32(tg_random* p_random)
{
	TG_ASSERT(p_random);

	u32 result = p_random->state;
	result ^= result << 13;
	result ^= result >> 17;
	result ^= result << 5;
	p_random->state = result;
	return result;
}

void tg__enclosing_sphere(u32 contained_point_count, const v3* p_contained_points, u32 boundary_point_count, v3* p_boundary_points, v3* p_center, f32* p_radius)
{
	if (contained_point_count == 0 || boundary_point_count == 4)
	{
		switch (boundary_point_count)
		{
		case 0:
		{
			*p_center = (v3) { 0.0f, 0.0f, 0.0f };
			*p_radius = -1.0f;
		} break;
		case 1:
		{
			*p_center = p_boundary_points[0];
			*p_radius = 0.0f;
		} break;
		case 2:
		{
			const v3 half_span = tgm_v3_divf(tgm_v3_sub(p_boundary_points[1], p_boundary_points[0]), 2.0f);
			*p_center = tgm_v3_add(p_boundary_points[0], half_span);
			*p_radius = tgm_v3_mag(half_span);
		} break;
		case 3:
		{
			const v3 v20 = tgm_v3_sub(p_boundary_points[0], p_boundary_points[2]);
			const v3 v21 = tgm_v3_sub(p_boundary_points[1], p_boundary_points[2]);
			const v3 cross = tgm_v3_cross(v20, v21);

			*p_center = tgm_v3_add(
				p_boundary_points[2],
				tgm_v3_divf(
					tgm_v3_cross(
						tgm_v3_sub(
							tgm_v3_mulf(v21, tgm_v3_magsqr(v20)),
							tgm_v3_mulf(v20, tgm_v3_magsqr(v21))
						),
						cross),
					2.0f * tgm_v3_magsqr(cross)
				)
			);
			*p_radius = tgm_v3_mag(tgm_v3_sub(*p_center, p_boundary_points[0]));
		} break;
		case 4:
		{
			m4 m = { 0 };
			m.col0.xyz = p_boundary_points[0]; m.m30 = 1.0f;
			m.col1.xyz = p_boundary_points[1]; m.m31 = 1.0f;
			m.col2.xyz = p_boundary_points[2]; m.m32 = 1.0f;
			m.col3.xyz = p_boundary_points[3]; m.m33 = 1.0f;

			m4 d = m;

			d.m00 = tgm_v3_magsqr(p_boundary_points[0]); d.m01 = tgm_v3_magsqr(p_boundary_points[1]); d.m02 = tgm_v3_magsqr(p_boundary_points[2]); d.m03 = tgm_v3_magsqr(p_boundary_points[3]);
			p_center->x = tgm_m4_determinant(d);

			d.m10 = m.m00; d.m11 = m.m01; d.m12 = m.m02; d.m13 = m.m03;
			p_center->y = -tgm_m4_determinant(d);

			d.m20 = m.m10; d.m21 = m.m11; d.m22 = m.m12; d.m23 = m.m13;
			p_center->z = tgm_m4_determinant(d);

			*p_center = tgm_v3_divf(*p_center, 2.0f * tgm_m4_determinant(m));
			*p_radius = tgm_v3_mag(tgm_v3_sub(*p_center, p_boundary_points[0]));
		} break;
		default: TG_INVALID_CODEPATH(); break;
		}
	}
	else
	{
		tg__enclosing_sphere(contained_point_count - 1, &p_contained_points[1], boundary_point_count, p_boundary_points, p_center, p_radius);

		const b32 contains = tgm_v3_magsqr(tgm_v3_sub(p_contained_points[0], *p_center)) * 0.99f <= *p_radius * *p_radius;
		if (!contains)
		{
			TG_ASSERT(boundary_point_count < 4);
			p_boundary_points[boundary_point_count] = p_contained_points[0];
			tg__enclosing_sphere(contained_point_count - 1, &p_contained_points[1], boundary_point_count + 1, p_boundary_points, p_center, p_radius);
		}
	}
}

void tgm_enclosing_sphere(u32 contained_point_count, const v3* p_contained_points, v3* p_center, f32* p_radius)
{
	TG_ASSERT(contained_point_count && p_contained_points && p_center && p_radius);

	v3 p_boundary_points[4] = { 0 };
	tg__enclosing_sphere(contained_point_count, p_contained_points, 0, p_boundary_points, p_center, p_radius);
}



/*------------------------------------------------------------+
| Intrinsics                                                  |
+------------------------------------------------------------*/

f32 acosf(f32 v);
f32 asinf(f32 v);
f32 atanf(f32 v);
f32 acoshf(f32 v);
f32 asinhf(f32 v);
f32 atanhf(f32 v);
f32 ceilf(f32 v);
f32 cosf(f32 v);
f32 coshf(f32 v);
f64 floor(f64 v);
f32 floorf(f32 v);
f64 log10(f64 v);
f32 log10f(f32 v);
f32 logf(f32 v);
f64 pow(f64 base, f64 exponent);
f32 powf(f32 base, f32 exponent);
f32 roundf(f32 v);
f32 sinf(f32 v);
f32 sinhf(f32 v);
f32 sqrtf(f32 v);
f32 tanf(f32 v);
f32 tanhf(f32 v);

f32 tgm_f32_acos(f32 v)
{
	const f32 result = acosf(v);
	return result;
}

f32 tgm_f32_acosh(f32 v)
{
	const f32 result = acoshf(v);
	return result;
}

f32 tgm_f32_asin(f32 v)
{
	const f32 result = asinf(v);
	return result;
}

f32 tgm_f32_asinh(f32 v)
{
	const f32 result = asinhf(v);
	return result;
}

f32 tgm_f32_atan(f32 v)
{
	const f32 result = atanf(v);
	return result;
}

f32 tgm_f32_atanh(f32 v)
{
	const f32 result = atanhf(v);
	return result;
}

f32 tgm_f32_ceil(f32 v)
{
	const f32 result = ceilf(v);
	return result;
}

f32 tgm_f32_cos(f32 v)
{
	const f32 result = cosf(v);
	return result;
}

f32 tgm_f32_cosh(f32 v)
{
	const f32 result = coshf(v);
	return result;
}

f32 tgm_f32_floor(f32 v)
{
	const f32 result = floorf(v);
	return result;
}

f32 tgm_f32_log10(f32 v)
{
	const f32 result = log10f(v);
	return result;
}

f32 tgm_f32_log2(f32 v)
{
	const f32 result = logf(v);
	return result;
}

f32 tgm_f32_pow(f32 base, f32 exponent)
{
	const f32 result = powf(base, exponent);
	return result;
}

f32 tgm_f32_sin(f32 v)
{
	const f32 result = sinf(v);
	return result;
}

f32 tgm_f32_sinh(f32 v)
{
	const f32 result = sinhf(v);
	return result;
}

f32 tgm_f32_sqrt(f32 v)
{
	const f32 result = sqrtf(v);
	return result;
}

f32 tgm_f32_tan(f32 v)
{
	const f32 result = tanf(v);
	return result;
}

f32 tgm_f32_tanh(f32 v)
{
	const f32 result = tanhf(v);
	return result;
}



f64 tgm_f64_pow(f64 base, f64 exponent)
{
	const f64 result = pow(base, exponent);
	return result;
}



f32 tgm_i32_log10(i32 v)
{
	const f32 result = log10f((f32)v);
	return result;
}

f32 tgm_i32_log2(i32 v)
{
	const f32 result = logf((f32)v);
	return result;
}

i32 tgm_i32_pow(i32 base, i32 exponent)
{
	const i32 result = (i32)powf((f32)base, (f32)exponent);
	return result;
}



f32 tgm_u32_log10(u32 v)
{
	const f32 result = log10f((f32)v);
	return result;
}

f32 tgm_u32_log2(u32 v)
{
	const f32 result = logf((f32)v);
	return result;
}

u32 tgm_u32_pow(u32 base, u32 exponent)
{
	const u32 result = (u32)powf((f32)base, (f32)exponent);
	return result;
}



f64 tgm_u64_log10(u64 v)
{
	const f64 result = log10((f64)v);
	return result;
}

u64 tgm_u64_pow(u64 base, u64 exponent)
{
	const u64 result = (u64)pow((f64)base, (f64)exponent);
	return result;
}



/*------------------------------------------------------------+
| Functional                                                  |
+------------------------------------------------------------*/

f32 tgm_f32_abs(f32 v)
{
	const f32 result = v < 0 ? -v : v;
	return result;
}

f32 tgm_f32_blerp(f32 v00, f32 v01, f32 v10, f32 v11, f32 tx, f32 ty)
{
	const f32 result = tgm_f32_lerp(tgm_f32_lerp(v00, v10, tx), tgm_f32_lerp(v01, v11, tx), ty);
	return result;
}

f32 tgm_f32_clamp(f32 v, f32 low, f32 high)
{
	const f32 result = tgm_f32_max(low, tgm_f32_min(high, v));
	return result;
}

f32 tgm_f32_lerp(f32 v0, f32 v1, f32 t)
{
	const f32 result = (1.0f - t) * v0 + t * v1;
	return result;
}

f32 tgm_f32_max(f32 v0, f32 v1)
{
	const f32 result = v0 > v1 ? v0 : v1;
	return result;
}

f32 tgm_f32_min(f32 v0, f32 v1)
{
	const f32 result = v0 < v1 ? v0 : v1;
	return result;
}

f32 tgm_f32_round(f32 v)
{
	const f32 result = (f32)tgm_f32_round_to_i32(v);
	return result;
}

i32 tgm_f32_round_to_i32(f32 v)
{
	const i32 result = v >= 0.0f ? (i32)(v + 0.5f) : (i32)(-(tgm_f32_abs(v) + 0.5f));
	return result;
}

f32 tgm_f32_tlerp(f32 v000, f32 v001, f32 v010, f32 v011, f32 v100, f32 v101, f32 v110, f32 v111, f32 tx, f32 ty, f32 tz)
{
	const f32 lz0 = tgm_f32_blerp(v000, v010, v100, v110, tx, ty);
	const f32 lz1 = tgm_f32_blerp(v001, v011, v101, v111, tx, ty);
	const f32 result = tgm_f32_lerp(lz0, lz1, tz);
	return result;
}

i32 tgm_f64_floor_to_i32(f64 v)
{
	const i32 result = (i32)floor(v);
	return result;
}

i32 tgm_i32_abs(i32 v)
{
	TG_ASSERT(v != TG_I32_MIN);

	const i32 result = v < 0 ? -v : v;
	return result;
}

i32 tgm_i32_clamp(i32 v, i32 low, i32 high)
{
	const i32 result = tgm_i32_max(low, tgm_i32_min(high, v));
	return result;
}

u32 tgm_i32_digits(i32 v)
{
	if (v == TG_I32_MIN)
	{
		v += 1;
	}
	const u32 result = v == 0 ? 1 : (u32)tgm_f32_floor(tgm_i32_log10(tgm_i32_abs(v))) + 1;
	return result;
}

b32 tgm_i32_is_power_of_two(i32 v)
{
	const b32 result = tgm_f32_ceil(tgm_i32_log2(v)) == tgm_f32_floor(tgm_i32_log2(v));
	return result;
}

i32 tgm_i32_max(i32 v0, i32 v1)
{
	const i32 result = v0 > v1 ? v0 : v1;
	return result;
}

i32 tgm_i32_min(i32 v0, i32 v1)
{
	const i32 result = v0 < v1 ? v0 : v1;
	return result;
}

u8 tgm_u8_max(u8 v0, u8 v1)
{
	const u8 result = v0 > v1 ? v0 : v1;
	return result;
}

u8 tgm_u8_min(u8 v0, u8 v1)
{
	const u8 result = v0 < v1 ? v0 : v1;
	return result;
}

u32 tgm_u32_clamp(u32 v, u32 low, u32 high)
{
	const u32 result = tgm_u32_max(low, tgm_u32_min(high, v));
	return result;
}

u32 tgm_u32_digits(u32 v)
{
	const u32 result = v == 0 ? 1 : (u32)tgm_f32_floor(tgm_u32_log10(v)) + 1;
	return result;
}

u32 tgm_u32_incmod(u32 v, u32 mod)
{
	u32 result = v + 1;
	if (result == mod)
	{
		result = 0;
	}
	return result;
}

b32 tgm_u32_is_power_of_two(u32 v)
{
	const b32 result = tgm_f32_ceil(tgm_u32_log2(v)) == tgm_f32_floor(tgm_u32_log2(v));
	return result;
}

u32 tgm_u32_max(u32 v0, u32 v1)
{
	const u32 result = v0 > v1 ? v0 : v1;
	return result;
}

u32 tgm_u32_min(u32 v0, u32 v1)
{
	const u32 result = v0 < v1 ? v0 : v1;
	return result;
}

u32 tgm_u64_digits(u64 v)
{
	const u32 result = v == 0 ? 1 : (u32)tgm_f32_floor((f32)tgm_u64_log10(v)) + 1;
	return result;
}

u64 tgm_u64_max(u64 v0, u64 v1)
{
	const u64 result = v0 > v1 ? v0 : v1;
	return result;
}

u64 tgm_u64_min(u64 v0, u64 v1)
{
	const u64 result = v0 < v1 ? v0 : v1;
	return result;
}



/*------------------------------------------------------------+
| Vectors                                                     |
+------------------------------------------------------------*/

v2 tgm_v2_add(v2 v0, v2 v1)
{
	v2 result = { 0 };
	result.x = v0.x + v1.x;
	result.y = v0.y + v1.y;
	return result;
}

v2 tgm_v2_divf(v2 v, f32 f)
{
	v2 result = { 0 };
	result.x = v.x / f;
	result.y = v.y / f;
	return result;
}

v2 tgm_v2_max(v2 v0, v2 v1)
{
	v2 result = { 0 };
	result.x = v0.x > v1.x ? v0.x : v1.x;
	result.y = v0.y > v1.y ? v0.y : v1.y;
	return result;
}

v2 tgm_v2_mulf(v2 v, f32 f)
{
	v2 result = { 0 };
	result.x = v.x * f;
	result.y = v.y * f;
	return result;
}

v2 tgm_v2_min(v2 v0, v2 v1)
{
	v2 result = { 0 };
	result.x = v0.x < v1.x ? v0.x : v1.x;
	result.y = v0.y < v1.y ? v0.y : v1.y;
	return result;
}

v2 tgm_v2_sub(v2 v0, v2 v1)
{
	v2 result = { 0 };
	result.x = v0.x - v1.x;
	result.y = v0.y - v1.y;
	return result;
}



v3 tgm_v3_abs(v3 v)
{
	v3 result = { 0 };
	result.x = v.x < 0 ? -v.x : v.x;
	result.y = v.y < 0 ? -v.y : v.y;
	result.z = v.z < 0 ? -v.z : v.z;
	return result;
}

v3 tgm_v3_add(v3 v0, v3 v1)
{
	v3 result = { 0 };
	result.x = v0.x + v1.x;
	result.y = v0.y + v1.y;
	result.z = v0.z + v1.z;
	return result;
}

v3 tgm_v3_addf(v3 v, f32 f)
{
	v3 result = { 0 };
	result.x = v.x + f;
	result.y = v.y + f;
	result.z = v.z + f;
	return result;
}

v3 tgm_v3_clamp(v3 v, v3 min, v3 max)
{
	v3 result = { 0 };
	result.x = tgm_f32_clamp(v.x, min.x, max.x);
	result.y = tgm_f32_clamp(v.y, min.y, max.y);
	result.z = tgm_f32_clamp(v.z, min.z, max.z);
	return result;
}

v3 tgm_v3_cross(v3 v0, v3 v1)
{
	v3 result = { 0 };
	result.x = v0.y * v1.z - v0.z * v1.y;
	result.y = v0.z * v1.x - v0.x * v1.z;
	result.z = v0.x * v1.y - v0.y * v1.x;
	return result;
}

v3 tgm_v3_div(v3 v0, v3 v1)
{
	TG_ASSERT(v1.x && v1.y && v1.z);

	v3 result = { 0 };
	result.x = v0.x / v1.x;
	result.y = v0.y / v1.y;
	result.z = v0.z / v1.z;
	return result;
}

v3 tgm_v3_divf(v3 v, f32 f)
{
	TG_ASSERT(f);

	v3 result = { 0 };
	result.x = v.x / f;
	result.y = v.y / f;
	result.z = v.z / f;
	return result;
}

f32 tgm_v3_dot(v3 v0, v3 v1)
{
	const f32 result = v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
	return result;
}

b32 tgm_v3_equal(v3 v0, v3 v1)
{
	const b32 result = v0.x == v1.x && v0.y == v1.y && v0.z == v1.z;
	return result;
}

v3 tgm_v3_floor(v3 v)
{
	v3 result = { 0 };
	result.x = floorf(v.x);
	result.y = floorf(v.y);
	result.z = floorf(v.z);
	return result;
}

v3 tgm_v3_lerp(v3 v0, v3 v1, f32 t)
{
	v3 result = { 0 };
	result.x = (1.0f - t) * v0.x + t * v1.x;
	result.y = (1.0f - t) * v0.y + t * v1.y;
	result.z = (1.0f - t) * v0.z + t * v1.z;
	return result;
}

f32 tgm_v3_mag(v3 v)
{
	const f32 result = tgm_f32_sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	return result;
}

f32 tgm_v3_magsqr(v3 v)
{
	const f32 result = v.x * v.x + v.y * v.y + v.z * v.z;
	return result;
}

v3 tgm_v3_max(v3 v0, v3 v1)
{
	v3 result = { 0 };
	result.x = v0.x > v1.x ? v0.x : v1.x;
	result.y = v0.y > v1.y ? v0.y : v1.y;
	result.z = v0.z > v1.z ? v0.z : v1.z;
	return result;
}

f32 tgm_v3_max_elem(v3 v)
{
	const f32 result = tgm_f32_max(tgm_f32_max(v.x, v.y), v.z);
	return result;
}

v3 tgm_v3_min(v3 v0, v3 v1)
{
	v3 result = { 0 };
	result.x = v0.x < v1.x ? v0.x : v1.x;
	result.y = v0.y < v1.y ? v0.y : v1.y;
	result.z = v0.z < v1.z ? v0.z : v1.z;
	return result;
}

f32 tgm_v3_min_elem(v3 v)
{
	const f32 result = tgm_f32_min(tgm_f32_min(v.x, v.y), v.z);
	return result;
}

v3 tgm_v3_mul(v3 v0, v3 v1)
{
	v3 result = { 0 };
	result.x = v0.x * v1.x;
	result.y = v0.y * v1.y;
	result.z = v0.z * v1.z;
	return result;
}

v3 tgm_v3_mulf(v3 v, f32 f)
{
	v3 result = { 0 };
	result.x = v.x * f;
	result.y = v.y * f;
	result.z = v.z * f;
	return result;
}

v3 tgm_v3_neg(v3 v)
{
	v3 result = { 0 };
	result.x = -v.x;
	result.y = -v.y;
	result.z = -v.z;
	return result;
}

v3 tgm_v3_normalized(v3 v)
{
	v3 result = { 0 };
	const f32 mag = tgm_v3_mag(v);
	TG_ASSERT(mag);
	result.x = v.x / mag;
	result.y = v.y / mag;
	result.z = v.z / mag;
	return result;
}

v3 tgm_v3_normalized_not_null(v3 v, v3 alt)
{
	v3 result = alt;
	const f32 magsqr = tgm_v3_magsqr(v);
	if (magsqr != 0.0f)
	{
		const f32 mag = tgm_f32_sqrt(magsqr);
		result = (v3){ v.x / mag, v.y / mag, v.z / mag };
	}
	return result;
}

v3 tgm_v3_reflect(v3 d, v3 n)
{
	TG_DEBUG_EXEC(
		const f32 ms = tgm_v3_magsqr(n);
		TG_ASSERT(ms < 1.02f && ms > 0.98f)
	);

	const v3 result = tgm_v3_sub(d, tgm_v3_mulf(n, 2.0f * tgm_v3_dot(n, d)));
	return result;
}

v3 tgm_v3_refract(v3 d, v3 n, f32 eta)
{
	TG_DEBUG_EXEC(
		const f32 msd = tgm_v3_magsqr(d);
		const f32 msn = tgm_v3_magsqr(n);
		TG_ASSERT(msd < 1.02f && msd > 0.98f && msn < 1.02f && msn > 0.98f)
	);

	v3 result = { 0 };
	const f32 dot = tgm_v3_dot(n, d);
	const f32 k = 1.0f - eta * eta * (1.0f - dot * dot);
	if (k >= 0.0f)
	{
		result = tgm_v3_sub(tgm_v3_mulf(d, eta), tgm_v3_mulf(n, eta * tgm_v3_dot(n, d) + tgm_f32_sqrt(k)));
	}
	return result;
}

v3 tgm_v3_round(v3 v)
{
	v3 result = { 0 };
	result.x = roundf(v.x);
	result.y = roundf(v.y);
	result.z = roundf(v.z);
	return result;
}

b32 tgm_v3_similar(v3 v0, v3 v1, f32 epsilon)
{
	const v3 v01 = tgm_v3_sub(v1, v0);
	const f32 mag_sqr = tgm_v3_magsqr(v01);
	const b32 result = mag_sqr <= epsilon * epsilon;
	return result;
}

v3 tgm_v3_sub(v3 v0, v3 v1)
{
	v3 result = { 0 };
	result.x = v0.x - v1.x;
	result.y = v0.y - v1.y;
	result.z = v0.z - v1.z;
	return result;
}

v3 tgm_v3_subf(v3 v, f32 f)
{
	v3 result = { 0 };
	result.x = v.x - f;
	result.y = v.y - f;
	result.z = v.z - f;
	return result;
}

v3i tgm_v3_to_v3i_ceil(v3 v)
{
	v3i result = { 0 };
	result.x = (i32)ceilf(v.x);
	result.y = (i32)ceilf(v.y);
	result.z = (i32)ceilf(v.z);
	return result;
}

v3i tgm_v3_to_v3i_floor(v3 v)
{
	v3i result = { 0 };
	result.x = (i32)floorf(v.x);
	result.y = (i32)floorf(v.y);
	result.z = (i32)floorf(v.z);
	return result;
}

v3i tgm_v3_to_v3i_round(v3 v)
{
	v3i result = { 0 };
	result.x = (i32)roundf(v.x);
	result.y = (i32)roundf(v.y);
	result.z = (i32)roundf(v.z);
	return result;
}

v4 tgm_v3_to_v4(v3 v, f32 w)
{
	v4 result = { 0 };
	result.x = v.x;
	result.y = v.y;
	result.z = v.z;
	result.w = w;
	return result;
}



v3i tgm_v3i_abs(v3i v)
{
	v3i result = { 0 };
	result.x = v.x < 0 ? -v.x : v.x;
	result.y = v.y < 0 ? -v.y : v.y;
	result.z = v.z < 0 ? -v.z : v.z;
	return result;
}

v3i tgm_v3i_add(v3i v0, v3i v1)
{
	v3i result = { 0 };
	result.x = v0.x + v1.x;
	result.y = v0.y + v1.y;
	result.z = v0.z + v1.z;
	return result;
}

v3i tgm_v3i_addi(v3i v0, i32 i)
{
	v3i result = { 0 };
	result.x = v0.x + i;
	result.y = v0.y + i;
	result.z = v0.z + i;
	return result;
}

v3i tgm_v3i_div(v3i v0, v3i v1)
{
	TG_ASSERT(v1.x && v1.y && v1.z);

	v3i result = { 0 };
	result.x = v0.x / v1.x;
	result.y = v0.y / v1.y;
	result.z = v0.z / v1.z;
	return result;
}

v3i tgm_v3i_divi(v3i v0, i32 i)
{
	TG_ASSERT(i != 0);

	v3i result = { 0 };
	result.x = v0.x / i;
	result.y = v0.y / i;
	result.z = v0.z / i;
	return result;
}

b32 tgm_v3i_equal(v3i v0, v3i v1)
{
	const b32 result = v0.x == v1.x && v0.y == v1.y && v0.z == v1.z;
	return result;
}

f32 tgm_v3i_mag(v3i v)
{
	const f32 result = tgm_f32_sqrt((f32)(v.x * v.x + v.y * v.y + v.z * v.z));
	return result;
}

i32 tgm_v3i_magsqr(v3i v)
{
	const i32 result = v.x * v.x + v.y * v.y + v.z * v.z;
	return result;
}

v3i tgm_v3i_max(v3i v0, v3i v1)
{
	v3i result = { 0 };
	result.x = v0.x > v1.x ? v0.x : v1.x;
	result.y = v0.y > v1.y ? v0.y : v1.y;
	result.z = v0.z > v1.z ? v0.z : v1.z;
	return result;
}

i32 tgm_v3i_max_elem(v3i v)
{
	const i32 result = tgm_i32_max(tgm_i32_max(v.x, v.y), v.z);
	return result;
}

v3i tgm_v3i_min(v3i v0, v3i v1)
{
	v3i result = { 0 };
	result.x = v0.x < v1.x ? v0.x : v1.x;
	result.y = v0.y < v1.y ? v0.y : v1.y;
	result.z = v0.z < v1.z ? v0.z : v1.z;
	return result;
}

i32 tgm_v3i_min_elem(v3i v)
{
	const i32 result = tgm_i32_min(tgm_i32_min(v.x, v.y), v.z);
	return result;
}

v3i tgm_v3i_mul(v3i v0, v3i v1)
{
	v3i result = { 0 };
	result.x = v0.x * v1.x;
	result.y = v0.y * v1.y;
	result.z = v0.z * v1.z;
	return result;
}

v3i tgm_v3i_muli(v3i v0, i32 i)
{
	v3i result = { 0 };
	result.x = v0.x * i;
	result.y = v0.y * i;
	result.z = v0.z * i;
	return result;
}

v3i tgm_v3i_sub(v3i v0, v3i v1)
{
	v3i result = { 0 };
	result.x = v0.x - v1.x;
	result.y = v0.y - v1.y;
	result.z = v0.z - v1.z;
	return result;
}

v3i tgm_v3i_subi(v3i v, i32 i)
{
	v3i result = { 0 };
	result.x = v.x - i;
	result.y = v.y - i;
	result.z = v.z - i;
	return result;
}

v3 tgm_v3i_to_v3(v3i v)
{
	v3 result = { 0 };
	result.x = (f32)v.x;
	result.y = (f32)v.y;
	result.z = (f32)v.z;
	return result;
}



v4 tgm_v4_add(v4 v0, v4 v1)
{
	v4 result = { 0 };
	result.x = v0.x + v1.x;
	result.y = v0.y + v1.y;
	result.z = v0.z + v1.z;
	result.w = v0.w + v1.w;
	return result;
}

v4 tgm_v4_addf(v4 v, f32 f)
{
	v4 result = { 0 };
	result.x = v.x + f;
	result.y = v.y + f;
	result.z = v.z + f;
	result.w = v.w + f;
	return result;
}

v4 tgm_v4_div(v4 v0, v4 v1)
{
	TG_ASSERT(v1.x && v1.y && v1.z && v1.w);

	v4 result = { 0 };
	result.x = v0.x / v1.x;
	result.y = v0.y / v1.y;
	result.z = v0.z / v1.z;
	result.w = v0.w / v1.w;
	return result;
}

v4 tgm_v4_divf(v4 v, f32 f)
{
	TG_ASSERT(f);

	v4 result = { 0 };
	result.x = v.x / f;
	result.y = v.y / f;
	result.z = v.z / f;
	result.w = v.w / f;
	return result;
}

f32 tgm_v4_dot(v4 v0, v4 v1)
{
	const f32 result = v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w;
	return result;
}

b32 tgm_v4_equal(v4 v0, v4 v1)
{
	const b32 result = v0.x == v1.x && v0.y == v1.y && v0.z == v1.z && v0.w == v1.w;
	return result;
}

f32 tgm_v4_mag(v4 v)
{
	const f32 result = tgm_f32_sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
	return result;
}

f32 tgm_v4_magsqr(v4 v)
{
	const f32 result = v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
	return result;
}

v4 tgm_v4_max(v4 v0, v4 v1)
{
	v4 result = { 0 };
	result.x = v0.x > v1.x ? v0.x : v1.x;
	result.y = v0.y > v1.y ? v0.y : v1.y;
	result.z = v0.z > v1.z ? v0.z : v1.z;
	result.w = v0.w > v1.w ? v0.w : v1.w;
	return result;
}

f32 tgm_v4_max_elem(v4 v)
{
	const f32 result = tgm_f32_max(tgm_f32_max(tgm_f32_max(v.x, v.y), v.z), v.w);
	return result;
}

v4 tgm_v4_min(v4 v0, v4 v1)
{
	v4 result = { 0 };
	result.x = v0.x < v1.x ? v0.x : v1.x;
	result.y = v0.y < v1.y ? v0.y : v1.y;
	result.z = v0.z < v1.z ? v0.z : v1.z;
	result.w = v0.w < v1.w ? v0.w : v1.w;
	return result;
}

f32 tgm_v4_min_elem(v4 v)
{
	const f32 result = tgm_f32_min(tgm_f32_min(tgm_f32_min(v.x, v.y), v.z), v.w);
	return result;
}

v4 tgm_v4_mul(v4 v0, v4 v1)
{
	v4 result = { 0 };
	result.x = v0.x * v1.x;
	result.y = v0.y * v1.y;
	result.z = v0.z * v1.z;
	result.w = v0.w * v1.w;
	return result;
}

v4 tgm_v4_mulf(v4 v, f32 f)
{
	v4 result = { 0 };
	result.x = v.x * f;
	result.y = v.y * f;
	result.z = v.z * f;
	return result;
}

v4 tgm_v4_neg(v4 v)
{
	v4 result = { 0 };
	result.x = -v.x;
	result.y = -v.y;
	result.z = -v.z;
	result.w = -v.w;
	return result;
}

v4 tgm_v4_normalized(v4 v)
{
	v4 result = { 0 };
	const f32 mag = tgm_v4_mag(v);
	TG_ASSERT(mag);
	result.x = v.x / mag;
	result.y = v.y / mag;
	result.z = v.z / mag;
	result.w = v.w / mag;
	return result;
}

v4 tgm_v4_sub(v4 v0, v4 v1)
{
	v4 result = { 0 };
	result.x = v0.x - v1.x;
	result.y = v0.y - v1.y;
	result.z = v0.z - v1.z;
	result.w = v0.w - v1.w;
	return result;
}

v4 tgm_v4_subf(v4 v, f32 f)
{
	v4 result = { 0 };
	result.x = v.x - f;
	result.y = v.y - f;
	result.z = v.z - f;
	result.w = v.w - f;
	return result;
}

v3 tgm_v4_to_v3(v4 v)
{
	v3 result = { 0 };
	if (v.w == 0.0f)
	{
		result.x = v.x;
		result.y = v.y;
		result.z = v.z;
	}
	else
	{
		result.x = v.x / v.w;
		result.y = v.y / v.w;
		result.z = v.z / v.w;
	}
	return result;
}



/*------------------------------------------------------------+
| Matrices                                                    |
+------------------------------------------------------------*/

m2 tgm_m2_identity(void)
{
	m2 result = { 0 };

	result.m00 = 1.0f;
	result.m10 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = 1.0f;

	return result;
}

m2 tgm_m2_multiply(m2 m0, m2 m1)
{
	m2 result = { 0 };

	result.m00 = m0.m00 * m1.m00 + m0.m01 * m1.m10;
	result.m10 = m0.m10 * m1.m00 + m0.m11 * m1.m10;

	result.m01 = m0.m00 * m1.m01 + m0.m01 * m1.m11;
	result.m11 = m0.m10 * m1.m01 + m0.m11 * m1.m11;

	return result;
}

v2 tgm_m2_multiply_v2(m2 m, v2 v)
{
	v2 result = { 0 };

	result.x = v.x * m.m00 + v.y * m.m01;
	result.y = v.x * m.m10 + v.y * m.m11;

	return result;
}

m2 tgm_m2_transposed(m2 m)
{
	m2 result = { 0 };

	result.m00 = m.m00;
	result.m10 = m.m01;
	result.m01 = m.m10;
	result.m11 = m.m11;

	return result;
}



m3 tgm_m3_identity(void)
{
	m3 result = { 0 };

	result.m00 = 1.0f;
	result.m10 = 0.0f;
	result.m20 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = 1.0f;
	result.m21 = 0.0f;

	result.m02 = 0.0f;
	result.m12 = 0.0f;
	result.m22 = 1.0f;

	return result;
}

m3 tgm_m3_mul(m3 m0, m3 m1)
{
	m3 result = { 0 };

	result.m00 = m0.m00 * m1.m00 + m0.m01 * m1.m10 + m0.m02 * m1.m20;
	result.m10 = m0.m10 * m1.m00 + m0.m11 * m1.m10 + m0.m12 * m1.m20;
	result.m20 = m0.m20 * m1.m00 + m0.m21 * m1.m10 + m0.m22 * m1.m20;

	result.m01 = m0.m00 * m1.m01 + m0.m01 * m1.m11 + m0.m02 * m1.m21;
	result.m11 = m0.m10 * m1.m01 + m0.m11 * m1.m11 + m0.m12 * m1.m21;
	result.m21 = m0.m20 * m1.m01 + m0.m21 * m1.m11 + m0.m22 * m1.m21;

	result.m02 = m0.m00 * m1.m02 + m0.m01 * m1.m12 + m0.m02 * m1.m22;
	result.m12 = m0.m10 * m1.m02 + m0.m11 * m1.m12 + m0.m12 * m1.m22;
	result.m22 = m0.m20 * m1.m02 + m0.m21 * m1.m12 + m0.m22 * m1.m22;

	return result;
}

v3 tgm_m3_mulv3(m3 m, v3 v)
{
	v3 result = { 0 };

	result.x = v.x * m.m00 + v.y * m.m01 + v.z * m.m02;
	result.y = v.x * m.m10 + v.y * m.m11 + v.z * m.m12;
	result.z = v.x * m.m20 + v.y * m.m21 + v.z * m.m22;

	return result;
}

m3 tgm_m3_orthographic(f32 left, f32 right, f32 bottom, f32 top)
{
	TG_ASSERT(right != left && top != bottom);

	m3 result = { 0 };

	result.m00 = 2.0f / (right - left);
	result.m10 = 0.0f;
	result.m20 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = -2.0f / (top - bottom);
	result.m21 = 0.0f;

	result.m02 = -(right + left) / (right - left);
	result.m12 = -(bottom + top) / (bottom - top);
	result.m22 = 1.0f;

	return result;
}

m3 tgm_m3_transposed(m3 m)
{
	m3 result = { 0 };

	result.m00 = m.m00;
	result.m10 = m.m01;
	result.m20 = m.m01;

	result.m01 = m.m10;
	result.m11 = m.m11;
	result.m21 = m.m12;

	result.m02 = m.m20;
	result.m12 = m.m21;
	result.m22 = m.m22;

	return result;
}



m4 tgm_m4_angle_axis(f32 angle_in_radians, v3 axis)
{
	m4 result = { 0 };

	const f32 c = tgm_f32_cos(angle_in_radians);
	const f32 s = tgm_f32_sin(angle_in_radians);
	const f32 omc = 1.0f - c;
	const f32 l = tgm_v3_mag(axis);
	TG_ASSERT(l);
	const f32 x = axis.x / l;
	const f32 y = axis.y / l;
	const f32 z = axis.z / l;

	result.m00 = x * x * omc + c;
	result.m10 = y * x * omc + z * s;
	result.m20 = z * x * omc - y * s;
	result.m30 = 0.0f;

	result.m01 = x * y * omc - z * s;
	result.m11 = y * y * omc + c;
	result.m21 = z * y * omc + x * s;
	result.m31 = 0.0f;

	result.m02 = x * z * omc + y * s;
	result.m12 = y * z * omc - x * s;
	result.m22 = z * z * omc + c;
	result.m32 = 0.0f;

	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	return result;
}

f32 tgm_m4_determinant(m4 m)
{
	const f32 result =
		m.m03 * m.m12 * m.m21 * m.m30 - m.m02 * m.m03 * m.m21 * m.m30 -
		m.m03 * m.m11 * m.m22 * m.m30 + m.m01 * m.m03 * m.m22 * m.m30 +
		m.m02 * m.m11 * m.m23 * m.m30 - m.m01 * m.m02 * m.m23 * m.m30 -
		m.m03 * m.m12 * m.m20 * m.m31 + m.m02 * m.m03 * m.m20 * m.m31 +
		m.m03 * m.m10 * m.m22 * m.m31 - m.m00 * m.m03 * m.m22 * m.m31 -
		m.m02 * m.m10 * m.m23 * m.m31 + m.m00 * m.m02 * m.m23 * m.m31 +
		m.m03 * m.m11 * m.m20 * m.m32 - m.m01 * m.m03 * m.m20 * m.m32 -
		m.m03 * m.m10 * m.m21 * m.m32 + m.m00 * m.m03 * m.m21 * m.m32 +
		m.m01 * m.m10 * m.m23 * m.m32 - m.m00 * m.m01 * m.m23 * m.m32 -
		m.m02 * m.m11 * m.m20 * m.m33 + m.m01 * m.m02 * m.m20 * m.m33 +
		m.m02 * m.m10 * m.m21 * m.m33 - m.m00 * m.m02 * m.m21 * m.m33 -
		m.m01 * m.m10 * m.m22 * m.m33 + m.m00 * m.m01 * m.m22 * m.m33;

	return result;
}

m4 tgm_m4_euler(f32 pitch_in_radians, f32 yaw_in_radians, f32 roll_in_radians)
{
	const m4 x = tgm_m4_rotate_x(pitch_in_radians);
	const m4 y = tgm_m4_rotate_y(yaw_in_radians);
	const m4 z = tgm_m4_rotate_z(roll_in_radians);
	const m4 yx = tgm_m4_mul(y, x);
	const m4 zyx = tgm_m4_mul(z, yx);
	return zyx;
}

m4 tgm_m4_identity(void)
{
	m4 result = { 0 };

	result.m00 = 1.0f;
	result.m10 = 0.0f;
	result.m20 = 0.0f;
	result.m30 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = 1.0f;
	result.m21 = 0.0f;
	result.m31 = 0.0f;

	result.m02 = 0.0f;
	result.m12 = 0.0f;
	result.m22 = 1.0f;
	result.m32 = 0.0f;

	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	return result;
}

m4 tgm_m4_inverse(m4 m)
{
	m4 result = { 0 };

	const f32 m2323 = m.m22 * m.m33 - m.m23 * m.m32;
	const f32 m1323 = m.m21 * m.m33 - m.m23 * m.m31;
	const f32 m1223 = m.m21 * m.m32 - m.m22 * m.m31;
	const f32 m0323 = m.m20 * m.m33 - m.m23 * m.m30;
	const f32 m0223 = m.m20 * m.m32 - m.m22 * m.m30;
	const f32 m0123 = m.m20 * m.m31 - m.m21 * m.m30;
	const f32 m2313 = m.m12 * m.m33 - m.m13 * m.m32;
	const f32 m1313 = m.m11 * m.m33 - m.m13 * m.m31;
	const f32 m1213 = m.m11 * m.m32 - m.m12 * m.m31;
	const f32 m2312 = m.m12 * m.m23 - m.m13 * m.m22;
	const f32 m1312 = m.m11 * m.m23 - m.m13 * m.m21;
	const f32 m1212 = m.m11 * m.m22 - m.m12 * m.m21;
	const f32 m0313 = m.m10 * m.m33 - m.m13 * m.m30;
	const f32 m0213 = m.m10 * m.m32 - m.m12 * m.m30;
	const f32 m0312 = m.m10 * m.m23 - m.m13 * m.m20;
	const f32 m0212 = m.m10 * m.m22 - m.m12 * m.m20;
	const f32 m0113 = m.m10 * m.m31 - m.m11 * m.m30;
	const f32 m0112 = m.m10 * m.m21 - m.m11 * m.m20;

	const f32 det = 1.0f / (
		m.m00 * (m.m11 * m2323 - m.m12 * m1323 + m.m13 * m1223) -
		m.m01 * (m.m10 * m2323 - m.m12 * m0323 + m.m13 * m0223) +
		m.m02 * (m.m10 * m1323 - m.m11 * m0323 + m.m13 * m0123) -
		m.m03 * (m.m10 * m1223 - m.m11 * m0223 + m.m12 * m0123));

	result.m00 = det *  (m.m11 * m2323 - m.m12 * m1323 + m.m13 * m1223);
	result.m01 = det * -(m.m01 * m2323 - m.m02 * m1323 + m.m03 * m1223);
	result.m02 = det *  (m.m01 * m2313 - m.m02 * m1313 + m.m03 * m1213);
	result.m03 = det * -(m.m01 * m2312 - m.m02 * m1312 + m.m03 * m1212);
	result.m10 = det * -(m.m10 * m2323 - m.m12 * m0323 + m.m13 * m0223);
	result.m11 = det *  (m.m00 * m2323 - m.m02 * m0323 + m.m03 * m0223);
	result.m12 = det * -(m.m00 * m2313 - m.m02 * m0313 + m.m03 * m0213);
	result.m13 = det *  (m.m00 * m2312 - m.m02 * m0312 + m.m03 * m0212);
	result.m20 = det *  (m.m10 * m1323 - m.m11 * m0323 + m.m13 * m0123);
	result.m21 = det * -(m.m00 * m1323 - m.m01 * m0323 + m.m03 * m0123);
	result.m22 = det *  (m.m00 * m1313 - m.m01 * m0313 + m.m03 * m0113);
	result.m23 = det * -(m.m00 * m1312 - m.m01 * m0312 + m.m03 * m0112);
	result.m30 = det * -(m.m10 * m1223 - m.m11 * m0223 + m.m12 * m0123);
	result.m31 = det *  (m.m00 * m1223 - m.m01 * m0223 + m.m02 * m0123);
	result.m32 = det * -(m.m00 * m1213 - m.m01 * m0213 + m.m02 * m0113);
	result.m33 = det *  (m.m00 * m1212 - m.m01 * m0212 + m.m02 * m0112);

	return result;
}

m4 tgm_m4_look_at(v3 from, v3 to, v3 up)
{
	TG_ASSERT(!tgm_v3_equal(from, to) && !tgm_v3_equal(from, up) && !tgm_v3_equal(to, up));

	m4 result = { 0 };

	const v3 f_negated = tgm_v3_normalized(tgm_v3_sub(to, from));
	const v3 r = tgm_v3_normalized(tgm_v3_cross(f_negated, tgm_v3_normalized(up)));
	const v3 u = tgm_v3_normalized(tgm_v3_cross(r, f_negated));
	const v3 f = tgm_v3_neg(f_negated);
	const v3 from_negated = tgm_v3_neg(from);

	result.m00 = r.x;
	result.m10 = u.x;
	result.m20 = f.x;
	result.m30 = 0.0f;

	result.m01 = r.y;
	result.m11 = u.y;
	result.m21 = f.y;
	result.m31 = 0.0f;

	result.m02 = r.z;
	result.m12 = u.z;
	result.m22 = f.z;
	result.m32 = 0.0f;

	result.m03 = tgm_v3_dot(r, from_negated);
	result.m13 = tgm_v3_dot(u, from_negated);
	result.m23 = tgm_v3_dot(f, from_negated);
	result.m33 = 1.0f;

	return result;
}

m4 tgm_m4_mul(m4 m0, m4 m1)
{
	m4 result = { 0 };

	result.m00 = m0.m00 * m1.m00 + m0.m01 * m1.m10 + m0.m02 * m1.m20 + m0.m03 * m1.m30;
	result.m10 = m0.m10 * m1.m00 + m0.m11 * m1.m10 + m0.m12 * m1.m20 + m0.m13 * m1.m30;
	result.m20 = m0.m20 * m1.m00 + m0.m21 * m1.m10 + m0.m22 * m1.m20 + m0.m23 * m1.m30;
	result.m30 = m0.m30 * m1.m00 + m0.m31 * m1.m10 + m0.m32 * m1.m20 + m0.m33 * m1.m30;

	result.m01 = m0.m00 * m1.m01 + m0.m01 * m1.m11 + m0.m02 * m1.m21 + m0.m03 * m1.m31;
	result.m11 = m0.m10 * m1.m01 + m0.m11 * m1.m11 + m0.m12 * m1.m21 + m0.m13 * m1.m31;
	result.m21 = m0.m20 * m1.m01 + m0.m21 * m1.m11 + m0.m22 * m1.m21 + m0.m23 * m1.m31;
	result.m31 = m0.m30 * m1.m01 + m0.m31 * m1.m11 + m0.m32 * m1.m21 + m0.m33 * m1.m31;

	result.m02 = m0.m00 * m1.m02 + m0.m01 * m1.m12 + m0.m02 * m1.m22 + m0.m03 * m1.m32;
	result.m12 = m0.m10 * m1.m02 + m0.m11 * m1.m12 + m0.m12 * m1.m22 + m0.m13 * m1.m32;
	result.m22 = m0.m20 * m1.m02 + m0.m21 * m1.m12 + m0.m22 * m1.m22 + m0.m23 * m1.m32;
	result.m32 = m0.m30 * m1.m02 + m0.m31 * m1.m12 + m0.m32 * m1.m22 + m0.m33 * m1.m32;

	result.m03 = m0.m00 * m1.m03 + m0.m01 * m1.m13 + m0.m02 * m1.m23 + m0.m03 * m1.m33;
	result.m13 = m0.m10 * m1.m03 + m0.m11 * m1.m13 + m0.m12 * m1.m23 + m0.m13 * m1.m33;
	result.m23 = m0.m20 * m1.m03 + m0.m21 * m1.m13 + m0.m22 * m1.m23 + m0.m23 * m1.m33;
	result.m33 = m0.m30 * m1.m03 + m0.m31 * m1.m13 + m0.m32 * m1.m23 + m0.m33 * m1.m33;

	return result;
}

v4 tgm_m4_mulv4(m4 m, v4 v)
{
	v4 result = { 0 };

	result.x = v.x * m.m00 + v.y * m.m01 + v.z * m.m02 + v.w * m.m03;
	result.y = v.x * m.m10 + v.y * m.m11 + v.z * m.m12 + v.w * m.m13;
	result.z = v.x * m.m20 + v.y * m.m21 + v.z * m.m22 + v.w * m.m23;
	result.w = v.x * m.m30 + v.y * m.m31 + v.z * m.m32 + v.w * m.m33;

	return result;
}

m4 tgm_m4_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near)
{
	TG_ASSERT(left != right && top != bottom && near != far);

	m4 result = { 0 };

	result.m00 = 2.0f / (right - left);
	result.m10 = 0.0f;
	result.m20 = 0.0f;
	result.m30 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = -2.0f / (top - bottom);
	result.m21 = 0.0f;
	result.m31 = 0.0f;

	result.m02 = 0.0f;
	result.m12 = 0.0f;
	result.m22 = -1.0f / (near - far);
	result.m32 = 0.0f;

	result.m03 = -(right + left) / (right - left);
	result.m13 = -(bottom + top) / (bottom - top);
	result.m23 = near / (near - far);
	result.m33 = 1.0f;

	return result;
}

m4 tgm_m4_perspective(f32 fov_y_in_radians, f32 aspect, f32 near, f32 far)
{
	m4 result = { 0 };

	const f32 tan_half_fov_y = tgm_f32_tan(fov_y_in_radians / 2.0f);

	const f32 a = -far / (far - near);
	const f32 b = -(2.0f * far * near) / (near - far);

	result.m00 = 1.0f / (aspect * tan_half_fov_y);
	result.m10 = 0.0f;
	result.m20 = 0.0f;
	result.m30 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = -1.0f / tan_half_fov_y;
	result.m21 = 0.0f;
	result.m31 = 0.0f;

	result.m02 = 0.0f;
	result.m12 = 0.0f;
	result.m22 = a;
	result.m32 = -1.0f;

	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = b;
	result.m33 = 0.0f;

	return result;
}

m4 tgm_m4_rotate_x(f32 angle_in_radians)
{
	m4 result = { 0 };

	result.m00 = 1.0f;
	result.m10 = 0.0f;
	result.m20 = 0.0f;
	result.m30 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = tgm_f32_cos(angle_in_radians);
	result.m21 = tgm_f32_sin(angle_in_radians);
	result.m31 = 0.0f;

	result.m02 = 0.0f;
	result.m12 = -tgm_f32_sin(angle_in_radians);
	result.m22 = tgm_f32_cos(angle_in_radians);
	result.m32 = 0.0f;

	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	return result;
}

m4 tgm_m4_rotate_y(f32 angle_in_radians)
{
	m4 result = { 0 };

	result.m00 = tgm_f32_cos(angle_in_radians);
	result.m10 = 0.0f;
	result.m20 = -tgm_f32_sin(angle_in_radians);
	result.m30 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = 1.0f;
	result.m21 = 0.0f;
	result.m31 = 0.0f;

	result.m02 = tgm_f32_sin(angle_in_radians);
	result.m12 = 0.0f;
	result.m22 = tgm_f32_cos(angle_in_radians);
	result.m32 = 0.0f;

	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	return result;
}

m4 tgm_m4_rotate_z(f32 angle_in_radians)
{
	m4 result = { 0 };

	result.m00 = tgm_f32_cos(angle_in_radians);
	result.m10 = tgm_f32_sin(angle_in_radians);
	result.m20 = 0.0f;
	result.m30 = 0.0f;

	result.m01 = -tgm_f32_sin(angle_in_radians);
	result.m11 = tgm_f32_cos(angle_in_radians);
	result.m21 = 0.0f;
	result.m31 = 0.0f;

	result.m02 = 0.0f;
	result.m12 = 0.0f;
	result.m22 = 1.0f;
	result.m32 = 0.0f;

	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	return result;
}

m4 tgm_m4_scale(v3 v)
{
	m4 result = { 0 };

	result.m00 = v.x;
	result.m10 = 0.0f;
	result.m20 = 0.0f;
	result.m30 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = v.y;
	result.m21 = 0.0f;
	result.m31 = 0.0f;

	result.m02 = 0.0f;
	result.m12 = 0.0f;
	result.m22 = v.z;
	result.m32 = 0.0f;

	result.m03 = 0.0f;
	result.m13 = 0.0f;
	result.m23 = 0.0f;
	result.m33 = 1.0f;

	return result;
}

m4 tgm_m4_translate(v3 v)
{
	m4 result = { 0 };

	result.m00 = 1.0f;
	result.m10 = 0.0f;
	result.m20 = 0.0f;
	result.m30 = 0.0f;

	result.m01 = 0.0f;
	result.m11 = 1.0f;
	result.m21 = 0.0f;
	result.m31 = 0.0f;

	result.m02 = 0.0f;
	result.m12 = 0.0f;
	result.m22 = 1.0f;
	result.m32 = 0.0f;

	result.m03 = v.x;
	result.m13 = v.y;
	result.m23 = v.z;
	result.m33 = 1.0f;

	return result;
}

m4 tgm_m4_transposed(m4 m)
{
	m4 result = { 0 };

	result.m00 = m.m00;
	result.m10 = m.m01;
	result.m20 = m.m02;
	result.m30 = m.m03;

	result.m01 = m.m10;
	result.m11 = m.m11;
	result.m21 = m.m12;
	result.m31 = m.m13;

	result.m02 = m.m20;
	result.m12 = m.m21;
	result.m22 = m.m22;
	result.m32 = m.m23;

	result.m03 = m.m30;
	result.m13 = m.m31;
	result.m23 = m.m32;
	result.m33 = m.m33;

	return result;
}
