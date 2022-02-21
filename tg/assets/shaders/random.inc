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
