struct tg_rand_xorshift32
{
	u32 state;
};

void tgm_rand_xorshift32_init(u32 seed, out tg_rand_xorshift32 state)
{
	state = tg_rand_xorshift32(seed);
}

u32 tgm_rand_xorshift32_next_u32(inout tg_rand_xorshift32 state)
{
	u32 result = state.state;
	result ^= result << 13;
	result ^= result >> 17;
	result ^= result << 5;
	state.state = result;
	return result;
}

f32 tgm_rand_xorshift32_next_f32(inout tg_rand_xorshift32 state)
{
	f32 result = f32(tgm_rand_xorshift32_next_u32(state)) / f32(TG_U32_MAX);
	return result;
}

f32 tgm_rand_xorshift32_next_f32_inclusive_range(inout tg_rand_xorshift32 state, f32 low, f32 high)
{
	f32 result = tgm_rand_xorshift32_next_f32(state) * (high - low) + low;
	return result;
}



v3 tg_min_component_wise(v3 a, v3 b)
{
    return v3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}

v3 tg_max_component_wise(v3 a, v3 b)
{
    return v3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}



u32 tg_hash_u32(u32 v) // MurmurHash3 algorithm by Austin Appleby
{
	u32 result = v;
	result ^= result >> 16;
	result *= 0x85ebca6b;
	result ^= result >> 13;
	result *= 0xc2b2ae35;
	result ^= result >> 16;
	return result;
}
